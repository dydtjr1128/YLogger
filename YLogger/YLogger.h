#ifndef YLOGGER_H
#define YLOGGER_H

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
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

	class YLogger {
	public:
		explicit YLogger(std::filesystem::path savePath = std::filesystem::current_path()) : savePath_(savePath), finish_(false) {
			std::thread(printThread);
		}
		~YLogger() {
			finish_ = true;
			log_cv.notify_all();
		}

		void Trace(std::string log) { pushLog(LogLevel::Trace, log); };
		void Debug(std::string log) { pushLog(LogLevel::Debug, log); };
		void Info(std::string log) { pushLog(LogLevel::Info, log); };
		void Warn(std::string log) { pushLog(LogLevel::Warn, log); };
		void Error(std::string log) { pushLog(LogLevel::Error, log); };
		void Fatal(std::string log) { pushLog(LogLevel::Trace, log); };

		void pushLog(LogLevel level, const std::string& log) {
			std::unique_lock lock(logMutex_);
			logQueue_.emplace(level, log);
		}

		void printThread() {
			while (true) {
				std::unique_lock lock(logMutex_);
				log_cv.wait(lock, [&] {return !logQueue_.empty() || finish_; });

				const Log& log = logQueue_.front();
				logQueue_.pop();
				lock.unlock();

				for (const auto& logger : logTypeVector_) {
					//logger.action()
				}
			}
		}
	private:
		std::queue<Log> logQueue_;
		std::vector<LoggerType> logTypeVector_;
		std::filesystem::path savePath_;
		std::mutex logMutex_;
		std::condition_variable log_cv;

		bool finish_;
	};

	class Appender {
	public:
		virtual void action(const Log& log) = 0;
	private:
	};

	class ConsoleAppender : public Appender {
	public:
		void action(const Log& log) {
			oss_.str("");
			oss_.clear();

			oss_ << "[" << logLevel[(int)log.level()] << "] [" << __func__ << "] : " << log.log() << std::endl;
			std::cout << oss_.str();
		}
	private:
		std::ostringstream oss_;
	};

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