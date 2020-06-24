#ifndef YLOGGER_H
#define YLOGGER_H

#include <chrono>
#include <condition_variable>
#include <ctime>   // localtime
#include <filesystem>
#include <functional>
#include <iomanip> // put_time
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace logger {

	namespace {
		constexpr size_t kDefaultFileWatchingTime = 1000;
	}

	enum class LoggerType {
		ConsoleAppender,
		FileAppender,
		RollingFileAppender
	};

	enum class LogLevel : int {
		Trace = 0,
		Debug,
		Info,
		Warn,
		Error,
		Fatal
	};
	std::string logLevel[] = { "Trace", "Debug", "Info", "Warn", "Error", "Fatal" };

	class Log {
	public:
		explicit Log(LogLevel level, const std::string& log) :level_(level), log_(log) {};
		LogLevel level() const { return level_; }
		std::string log() const { return log_; }
	private:
		LogLevel level_;
		std::string log_;
	};

	class Appender {
	public:
		virtual void action(const Log& log) = 0;
	private:
	};

	class ConsoleAppender : public Appender {
	public:

		void action(const Log& log) {

			std::cout << log.log();
		}
	private:
	};

	class YLogger {
	public:
		static YLogger* getLogger() {
			if (logger == nullptr)
				logger = new YLogger();
			return logger;
		}
		explicit YLogger(std::filesystem::path savePath = std::filesystem::current_path()) : savePath_(savePath), finish_(false), logLevel_(LogLevel::Debug) {
			loggingThread_ = std::thread([&] {
				while (true) {
					std::unique_lock lock(logMutex_);
					log_cv.wait(lock, [&] {return !logQueue_.empty() || finish_; });

					if (logQueue_.empty() && finish_) {
						break;
					}

					const Log log = logQueue_.front();
					logQueue_.pop();
					lock.unlock();

					for (const auto& appender : logTypeVector_) {
						appender->action(log);
					}
				}
				});
			ReadConfig();
			std::thread t([&]() {
				while (true) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					std::cout << "@@" << logQueue_.size() << std::endl;
				}
				});
			t.detach();
		}
		~YLogger() {
			std::cout << logQueue_.size() << std::endl;
			finish_ = true;
			log_cv.notify_all();
			loggingThread_.join();
			std::cout << logQueue_.size() << std::endl;
		}

		//void Trace(std::string log) { pushLog(LogLevel::Trace, log); };
		//void Debug(std::string log) { pushLog(LogLevel::Debug, log); };
		//void Info(std::string log) { pushLog(LogLevel::Info, log); };
		//void Warn(std::string log) { pushLog(LogLevel::Warn, log); };
		//void Error(std::string log) { pushLog(LogLevel::Error, log); };
		//void Fatal(std::string log) { pushLog(LogLevel::Trace, log); };

		//void pushLog(LogLevel level, const std::string& log) {
		//	std::unique_lock lock(logMutex_);

		//	logQueue_.emplace(level, log);
		//}

		void pushLog(LogLevel level, std::string file, std::string func, int line, std::string log) {
			std::unique_lock lock(logMutex_);
			oss_.str("");
			oss_.clear();

			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			std::string time(30, '\0');

			std::strftime(&time[0], time.size(), "%H:%M:%S", &localtime(nowTime));
			auto milliseconds = now - std::chrono::time_point_cast<std::chrono::seconds>(now);

			oss_ << "[" << std::setw(5) << std::setfill(' ') << logLevel[(int)level] << "] [" << func << " at " << file << "::" << line << "] [" << time << "." << std::setw(3) << std::setfill('0') << milliseconds.count() / 10000 << "] : " << log << std::endl;
			logQueue_.emplace(level, oss_.str());
			log_cv.notify_one();
		}

		//void print() {
		//	while (true) {
		//		std::unique_lock lock(logMutex_);
		//		log_cv.wait(lock, [&] {return !logQueue_.empty() || finish_; });

		//		const Log& log = logQueue_.front();
		//		logQueue_.pop();
		//		lock.unlock();

		//		for (const auto& appender : logTypeVector_) {
		//			appender->action(log);
		//		}
		//	}
		//}

		LogLevel GetLogLevel() {
			return logLevel_;
		}

		static inline YLogger* logger = nullptr;
	private:


		std::thread loggingThread_;
		std::queue<Log> logQueue_;
		std::vector<std::unique_ptr<Appender>> logTypeVector_;
		std::filesystem::path savePath_;
		std::mutex logMutex_;
		std::condition_variable log_cv;

		bool finish_;
		LogLevel logLevel_;
		std::ostringstream oss_;

		inline std::tm localtime(std::time_t timer)
		{
			std::tm bt{};
#if defined(__unix__)
			localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
			localtime_s(&bt, &timer);
#else
			static std::mutex mtx;
			std::lock_guard<std::mutex> lock(mtx);
			bt = *std::localtime(&timer);
#endif
			return bt;
		}


		void ReadConfig() {
			//파일 읽어서 설정 보고 맞는 appender 객체 생성 및 추가

			logTypeVector_.emplace_back(std::make_unique<ConsoleAppender>());
		}
	};
	inline logger::YLogger* getLogger() { return logger::YLogger::getLogger(); }
