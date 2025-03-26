// Solution `back_end` is a C++ back end for playing Settlers of Catan.

// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "ai/neural_network.hpp"
#include "ai/self_play.hpp"
#include "ai/training.hpp"

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


// TODO: Consider whether `struct` `Config` belongs in another file.
struct Config {
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
};


// TODO: Consider whether function `loadConfig` belongs in another file.
Config loadConfig() {
	Config config;
	std::ifstream file("config.json");
	if (!file.is_open()) {
		throw std::runtime_error("config.json could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	auto configJson = crow::json::load(buffer.str());
	if (!configJson) {
		throw std::runtime_error("Parsing config.json failed.");
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


/* TODO: Align this back end with the Python back end.
* Migrate self play and training logic.
*/
// TODO: Consider whether database driven state management needs to be implemented more.

// Structure CorsMiddleware is a simple middleware to add CORS headers.
struct CorsMiddleware {
	struct context { };

	void before_handle(crow::request&, crow::response&, context&) {
		// Do nothing.
	}

	// After handling, add CORS headers.
	template <typename AllContext>
	void after_handle(const crow::request&, crow::response& res, context&, AllContext&) {
		res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
		res.add_header("Access-Control-Allow-Headers", "Content-Type");
	}
};


// Atomic `stopModelWatcher` is a global flag to stop thread for reloading parameters for neural network on shutdown.
// Atomic `stopTraining` is a global flag to top training neural network on shutdown.
std::atomic<bool> stopModelWatcher{ false };
std::atomic<bool> stopTraining{ false };


// Function `modelWatcher` periodically reloads neural network parameters if file of parameters was updated.
// TODO: Consider whether function `modelWatcher` belongs in another file.
static void modelWatcher(SettlersNeuralNet* neuralNet, int modelWatcherInterval) {
	while (!stopModelWatcher.load()) {
		try {
			neuralNet->reloadIfUpdated();
		}
		catch (const std::exception& e) {
			std::cerr << "[MODEL WATCHER ERROR] " << e.what() << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::seconds(modelWatcherInterval));
	}
}

/* Function `trainingLoop` runs on a background thread and
* continuously runs self play games to collect training examples and
* triggers training when enough examples have been collected.
*/
// TODO: Consider whether function `trainingLoop` belongs in another file.
// TODO: Consider simulating full game trajectories and computing game outcomes.
static void trainingLoop(
	Database* db,
	SettlersNeuralNet* neuralNet,
	int trainingThreshold,
	int numberOfSimulations,
	double cPuct,
	double tolerance
) {
	std::vector<TrainingExample> trainingData;
	while (!stopTraining.load()) {
		try {
			// Run one self play game to generate training examples.
			// TODO: Consider whether running more than one game is important.
			auto trainingExamples = runSelfPlayGame(*neuralNet, *db, numberOfSimulations, cPuct, tolerance);
			trainingData.insert(trainingData.end(), trainingExamples.begin(), trainingExamples.end());
			std::clog << "[TRAINING] Collected " << trainingData.size() << " training examples." << std::endl;
			if (trainingData.size() >= trainingThreshold) {
				std::clog << "[TRAINING] Training neural network with " << trainingData.size() << " examples." << std::endl;
				trainNeuralNetworkIfNeeded(trainingData, neuralNet);
				trainingData.clear();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[TRAINING] The following exception occurred." << e.what() << std::endl;
		}
	}
}


// Function main sets up Crow app, database, neural network, model watcher thread, continuous training thread, and endpoints.
int main() {

	Config config;
	try {
		config = loadConfig();
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Loading configuration failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}


    crow::App<CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);


    Database db(config.dbName, config.dbHost, config.dbPassword, config.dbPort, config.dbUsername);
	try {
		db.initialize();
		std::clog << "[INFO] Database was initialized.\n";
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Database initialization failed with the following error." << e.what() << std::endl;
		return EXIT_FAILURE;
	}


	SettlersNeuralNet neuralNet(config.modelPath);


	/* Start background threads.
	* 1. Model watcher: Reload parameters for neural network if file of parameters has been updated.
	* 2. Continuous training: Run self play games to accumulate training data and update neural network.
	*/
	std::thread modelWatcherThread(modelWatcher, &neuralNet, config.modelWatcherInterval);
	std::thread trainingThread(
		trainingLoop,
		&db,
		&neuralNet,
		config.trainingThreshold,
		config.numberOfSimulations,
		config.cPuct,
		config.tolerance
	);


    CROW_ROUTE(app, "/cities").methods("GET"_method)
    ([&db]() -> crow::json::wvalue {
		try {
			return db.getCitiesJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting cities failed with the following error. ") + e.what();
			return error;
		}
    });


	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db, &neuralNet, &config]() -> crow::json::wvalue {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint next. The game state will be transitioned." << std::endl;
			GameState currentGameState = db.getGameState();
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db, neuralNet, config.numberOfSimulations, config.cPuct, config.tolerance);
			db.updateGameState(currentGameState);
		}
		catch (const std::exception& e) {
			response["error"] = std::string("The following error occurred while transitioning the game state. ") + e.what();
			std::cerr << "[ERROR] " << std::string("The following error occurred while transitioning the game state. ") + e.what() << std::endl;
		}
		return response;
	});


	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db]() -> crow::json::wvalue {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint reset. Game state and database will be reset." << std::endl;
			bool success = db.resetGame();
			response["message"] = success ? "Game has been reset to initial state." : "Resetting game failed.";
		}
		catch (const std::exception& e) {
			response["error"] = std::string("Resetting game failed with the following error. ") + e.what();
			std::cerr << "[ERROR] " << std::string("Resetting game failed with the following error. ") + e.what() << std::endl;
		}
		return response;
	});


	CROW_ROUTE(app, "/roads").methods("GET"_method)
	([&db]() -> crow::json::wvalue {
		try {
			return db.getRoadsJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting roads failed with the following error. ") + e.what();
			return error;
		}
	});


	CROW_ROUTE(app, "/")
	([]() -> crow::json::wvalue {
		crow::json::wvalue result;
		result["message"] = "Welcome to the Settlers of Catan API!";
		return result;
	});


	CROW_ROUTE(app, "/settlements").methods("GET"_method)
	([&db]() -> crow::json::wvalue {
		try {
			return db.getSettlementsJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting settlements failed with the following error. ") + e.what();
			return error;
		}
	});

	
	std::clog << "[INFO] The back end will be started on port " << config.backEndPort << std::endl;
	app.port(config.backEndPort).multithreaded().run();


	// On shutdown, stop model watcher and training threads and join these threads with the main thread.
	stopModelWatcher.store(true);
	stopTraining.store(true);
	if (modelWatcherThread.joinable()) {
		modelWatcherThread.join();
	}
	if (trainingThread.joinable()) {
		trainingThread.join();
	}

    return EXIT_SUCCESS;
}