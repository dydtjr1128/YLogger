#include <iostream>

#include "YLogger.h"

int main() {
	system("chcp 949");
	logger::YLogger logger;
	logger.Info("로그 1 입니다.");
	
	for (int i = 0; i < 1000; i++) {
		std::thread tt([i,&logger]() {			
			if(i%2 == 0)
				logger.Info("로그" + std::to_string(i));
			else
				logger.Debug("로그" + std::to_string(i));
			});
		tt.detach();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}
