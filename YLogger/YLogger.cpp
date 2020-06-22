#include "YLogger.h"
#include <filesystem>

namespace logger {

	YLogger::YLogger(std::filesystem::path path) : path_(path){
	}

	YLogger::~YLogger() {
	}
}
