#pragma once
#include "Search.h"

#define CACHED_MOVES_LENGTH 512

class UCI {
public:
	Search m_Search;

	

	UCI() 
		: m_CachedLength(0)
	{
		for (int i = 0; i < CACHED_MOVES_LENGTH; i++) {
			m_CachedMoves[i].reserve(5);
		}
	}

	void Start();
private:
	std::string m_CachedMoves[CACHED_MOVES_LENGTH];
	int m_CachedLength;
};