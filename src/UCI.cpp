#include <string>
#include <iostream>
#include <sstream>

#include "UCI.h"

void UCI::Start() {
    std::string command;
    bool stop = false;
    bool uciMode = false;
    while (!stop) {
        if (!std::getline(std::cin, command)) {
            break;
        }
        std::istringstream istream(command);
        std::string token;
        istream >> token;
    }
}