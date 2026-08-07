#pragma once
#include "Search.h"

#include <string>
#include <sstream>
#include <vector>

class UCI {
public:
    Search m_Search;

    UCI() = default;

    void Start();

private:
    void SetPosition(const std::string& fen, const std::vector<std::string>& moves);
    void SetOption(std::istringstream& istream);
    void NewGame();

    std::string m_CachedFen;
    std::vector<std::string> m_CachedMoves;
};
