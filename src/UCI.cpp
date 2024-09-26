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
                //printf("info %s\n", command.c_str());
                m_Search.LoadPosition(Lookup::starting_pos);
                GetToken(istream, token);
                if (token == "moves") {
                    while (!istream.eof()) {
                        GetToken(istream, token);
                        Move move = m_Search.GetMove(token);
                        m_Search.MoveRootPiece(move);
                        printf("info %s, %s\n", move.toString().c_str(), BoardtoFen(*m_Search.m_RootBoard, m_Search.m_Info[0]).c_str());
                    }
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
            int64 time = 10000;
            GetToken(istream, token);
            if (token == "depth") {
                GetToken(istream, token);
                int depth = std::atoi(token.c_str());
                std::async(std::launch::async, &Search::Perft, &m_Search, depth);
                continue;
            } else if (token == "movetime") {
                GetToken(istream, token);
                time = std::atoi(token.c_str());
            }
            auto a = std::async(std::launch::async, &Search::UCIMove, &m_Search, time);
        }
    }
}