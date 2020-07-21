#include "YLogger.h" // only include single header file

void func1(int i) {
	LOG_DEBUG("로그 - " + std::to_string(i));
}

void func2(int i) {
	LOG_INFO("로그2 - " + std::to_string(i));
}

int main() {
	//system("chcp 949");
	//logger::YLogger::Initialize(); // Default config, Not essential. default is logger::LogLevel::Debug
	logger::YLogger::Initialize(logger::LogLevel::Info); // Set logger level
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::ConsoleAppender);
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::FileAppender, "./temp2", "Hello");

	std::cout << "Write 100 lines" << std::endl;

	for (int i = 0; i < 100; i++) {
		std::thread tt([i]() {
			// thread-safe log
			if (i % 2 == 0) {
				func1(i);
			}
			else {
				func2(i);
			}
			});
		tt.detach();
	}

	return 0;
}