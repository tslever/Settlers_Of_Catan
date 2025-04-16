#include <chrono>
#include <iostream>
#include <string>


namespace Logger {

	std::string currentTime() {
		std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::now();
		time_t time = std::chrono::system_clock::to_time_t(timePoint);
		const int numberOfCharactersInBufferPerCTimeS = 26;
		char buffer[numberOfCharactersInBufferPerCTimeS];
		errno_t errorNumber = ctime_s(buffer, sizeof(buffer), &time);
		if (errorNumber != 0) {
			throw std::runtime_error("ctime_s failed with error number " + std::to_string(errorNumber) + ".");
		}
		std::string timeString(buffer);
		if (!timeString.empty() && timeString.back() == '\n') {
			timeString.pop_back();
		}
		return timeString;
	}

	void error(const std::string& context, const std::exception& e) {
		std::cerr << "[ERROR][" << currentTime() << "][" << context << "] " << e.what() << std::endl;
	}

	void error(const std::string& context, const std::string& message) {
		std::cerr << "[ERROR][" << currentTime() << "][" << context << "] " << message << std::endl;
	}

	void warn(const std::string& context, const std::string& message) {
		std::cerr << "[WARNING][" << currentTime() << "][" << context << "] " << message << std::endl;
	}

	void info(const std::string& message) {
		std::clog << "[INFO][" << currentTime() << "]" << message << std::endl;
	}

}