#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "UCI.h"
#include "Perft.h"

static void GetToken(std::istringstream& uip, std::string& token) {
    token.clear();
    uip >> token;
}

// A fen is expected to have 6 fields, the parser reads past the end without them
static std::string BuildFen(const std::vector<std::string>& fields) {
    static const char* defaults[6] = { "8/8/8/8/8/8/8/8", "w", "-", "-", "0", "1" };
    std::string fen;
    for (int i = 0; i < 6; i++) {
        if (i > 0) fen += " ";
        fen += (size_t)i < fields.size() ? fields[i] : defaults[i];
    }
    return fen;
}

// setoption name <Name> value <Value>, either may contain spaces
void UCI::SetOption(std::istringstream& istream) {
    std::string token, name, value;
    if (!(istream >> token) || token != "name") return;

    bool inValue = false;
    while (istream >> token) {
        if (!inValue && token == "value") {
            inValue = true;
            continue;
        }
        std::string& target = inValue ? value : name;
        if (!target.empty()) target += " ";
        target += token;
    }

    if (name == "Hash") {
        int mb = std::atoi(value.c_str());
        if (mb >= 1) m_Search.SetHashSize(mb);
        else sync_printf("info string bad Hash value %s\n", value.c_str());
    } else if (name == "SyzygyPath") {
        if (!value.empty() && value != "<empty>") TableBase::Init(value);
    } else {
        sync_printf("info string unknown option %s\n", name.c_str());
    }
}

void UCI::NewGame() {
    m_Search.Stop();
    m_Search.ClearTables();
    m_Search.LoadPosition(Lookup::starting_pos);
    m_CachedFen = Lookup::starting_pos;
    m_CachedMoves.clear();
}

void UCI::SetPosition(const std::string& fen, const std::vector<std::string>& moves) {
    m_Search.Stop();

    size_t applied = 0;
    if (fen == m_CachedFen && moves.size() >= m_CachedMoves.size()
        && std::equal(m_CachedMoves.begin(), m_CachedMoves.end(), moves.begin())) {
        applied = m_CachedMoves.size(); // Extending the previous position keeps the transposition table
    } else {
        m_Search.LoadPosition(fen);
        m_CachedFen = fen;
        m_CachedMoves.clear();
    }

    for (size_t i = applied; i < moves.size(); i++) {
        const std::string& str = moves[i];
        Move move = str.size() >= 4 ? m_Search.GetMove(str) : Move();
        if (MovePieceType(move) == NOPIECE) {
            sync_printf("info string illegal move %s in position %s\n", str.c_str(),
                        m_Search.m_Position.ToFen().c_str());
            m_CachedFen.clear();
            return;
        }
        m_Search.m_Position.MovePiece(move);
        m_CachedMoves.push_back(str);
    }
}

void UCI::Start() {
    std::string command;
    bool stop = false;
    std::string token;

    m_CachedFen = Lookup::starting_pos;

    while (!stop) {
        if (!std::getline(std::cin, command)) {
            break;
        }
        command = trim_str(command);

        std::istringstream istream(command);
        GetToken(istream, token);

        if (token == "quit") {
            stop = true;
            m_Search.Stop();
        } else if (token == "uci") {
            printf("id name MilesBot 1.0\n");
            printf("id author Miles\n");
            printf("option name Hash type spin default %llu min 1 max 4096\n", DEFAULT_HASH_MB);
            printf("option name SyzygyPath type string default <empty>\n");
            printf("uciok\n");
        } else if (token == "isready") {
            printf("readyok\n");
        } else if (token == "setoption") {
            SetOption(istream);
        } else if (token == "ucinewgame") {
            NewGame();
        } else if (token == "position") {
            std::string fen;
            GetToken(istream, token);
            if (token == "startpos") {
                fen = Lookup::starting_pos;
                GetToken(istream, token);
            } else if (token == "fen") {
                std::vector<std::string> fields;
                while (istream >> token && token != "moves") {
                    fields.push_back(token);
                }
                fen = BuildFen(fields);
            } else {
                continue;
            }

            std::vector<std::string> moves;
            if (token == "moves") {
                while (istream >> token) {
                    moves.push_back(token);
                }
            }

            SetPosition(fen, moves);
        } else if (token == "stop") {
            m_Search.Stop();
        } else if (token == "perft") {
            GetToken(istream, token);
            int depth = std::atoi(token.c_str());
            if (depth > 0) PerftDivide(m_Search.m_Position, depth, true);
        } else if (token == "go") {
            int64 time = 1000;
            int64 wtime = -1;
            int64 btime = -1;
            int64 winc = 0;
            int64 binc = 0;
            int depth = MAX_DEPTH;
            int perft = 0;
            bool movetime = false;

            while (istream >> token) {
                if (token == "perft") {
                    istream >> token;
                    perft = std::atoi(token.c_str());
                } else if (token == "depth") {
                    istream >> token;
                    depth = std::clamp(std::atoi(token.c_str()), 1, MAX_DEPTH);
                    time = -1;
                } else if (token == "movetime") {
                    istream >> token;
                    time = std::atoi(token.c_str());
                    movetime = true;
                } else if (token == "infinite") {
                    time = -1;
                } else if (token == "wtime") {
                    istream >> token;
                    wtime = std::atoi(token.c_str());
                } else if (token == "btime") {
                    istream >> token;
                    btime = std::atoi(token.c_str());
                } else if (token == "winc") {
                    istream >> token;
                    winc = std::atoi(token.c_str());
                } else if (token == "binc") {
                    istream >> token;
                    binc = std::atoi(token.c_str());
                }
            }

            if (perft > 0) {
                PerftDivide(m_Search.m_Position, perft, true);
            } else if (!movetime && wtime > 0 && btime > 0) {
                m_Search.MoveTimed(wtime, btime, winc, binc, depth);
            } else {
                m_Search.UCIMove(time, depth);
            }
        }
    }

    m_Search.Stop();
}
