#include "YLogger.h" // only include single header file

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

void func1(int i) {
	LOG_DEBUG("로그 - " + std::to_string(i));
}

void func2(int i) {
	LOG_INFO("로그2 - " + std::to_string(i));
}

int main() {
	system("chcp 949");
	//logger::YLogger::Initialize(); // Default config, Not essential. default is logger::LogLevel::Debug
	logger::YLogger::Initialize(logger::LogLevel::Info); // Set logger level
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::ConsoleAppender);
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::FileAppender, "./temp2", "Hello");

	while (true) {
		std::cout << "Write 10000 lines" << std::endl;
		for (int i = 0; i < 10000; i++) {
			if (i % 2 == 0) {
				func1(i);
			}
			else {
				func2(i);
			}
			std::this_thread::sleep_for(10s);
		}
	}

	return 0;
}