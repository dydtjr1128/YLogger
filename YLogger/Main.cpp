#include "YLogger.h" // only include single header file

void func1(int i) {
	LOG_DEBUG("로그 - " + std::to_string(i));
}

void func2(int i) {
	LOG_INFO("로그2 - " + std::to_string(i));
}

int main() {
	//logger::YLogger::Initialize(); // default config, Not essential.
	//logger::YLogger::Initialize(".\YLogger.config"); // put config path
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::ConsoleAppender);
	logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::FileAppender);

	std::cout << "FileAppender write 100,000 lines" << std::endl;
	logger::SetStartTime();

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