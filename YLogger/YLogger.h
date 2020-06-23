#ifndef YLOGGER_H
#define YLOGGER_H

#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>
#include <unordered_map>

namespace logger {

	namespace {
		constexpr size_t kDefaultFileWatchingTime = 1000;
	}

	enum class LoggerType {
		ConsoleAppender,
		FileAppender,
		RollingFileAppender
	};


	class YLogger {
	public:
		explicit YLogger(std::filesystem::path path = std::filesystem::current_path()) : path_(path) {}
		~YLogger() {}
	private:
		std::filesystem::path path_;
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