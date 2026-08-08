#!/usr/bin/env python3
"""Play against MilesBot in a browser.

Serves a board at http://localhost:8080 and drives ./build/engine over UCI.
The engine stays authoritative: legal moves come from `perft 1` and the
position is read back with `d`, so the GUI never second guesses the movegen.

    ./play.py                     # localhost only
    ./play.py --host 0.0.0.0      # let others on the LAN join the same game
    ./play.py --engine ./build/engine --hash 256
"""

import argparse
import json
import os
import queue
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
HERE = os.path.dirname(os.path.abspath(__file__))
PAGE = os.path.join(HERE, "tools", "play.html")


# --------------------------------------------------------------------------
# Engine
# --------------------------------------------------------------------------


class EngineError(RuntimeError):
    pass


def _int(text):
    try:
        return int(text)
    except ValueError:
        return text


class Engine:
    """One UCI subprocess, serialized behind a lock."""

    def __init__(self, path, hash_mb=None, syzygy=None):
        self.path = path
        self.hash_mb = hash_mb
        self.syzygy = syzygy
        self.lock = threading.RLock()
        self.lines = queue.Queue()
        self.info = {}  # latest search info, read by /api/info while thinking
        self.info_lock = threading.Lock()
        self.proc = None
        self.start()

    def start(self):
        if not os.path.exists(self.path):
            raise EngineError(
                f"engine not found at {self.path} - build it first with `make engine`"
            )
        self.proc = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
            cwd=HERE,
        )
        threading.Thread(target=self._reader, daemon=True).start()
        self.send("uci")
        self.expect("uciok")
        if self.hash_mb:
            self.send(f"setoption name Hash value {self.hash_mb}")
        if self.syzygy:
            self.send(f"setoption name SyzygyPath value {self.syzygy}")
        self.send("ucinewgame")
        self.send("isready")
        self.expect("readyok")

    def _reader(self):
        proc = self.proc
        for line in proc.stdout:
            line = line.rstrip("\n")
            if line.startswith("info "):
                self._note_info(line)
            self.lines.put(line)
        self.lines.put(None)  # engine died

    def _note_info(self, line):
        parts = line.split()[1:]
        info = {}
        i = 0
        while i < len(parts):
            key = parts[i]
            if key == "pv":  # "pv <move> <move> ..." runs to the end of the line
                info["pv"] = parts[i + 1:]
                break
            if key == "string":
                info["string"] = " ".join(parts[i + 1:])
                break
            if key == "score" and i + 2 < len(parts):  # "score cp <n>" / "score mate <n>"
                info["scoreType"] = parts[i + 1]
                info["score"] = _int(parts[i + 2])
                i += 3
                continue
            if i + 1 >= len(parts):
                break
            info[key] = _int(parts[i + 1])
            i += 2
        with self.info_lock:
            self.info.update(info)

    def alive(self):
        return self.proc is not None and self.proc.poll() is None

    def send(self, command):
        if not self.alive():
            raise EngineError("engine process is not running")
        try:
            self.proc.stdin.write(command + "\n")
            self.proc.stdin.flush()
        except (BrokenPipeError, ValueError):
            raise EngineError("engine closed the pipe")

    def drain(self):
        while True:
            try:
                self.lines.get_nowait()
            except queue.Empty:
                return

    def expect(self, prefix, timeout=60.0):
        """Read lines until one starts with prefix; return the matching line."""
        collected = []
        while True:
            try:
                line = self.lines.get(timeout=timeout)
            except queue.Empty:
                raise EngineError(f"engine did not answer with `{prefix}` in time")
            if line is None:
                raise EngineError("engine exited unexpectedly")
            if line.startswith(prefix):
                return line
            collected.append(line)

    def read_until_blank(self, timeout=20.0):
        """Read lines up to the empty line that ends a perft listing."""
        collected = []
        while True:
            try:
                line = self.lines.get(timeout=timeout)
            except queue.Empty:
                raise EngineError("engine did not finish listing moves in time")
            if line is None:
                raise EngineError("engine exited unexpectedly")
            if line == "":
                return collected
            collected.append(line)

    def restart(self):
        with self.lock:
            if self.proc is not None:
                try:
                    self.proc.kill()
                except OSError:
                    pass
            self.lines = queue.Queue()
            self.start()

    def position(self, fen, moves):
        cmd = "position " + ("startpos" if fen == START_FEN else f"fen {fen}")
        if moves:
            cmd += " moves " + " ".join(moves)
        self.send(cmd)

    def legal_moves(self, fen, moves):
        """`perft 1` prints one line per legal root move."""
        with self.lock:
            self.drain()
            self.position(fen, moves)
            self.send("perft 1")
            lines = self.read_until_blank()
            return [l.split(":")[0] for l in lines if ":" in l]

    def current_fen(self, fen, moves):
        with self.lock:
            self.drain()
            self.position(fen, moves)
            self.send("d")
            return self.expect("Fen:", timeout=20.0).split(" ", 1)[1].strip()

    def search(self, fen, moves, movetime):
        """Think for movetime ms and return (bestmove, info)."""
        with self.lock:
            self.drain()
            with self.info_lock:
                self.info = {}
            self.position(fen, moves)
            self.send(f"go movetime {int(movetime)}")
            timeout = movetime / 1000.0 + 30.0
            best = self.expect("bestmove", timeout=timeout).split()
            with self.info_lock:
                info = dict(self.info)
            return (best[1] if len(best) > 1 else "0000"), info

    def stop(self):
        try:
            if self.alive():
                self.send("quit")
                self.proc.wait(timeout=3)
        except Exception:
            if self.proc is not None:
                self.proc.kill()


