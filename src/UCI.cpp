#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <future>

#include "UCI.h"

static void GetToken(std::istringstream& uip, std::string& token) {
    token.clear();
    uip >> token;
}

void UCI::Start() {
    std::string command;
    bool stop = false;
    bool uciMode = false;
    std::string token;
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
            uciMode = true;
            printf("id name MilesBot 1.0\n");
            printf("id author Miles\n");
            printf("uciok\n");
        } else if (token == "isready") {
            printf("readyok\n");
        } else if (token == "position") {
            GetToken(istream, token);
            if (token == "startpos") {
                GetToken(istream, token);
                if (token == "moves") {
                    int i = 0;
                    bool reset = false;

                    // Cache moves from before
                    std::string moves[CACHED_MOVES_LENGTH] = {};
                    while (!istream.eof()) {
                        GetToken(istream, token);
                        moves[i] = token;
                        if (i >= m_CachedLength && !reset) {
                            m_CachedMoves[i] = token;
                            Move move = m_Search.GetMove(token);
                            m_Search.m_Position.MovePiece(move);
                            m_CachedLength++;
                        } else if (m_CachedMoves[i] != token) {
                            m_CachedLength = 0;
                            if (!reset) {
                                m_Search.LoadPosition(Lookup::starting_pos);
                                int k = 0;
                                while (m_CachedMoves[k] != "") {
                                    m_CachedMoves[k] = "";
                                    k++;
                                }
                                reset = true;
                            }
                        }
                        i++;
                    }
                    if (i < m_CachedLength) {
                        m_CachedLength = 0;
                        m_Search.LoadPosition(Lookup::starting_pos);
                        int k = 0;
                        while (m_CachedMoves[k] != "") {
                            m_CachedMoves[k] = "";
                            k++;
                        }
                        reset = true;
                    }

                    if (reset) {
                        for (int j = 0; j < i; j++) {
                            token = moves[j];
                            m_CachedMoves[j] = token;
                            m_CachedLength++;
                            Move move = m_Search.GetMove(token);
                            m_Search.m_Position.MovePiece(move);
                        }
                    }


                    printf("info %s\n", m_Search.m_Position.ToFen().c_str());
                } else {
                    m_Search.LoadPosition(Lookup::starting_pos);
                }
            } else if (token == "fen") {
                std::string fen;
                while (!istream.eof()) {
                    GetToken(istream, token);
                    fen += token;
                    fen += " ";
                }
                m_Search.LoadPosition(fen);
            }
        } else if (token == "stop") {
            m_Search.Stop();
        } else if (token == "go") {
            int64 time = 1000;
            int64 wtime = -1;
            int64 btime = -1;
            int64 winc = 0;
            int64 binc = 0;

            GetToken(istream, token);
            if (token == "depth") {
                GetToken(istream, token);
                int depth = std::atoi(token.c_str());
                std::async(std::launch::async, &Search::Perft, &m_Search, depth);
                continue;
            } else if (token == "movetime") {
                GetToken(istream, token);
                time = std::atoi(token.c_str());
            } else if (token == "infinite") {
                time = -1;
            }
            while (!istream.eof()) {
                if (token == "wtime") {
                    GetToken(istream, token);
                    wtime = std::atoi(token.c_str());
                } else if (token == "btime") {
                    GetToken(istream, token);
                    btime = std::atoi(token.c_str());
                } else if (token == "winc") {
                    GetToken(istream, token);
                    winc = std::atoi(token.c_str());
                } else if (token == "binc") {
                    GetToken(istream, token);
                    binc = std::atoi(token.c_str());
                }
                GetToken(istream, token);
            }
            if (wtime > 0 && btime > 0) {
                auto a = std::async(std::launch::async, &Search::MoveTimed, &m_Search, wtime, btime, winc, binc);
                continue;
            }
            else {
                m_Search.UCIMove(time);
                continue;
            }
        }
    }
}