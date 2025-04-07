// Solution `back_end` is a C++ back end for playing Settlers of Catan.

// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "config.hpp"
#include "server.hpp"
#include "ai/trainer.hpp"

// TODO: Consider whether database driven state management needs to be implemented more.
// TODO: Consider using a graph database.

int main() {

	Config::Config config;
	try {
		config = config.load("config.json");
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Loading configuration failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}


    crow::App<Server::CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);


    DB::Database liveDb(config.dbName, config.dbHost, config.dbPassword, config.dbPort, config.dbUsername, "live_");
	try {
		liveDb.initialize();
		std::clog << "[INFO] Database was initialized.\n";
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Initializing database failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	AI::WrapperOfNeuralNetwork neuralNet(config.modelPath);

	AI::Trainer trainer(
		&neuralNet,
		config.modelWatcherInterval,
		config.trainingThreshold,
		config.numberOfSimulations,
		config.cPuct,
		config.tolerance
	);
	trainer.startModelWatcher();
	trainer.runTrainingLoop();

	Server::setUpRoutes(app, liveDb, neuralNet, config);

	std::clog << "[INFO] The back end will be started on port " << config.backEndPort << std::endl;
	app.port(config.backEndPort).multithreaded().run();

    return EXIT_SUCCESS;
}