#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

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

        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        std::istringstream istream(command);
        GetToken(istream, token);

        if (token == "quit") {
            stop = true;
        }
        else if (token == "uci") {
            uciMode = true;
            printf("id name MilesBot 1.0\n");
            printf("id author Sebastian M\n");
            printf("uciok\n");
        }
        else if (token == "position") {
            GetToken(istream, token);
            if (token == "startpos") {

            }
            else if (token == "fen") {
                std::string fen;
                while (!istream.eof()) {
                    GetToken(istream, token);
                    fen += token;
                    fen += " ";
                }
            }
        }
    }
}