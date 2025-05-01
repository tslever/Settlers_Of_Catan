#pragma once


#include <chrono>
#include <cmath>
#include <iostream>
#include <string>


namespace Logger {

	std::string currentTime() {
		std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::now();
		auto now = std::chrono::time_point_cast<std::chrono::seconds>(timePoint);
		return std::format("{:%Y-%m-%d %H:%M:%S}", now);
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