#include <iostream>

#include "YLogger.h"

void func() {

}

int main() {
	system("chcp 949");
	logger::YLogger::Initialize(); // default config
	//logger::YLogger::Initialize(".\YLogger.config"); // put config path
	LOG_DEBUG("로그 1 입니다.");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	for (int i = 0; i < 1000; i++) {
		std::thread tt([i]() {			
			if (i % 2 == 0) {
				LOG_DEBUG("로그" + std::to_string(i));
				//logger.Info("로그" + std::to_string(i));
			}
			else {
				LOG_INFO("로그" + std::to_string(i));
				//logger.Debug("로그" + std::to_string(i));
			}
			});
		tt.detach();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	LOG_FATAL("로그 종료");
}