#define LOG_TRACE(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL)																  logger->pushLog(logger::LogLevel::Trace,  __FILE__,  __func__, __LINE__, log); }
#define LOG_DEBUG(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->GetLogLevel() <= (int)logger::LogLevel::Debug) logger->pushLog(logger::LogLevel::Debug,  __FILE__,  __func__, __LINE__, log); }
#define LOG_INFO(log)  { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->GetLogLevel() <= (int)logger::LogLevel::Info ) logger->pushLog(logger::LogLevel::Info ,  __FILE__,  __func__, __LINE__, log); }
#define LOG_WARN(log)  { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->GetLogLevel() <= (int)logger::LogLevel::Warn ) logger->pushLog(logger::LogLevel::Warn ,  __FILE__,  __func__, __LINE__, log); }
#define LOG_ERROR(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->GetLogLevel() <= (int)logger::LogLevel::Error) logger->pushLog(logger::LogLevel::Error,  __FILE__,  __func__, __LINE__, log); }
#define LOG_FATAL(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->GetLogLevel() <= (int)logger::LogLevel::Fatal) logger->pushLog(logger::LogLevel::Fatal,  __FILE__,  __func__, __LINE__, log); }


	enum class FileStatus {
		Created,
		Modified,
		Erased
	};

	class FileWatchar {
	public:
		FileWatchar() :running_(true) {}
		void start(std::function<void(std::filesystem::path, FileStatus)> function) {
			while (running_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileWatchingTime));

				for (const auto& it : file_watching_map_) {
					if (std::filesystem::exists(it.first) == false) {
						function(std::filesystem::path(it.first), FileStatus::Erased);
					}
					else if (it.second == std::filesystem::file_time_type::min() && std::filesystem::exists(it.first)) {
						function(std::filesystem::path(it.first), FileStatus::Created);
					}
					else if (it.second != std::filesystem::last_write_time(it.first)) {
						function(std::filesystem::path(it.first), FileStatus::Modified);
					}
				}

			}
		}
		void addPath(std::filesystem::path path) {
			if (std::filesystem::exists(path)) {
				file_watching_map_[path.string()] = std::filesystem::last_write_time(path);
			}
			else {
				file_watching_map_[path.string()] = std::filesystem::file_time_type::min();
			}
		}
	private:
		std::unordered_map<std::string, std::filesystem::file_time_type> file_watching_map_;
		bool running_;
		bool IsContain(const std::string& key) {
			return file_watching_map_.find(key) != file_watching_map_.end();
		}
	};
}

#endif // !YLOGGER_H