# --------------------------------------------------------------------------
# Just enough board knowledge for check / mate / draw reporting
# --------------------------------------------------------------------------


def board_of(fen):
    board = {}
    for i, row in enumerate(fen.split()[0].split("/")):
        rank, file = 7 - i, 0
        for ch in row:
            if ch.isdigit():
                file += int(ch)
            else:
                board[(file, rank)] = ch
                file += 1
    return board


def attacked(board, square, by_white):
    """Is `square` attacked by the white (or black) side?"""
    f, r = square
    pawn_rank = r - 1 if by_white else r + 1
    for df in (-1, 1):
        if board.get((f + df, pawn_rank)) == ("P" if by_white else "p"):
            return True
    for df, dr in ((1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)):
        if board.get((f + df, r + dr)) == ("N" if by_white else "n"):
            return True
    for df in (-1, 0, 1):
        for dr in (-1, 0, 1):
            if (df or dr) and board.get((f + df, r + dr)) == ("K" if by_white else "k"):
                return True
    rays = (
        (((1, 1), (1, -1), (-1, 1), (-1, -1)), "BQ"),
        (((1, 0), (-1, 0), (0, 1), (0, -1)), "RQ"),
    )
    for dirs, pieces in rays:
        for df, dr in dirs:
            x, y = f + df, r + dr
            while 0 <= x < 8 and 0 <= y < 8:
                piece = board.get((x, y))
                if piece:
                    if piece.isupper() == by_white and piece.upper() in pieces:
                        return True
                    break
                x, y = x + df, y + dr
    return False


def king_square(fen):
    board = board_of(fen)
    white_to_move = fen.split()[1] == "w"
    king = "K" if white_to_move else "k"
    for square, piece in board.items():
        if piece == king:
            return square, board, white_to_move
    return None, board, white_to_move


def in_check(fen):
    square, board, white_to_move = king_square(fen)
    if square is None:
        return False, None
    return attacked(board, square, not white_to_move), square


def validated_fen(fen):
    """Accept a pasted FEN, or fall back to the starting position."""
    if not fen or not fen.strip():
        return START_FEN
    fields = fen.split()
    board = fields[0]
    if len(board.split("/")) != 8 or "K" not in board or "k" not in board:
        raise ValueError("that does not look like a FEN with two kings")
    if len(fields) > 1 and fields[1] not in ("w", "b"):
        raise ValueError("FEN side to move must be w or b")
    defaults = ["", "w", "-", "-", "0", "1"]
    return " ".join(fields[i] if i < len(fields) else defaults[i] for i in range(6))


