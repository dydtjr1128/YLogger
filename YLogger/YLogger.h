#ifndef YLOGGER_H
#define YLOGGER_H

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <ctime>   // localtime
#include <filesystem>
#include <functional>
#include <fstream>
#include <iomanip> // put_time
#include <iostream>
#include <locale>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace logger {


	namespace {
		const std::string logLevelString[] = { "Trace", "Debug", "Info", "Warn", "Error", "Fatal" };
		const std::string kDefaultLogSavePathString = "./Log";
		const std::string kDefaultlogFileNameHeader = "YLogger";
		constexpr size_t kDefaultFileWatchingTime = 1000;

		constexpr const char* file_name(const char* path) {
			const char* file = path;
			while (*path) {
				if (*path++ == '\\') {
					file = path;
				}
			}
			return file;
		}

		inline std::tm localtime(std::time_t timer) {
			std::tm bt{};
#if defined(__unix__)
			localtime_r(&timer, &bt);
#elif defined(_MSC_VER) && defined(_WIN32)
			localtime_s(&bt, &timer);
#else
			static std::mutex mtx;
			std::lock_guard<std::mutex> lock(mtx);
			bt = *std::localtime(&timer);
#endif
			return bt;
		}
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

	class Log {
	public:
		explicit Log(logger::LogLevel level, const std::string& log) :level_(level), log_(log) {};
		logger::LogLevel level() const { return level_; }
		std::string log() const { return log_; }

		Log(Log&& rhs) noexcept { // 이동 생성자, 얕은 복사 개념
			auto start_time = std::chrono::system_clock::now();
			this->level_ = rhs.level_;
			this->log_ = rhs.log_;
		}
	private:
		logger::LogLevel level_;
		std::string log_;
	};

	class Appender {
	public:
		virtual void action(const Log& log) = 0;
		virtual void actions(const std::vector<Log>& logVector) = 0;
	private:
	};

	class ConsoleAppender : public Appender {
	public:
		void action(const Log& log) {
			std::cout << log.log();
		}
		void actions(const std::vector<Log>& logVector) {
			for (auto& log : logVector) {
				std::cout << log.log();
			}
		}
	private:
	};

	class FileAppender : public Appender {
	public:
		explicit FileAppender(const std::string& logSavePathString = kDefaultLogSavePathString) : logFileNameHeader_(kDefaultlogFileNameHeader), filePath_(logSavePathString) {
			if (std::filesystem::exists(filePath_) == false) {
				std::filesystem::create_directories(filePath_);
			}
		};

		void action(const Log& log) {
			logName_ = logFileNameHeader_ + GetTodayTimeFormatString() + ".log";
			WriteLog(log, filePath_ / logName_);
		}

		void actions(const std::vector<Log>& logVector) {
			logName_ = logFileNameHeader_ + GetTodayTimeFormatString() + ".log";
			WriteLogs(logVector, filePath_ / logName_);
		}

	private:
		std::string logFileNameHeader_;
		std::filesystem::path filePath_;
		std::filesystem::path logName_;

		std::string GetTodayTimeFormatString() {
			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			std::string time(15, '\0');

			auto lt = localtime(nowTime);
			std::strftime(&time[0], time.size(), ".%Y-%m-%d", &lt);
			return time.substr(0, 11);
		}

		void WriteLog(const Log& log, const std::filesystem::path& path) {
			std::ofstream writeFile;

			writeFile.open(path.string(), std::ios_base::app);

			if (writeFile.is_open()) {
				const auto& logLine = log.log();
				writeFile.write(logLine.c_str(), logLine.size());
			}
			writeFile.close();
		}
		void WriteLogs(const std::vector<Log>& logVector, const std::filesystem::path& path) {
			std::ofstream writeFile;

			writeFile.open(path.string(), std::ios_base::app);
			if (writeFile.is_open()) {
				for (auto& log : logVector) {
					const auto& logLine = log.log();
					writeFile.write(logLine.c_str(), logLine.size());
				}
			}
			writeFile.close();
		}

	};

	class YLogger {
	public:
		static YLogger* GetLogger() {
			if (logger == nullptr) {
				logger = std::make_unique<YLogger>();
			}
			return logger.get();
		}

		static void Initialize(logger::LogLevel loggerLevel = logger::LogLevel::Debug) {
			if (logger == nullptr) {
				logger = std::make_unique<YLogger>(loggerLevel);
			}
		}

		explicit YLogger(logger::LogLevel loggerLevel = logger::LogLevel::Debug) : finish_(false), loggerLevel_(loggerLevel) {
			loggingThread_ = std::thread([&] {
				while (true) { // save multi log at one loop
					std::vector<logger::Log> logVector;
					std::unique_lock lock(logMutex_);
					log_cv.wait(lock, [&] {return !logQueue_.empty() || finish_; });

					if (logQueue_.empty() && finish_) {
						break;
					}

					while (!logQueue_.empty()) {
						Log& log = logQueue_.front();
						logVector.push_back(std::move(log)); // push_back(T&& value); 이동 생성자 사용

						logQueue_.pop();
					}
					lock.unlock();

					for (const auto& appender : logTypeVector_) {
						appender->actions(logVector);
					}
				}
				});
		}

		// YLogger instance is unique_ptr. So it will be deleted when the function that first called initialize function (such as the main function) is finish.
		// like -> logger::YLogger::GetLogger()->AddLogger(logger::LoggerType::ConsoleAppender);
		~YLogger() {
			finish_ = true;
			log_cv.notify_all();
			loggingThread_.join();
		}

		inline void pushLog(logger::LogLevel level, std::string file, std::string func, int line, std::string log) {
			std::unique_lock lock(logMutex_);
			oss_.str("");
			oss_.clear();

			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			std::string time(30, '\0');

			auto lt = localtime(nowTime);
			std::strftime(&time[0], time.size(), "%H:%M:%S", &lt);
			auto milliseconds = now - std::chrono::time_point_cast<std::chrono::seconds>(now); // calc millisecond

			oss_ << "[" << std::setw(5) << std::setfill(' ') << logLevelString[(int)level] << "] [" << func << " at " << file << "::" << line << "] [" << time.substr(0, 8) << "." << std::setw(3) << std::setfill('0') << milliseconds.count() / 10000 << "] : " << log << std::endl;
			logQueue_.emplace(level, oss_.str());
			log_cv.notify_one();
		}

		void AddLogger(const LoggerType type, const std::string& logSavePathString = kDefaultLogSavePathString) {
			switch (type) {
			case logger::LoggerType::ConsoleAppender:
				logTypeVector_.emplace_back(std::make_unique<ConsoleAppender>());
				break;
			case logger::LoggerType::FileAppender:
			case logger::LoggerType::RollingFileAppender:
				logTypeVector_.emplace_back(std::make_unique<FileAppender>(logSavePathString));
				break;
			default:
				break;
			}
		}

		logger::LogLevel loggerLevel() const {
			return loggerLevel_;
		}

		static inline std::unique_ptr<YLogger> logger = nullptr;
	private:
		std::thread loggingThread_;
		std::queue<Log> logQueue_;
		std::vector<std::unique_ptr<Appender>> logTypeVector_;
		std::mutex logMutex_;
		std::condition_variable log_cv;

		bool finish_;
		logger::LogLevel loggerLevel_;
		std::ostringstream oss_;

		void ReadConfig() {
			//파일 읽어서 설정 보고 맞는 appender 객체 생성 및 추가
			logTypeVector_.emplace_back(std::make_unique<ConsoleAppender>());
			logTypeVector_.emplace_back(std::make_unique<FileAppender>());
		}
	};
	inline logger::YLogger* getLogger() { return logger::YLogger::GetLogger(); }
#define __FILE_NAME__ logger::file_name(__FILE__)
#define LOG_TRACE(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL)															   logger->pushLog(logger::LogLevel::Trace,  __FILE_NAME__,  __func__, __LINE__, log); }
#define LOG_DEBUG(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->loggerLevel() <= (int)logger::LogLevel::Debug) logger->pushLog(logger::LogLevel::Debug,  __FILE_NAME__,  __func__, __LINE__, log); }
#define LOG_INFO(log)  { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->loggerLevel() <= (int)logger::LogLevel::Info ) logger->pushLog(logger::LogLevel::Info ,  __FILE_NAME__,  __func__, __LINE__, log); }
#define LOG_WARN(log)  { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->loggerLevel() <= (int)logger::LogLevel::Warn ) logger->pushLog(logger::LogLevel::Warn ,  __FILE_NAME__,  __func__, __LINE__, log); }
#define LOG_ERROR(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->loggerLevel() <= (int)logger::LogLevel::Error) logger->pushLog(logger::LogLevel::Error,  __FILE_NAME__,  __func__, __LINE__, log); }
#define LOG_FATAL(log) { logger::YLogger* logger = logger::getLogger(); if (logger != NULL && (int)logger->loggerLevel() <= (int)logger::LogLevel::Fatal) logger->pushLog(logger::LogLevel::Fatal,  __FILE_NAME__,  __func__, __LINE__, log); }


	namespace watcher {
		namespace {
			constexpr int kDefaultFileWatchingTime = 500;
		}
		enum class FileStatus {
			Created,
			Modified,
			Erased,
		};

		class FileWatchar {
		public:
			FileWatchar() :running_(true) {}
			void start(std::function<void(std::filesystem::path, FileStatus)> function) {
				while (running_) {
					std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultFileWatchingTime));

					for (const auto& it : file_watching_map_) {
						bool exist = std::filesystem::exists(it.first);
						if (exist) {
							if (it.second == std::filesystem::file_time_type::min()) {
								function(std::filesystem::path(it.first), FileStatus::Created);
							}
							else if (it.second != std::filesystem::last_write_time(it.first)) {
								function(std::filesystem::path(it.first), FileStatus::Modified);
							}
							file_watching_map_[it.first] = std::filesystem::last_write_time(it.first);
						}
						else if (it.second != std::filesystem::file_time_type::min()) {
							file_watching_map_[it.first] = std::filesystem::file_time_type::min();
							function(std::filesystem::path(it.first), FileStatus::Erased);
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

		};
	}
}

#endif // !YLOGGER_H