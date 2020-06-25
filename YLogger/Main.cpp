#include "YLogger.h" // only include single header file

void func1(int i) {
    LOG_DEBUG("로그 - " + std::to_string(i));
}

void func2(int i) {
    LOG_INFO("로그2 - " + std::to_string(i));
}

int main() {
    system("chcp 949");
    logger::YLogger::Initialize(); // default config
    //logger::YLogger::Initialize(".\YLogger.config"); // put config path
    LOG_DEBUG("로그 1 입니다.");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (int i = 0; i < 1000; i++) {
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
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    LOG_FATAL("로그 종료");

    return 0;
}