def insufficient_material(fen):
    pieces = [p for p in board_of(fen).values() if p.upper() != "K"]
    if not pieces:
        return True
    if len(pieces) == 1 and pieces[0].upper() in "BN":
        return True
    if len(pieces) == 2 and all(p.upper() == "B" for p in pieces):
        squares = [sq for sq, p in board_of(fen).items() if p.upper() == "B"]
        colors = {(f + r) % 2 for f, r in squares}
        return pieces[0].isupper() != pieces[1].isupper() and len(colors) == 1
    return False


# --------------------------------------------------------------------------
# Game state
# --------------------------------------------------------------------------


class Game:
    def __init__(self, engine):
        self.engine = engine
        self.lock = threading.RLock()
        self.thinking = False
        self.error = None
        self.reset()

    def stop_search(self, timeout=10.0):
        """Ask a running search to finish so we can touch the position again."""
        if not self.thinking:
            return
        try:
            self.engine.send("stop")
        except EngineError:
            return
        deadline = time.monotonic() + timeout
        while self.thinking and time.monotonic() < deadline:
            time.sleep(0.02)

    def reset(self, human_white=True, movetime=3000, start_fen=START_FEN):
        self.stop_search()
        with self.lock:
            self.start_fen = start_fen
            self.moves = []
            self.keys = []
            self.human_white = human_white
            self.movetime = movetime
            self.last_info = {}
            self.error = None
            with self.engine.lock:
                self.engine.send("ucinewgame")
            self.refresh()

    def refresh(self):
        self.fen = self.engine.current_fen(self.start_fen, self.moves)
        self.legal = self.engine.legal_moves(self.start_fen, self.moves)
        # keys[i] is the position after i moves; moves only grow or shrink at the end
        self.keys = self.keys[:len(self.moves)] + [" ".join(self.fen.split()[:4])]

    def repetitions(self):
        return self.keys.count(self.keys[-1])

    def result(self):
        """(code, text) for a finished game, or (None, None)."""
        check, _ = in_check(self.fen)
        if not self.legal:
            if check:
                winner = "Black" if self.fen.split()[1] == "w" else "White"
                return "mate", f"Checkmate - {winner} wins"
            return "stalemate", "Stalemate - draw"
        fields = self.fen.split()
        if len(fields) > 4 and fields[4].isdigit() and int(fields[4]) >= 100:
            return "fifty", "Draw by the 50 move rule"
        if insufficient_material(self.fen):
            return "material", "Draw - insufficient material"
        if len(self.moves) >= 8 and self.repetitions() >= 3:
            return "repetition", "Draw by threefold repetition"
        return None, None

    def state(self):
        with self.lock:
            check, king = in_check(self.fen)
            code, text = self.result()
            legal = {}
            for move in self.legal:
                legal.setdefault(move[:2], []).append(move)
            return {
                "fen": self.fen,
                "turn": self.fen.split()[1],
                "moves": self.moves,
                "legal": legal,
                "humanWhite": self.human_white,
                "movetime": self.movetime,
                "check": f"{chr(97 + king[0])}{king[1] + 1}" if check and king else None,
                "result": code,
                "resultText": text,
                "lastMove": self.moves[-1] if self.moves else None,
                "thinking": self.thinking,
                "info": self.last_info,
                "engineTurn": (self.fen.split()[1] == "w") != self.human_white and not code,
                "error": self.error,
            }

    def play(self, uci):
        with self.lock:
            if self.thinking:
                raise ValueError("the engine is still thinking")
            if uci not in self.legal:
                raise ValueError(f"illegal move {uci}")
            self.moves.append(uci)
            self.refresh()

    def engine_move(self):
        with self.lock:
            # Several browsers can share one board, so only ever run one search
            # and only when it really is the engine's turn.
            if self.thinking or self.result()[0]:
                return self.state()
            if (self.fen.split()[1] == "w") == self.human_white:
                return self.state()
            self.thinking = True
            self.error = None
            white_to_move = self.fen.split()[1] == "w"
        try:
            best, info = self.engine.search(self.start_fen, self.moves, self.movetime)
            with self.lock:
                if "score" in info:  # pin the sign now; the side to move is about to flip
                    info["whiteScore"] = info["score"] if white_to_move else -info["score"]
                self.last_info = info
                if best in self.legal:
                    self.moves.append(best)
                    self.refresh()
                else:
                    self.error = f"engine returned {best}, which is not legal here"
        except EngineError as err:
            with self.lock:
                self.error = str(err)
        finally:
            with self.lock:
                self.thinking = False
        return self.state()

    def hint(self):
        best, info = self.engine.search(self.start_fen, self.moves, min(self.movetime, 2000))
        return {"move": best, "info": info}

    def undo(self):
        """Take back to the human's previous turn."""
        with self.lock:
            if self.thinking:
                raise ValueError("the engine is still thinking")
            if not self.moves:
                return self.state()
            self.moves.pop()
            white_to_move = len(self.moves) % 2 == 0
            if self.moves and white_to_move != self.human_white:
                self.moves.pop()
            self.last_info = {}
            self.error = None
            self.refresh()
            return self.state()


