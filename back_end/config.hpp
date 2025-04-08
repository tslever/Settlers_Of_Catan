#pragma once

#include <fstream>
#include <crow/json.h>
#include <sstream>
#include <string>


namespace Config {
	class Config {
	public:
		int backEndPort;
		int batchSize;
		double cPuct;
		double dirichletMixingWeight;
		double dirichletShape;
		std::string dbName;
		std::string dbHost;
		std::string dbPassword;
		unsigned int dbPort;
		std::string dbUsername;
		double learningRate;
		std::string modelPath;
		int modelWatcherInterval;
		int numberOfEpochs;
		int numberOfNeurons;
		int numberOfSimulations;
		double tolerance;
		int trainingThreshold;
		// TODO: Consider configuring simulation depth.

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
			config.batchSize = configJson["batchSize"].i();
			config.cPuct = configJson["cPuct"].d();
			config.dirichletMixingWeight = configJson["dirichletMixingWeight"].d();
			config.dirichletShape = configJson["dirichletShape"].d();
			config.dbName = configJson["dbName"].s();
			config.dbHost = configJson["dbHost"].s();
			config.dbPassword = configJson["dbPassword"].s();
			config.dbPort = configJson["dbPort"].i();
			config.dbUsername = configJson["dbUsername"].s();
			config.learningRate = configJson["learningRate"].d();
			config.modelPath = configJson["modelPath"].s();
			config.modelWatcherInterval = configJson["modelWatcherInterval"].i();
			config.numberOfEpochs = configJson["numberOfEpochs"].i();
			config.numberOfNeurons = configJson["numberOfNeurons"].i();
			config.numberOfSimulations = configJson["numberOfSimulations"].i();
			config.tolerance = configJson["tolerance"].d();
			config.trainingThreshold = configJson["trainingThreshold"].i();
			return config;
		}
	};
}