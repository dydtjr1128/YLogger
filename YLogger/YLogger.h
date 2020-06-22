#ifndef YLOGGER_H
#define YLOGGER_H

#include <filesystem>
namespace logger {
	class YLogger {
	public:
		explicit YLogger(std::filesystem::path path = std::filesystem::current_path());
		~YLogger();
	private:
		std::filesystem::path path_;
	};
}

#endif // !YLOGGER_H