# --------------------------------------------------------------------------
# HTTP
# --------------------------------------------------------------------------


class Handler(BaseHTTPRequestHandler):
    game = None
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass  # the board is chatty enough

    def _send(self, code, body, content_type):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _json(self, payload, code=200):
        self._send(code, json.dumps(payload), "application/json")

    def _body(self):
        length = int(self.headers.get("Content-Length") or 0)
        return json.loads(self.rfile.read(length) or "{}")

    def do_GET(self):
        path = self.path.split("?")[0]
        if path == "/":
            try:
                with open(PAGE, "r", encoding="utf-8") as fh:
                    self._send(200, fh.read(), "text/html; charset=utf-8")
            except OSError:
                self._send(500, f"missing {PAGE}", "text/plain")
        elif path == "/api/state":
            self._json(self.game.state())
        elif path == "/api/info":
            with self.game.engine.info_lock:
                info = dict(self.game.engine.info)
            self._json({"thinking": self.game.thinking, "info": info})
        else:
            self._send(404, "not found", "text/plain")

    def do_POST(self):
        path = self.path.split("?")[0]
        try:
            if path == "/api/new":
                body = self._body()
                color = body.get("color", "white")
                if color == "random":
                    color = "white" if os.urandom(1)[0] % 2 else "black"
                self.game.reset(
                    human_white=color == "white",
                    movetime=max(50, min(int(body.get("movetime", 3000)), 120000)),
                    start_fen=validated_fen(body.get("fen")),
                )
                self._json(self.game.state())
            elif path == "/api/move":
                self.game.play(self._body().get("uci", ""))
                self._json(self.game.state())
            elif path == "/api/go":
                self._json(self.game.engine_move())
            elif path == "/api/hint":
                self._json(self.game.hint())
            elif path == "/api/undo":
                self._json(self.game.undo())
            elif path == "/api/movetime":
                self.game.movetime = max(50, min(int(self._body().get("movetime", 3000)), 120000))
                self._json(self.game.state())
            elif path == "/api/restart-engine":
                self.game.engine.restart()
                self.game.reset(self.game.human_white, self.game.movetime)
                self._json(self.game.state())
            else:
                self._send(404, "not found", "text/plain")
        except ValueError as err:
            self._json({"error": str(err)}, 400)
        except EngineError as err:
            self._json({"error": str(err)}, 503)


def main():
    parser = argparse.ArgumentParser(description="Play against MilesBot in a browser")
    parser.add_argument("--engine", default=os.path.join(HERE, "build", "engine"))
    parser.add_argument("--host", default="127.0.0.1", help="use 0.0.0.0 to open it to the LAN")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--hash", type=int, default=None, help="engine hash in MB")
    parser.add_argument("--syzygy", default=None, help="path to syzygy tablebases")
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()

    try:
        engine = Engine(args.engine, args.hash, args.syzygy)
    except EngineError as err:
        print(f"error: {err}", file=sys.stderr)
        return 1

    Handler.game = Game(engine)
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    shown = "localhost" if args.host in ("127.0.0.1", "0.0.0.0") else args.host
    url = f"http://{shown}:{args.port}"
    print(f"MilesBot is waiting on {url}   (ctrl-c to stop)")
    if args.host == "0.0.0.0":
        print("Reachable from the LAN - everyone who opens it shares the same board.")
    if not args.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")
    finally:
        engine.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
