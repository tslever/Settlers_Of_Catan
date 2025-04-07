#pragma once

#include <fstream>
#include <crow/json.h>
#include <sstream>
#include <string>


namespace Config {
	class Config {
	public:
		int backEndPort;
		double cPuct;
		std::string dbName;
		std::string dbHost;
		std::string dbPassword;
		unsigned int dbPort;
		std::string dbUsername;
		std::string modelPath;
		int modelWatcherInterval;
		int numberOfSimulations;
		double tolerance;
		int trainingThreshold;
		// TODO: Consider configuring simulation depth.
		// TODO: Consider configuring number of training epochs.

		static Config load(const std::string& path) {
			Config config;
			std::ifstream file(path);
			if (!file.is_open()) {
				throw std::runtime_error("Configuration file could not be opened.");
			}
			std::stringstream buffer;
			buffer << file.rdbuf();
			auto configJson = crow::json::load(buffer.str());
			if (!configJson) {
				throw std::runtime_error("Parsing configuration file failed.");
			}
			config.backEndPort = configJson["backEndPort"].i();
			config.cPuct = configJson["cPuct"].d();
			config.dbName = configJson["dbName"].s();
			config.dbHost = configJson["dbHost"].s();
			config.dbPassword = configJson["dbPassword"].s();
			config.dbPort = configJson["dbPort"].i();
			config.dbUsername = configJson["dbUsername"].s();
			config.modelPath = configJson["modelPath"].s();
			config.modelWatcherInterval = configJson["modelWatcherInterval"].i();
			config.numberOfSimulations = configJson["numberOfSimulations"].i();
			config.tolerance = configJson["tolerance"].d();
			config.trainingThreshold = configJson["trainingThreshold"].i();
			return config;
		}
	};
}