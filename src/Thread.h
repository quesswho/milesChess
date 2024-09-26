#pragma once
#include <thread>

class ThreadManger {

	std::unique_ptr<std::thread> m_Thread;

};