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

/* Function `modelWatcher` runs on a background thread and periodically checks
* whether file of parameters for neural network is updated and reloads parameters.
*/
// TODO: Consider whether function `modelWatcher` belongs in another file.
static void modelWatcher(SettlersNeuralNet* neuralNet) {
	while (!stopModelWatcher.load()) {
		neuralNet->reloadIfUpdated();
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

/* Function `trainingLoop` runs on a background thread and
* continuously generates self play training examples and
* triggers training when enough examples are collected.
*/
// TODO: Consider whether function `trainingLoop` belongs in another file.
static void trainingLoop(Database* db, SettlersNeuralNet* neuralNet) {
	std::vector<TrainingExample> trainingData;
	const size_t trainingThreshold = 20; // TODO: Use value from configuration file.
	while (!stopTraining.load()) {
		try {
			// Run one self play game to generate training examples.
			// TODO: Consider whether running more than one game is important.
			auto trainingExamples = runSelfPlayGame(*neuralNet, *db);
			trainingData.insert(trainingData.end(), trainingExamples.begin(), trainingExamples.end());
			std::clog << "[TRAINING] Collected " << trainingData.size() << " training examples." << std::endl;
			if (trainingData.size() >= trainingThreshold) {
				std::clog << "[TRAINING] Triggered training with " << trainingData.size() << " examples." << std::endl;
				trainNeuralNetworkIfNeeded(trainingData, neuralNet);
				trainingData.clear();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[TRAINING] The following exception occurred." << e.what() << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}


// Function main sets up Crow app, database, neural network, model watcher thread, continuous training thread, and endpoints.
int main() {

	// Create Crow app with CORS middleware.
    crow::App<CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);

    // Configure database.
	// TODO: Use values from configuration file.
	std::string dbName = "game";
    std::string host = "localhost";
	std::string password = "settlers_of_catan";
	unsigned int port = 33060;
    std::string username = "administrator";
    Database db(dbName, host, password, port, username);
	try {
		db.initialize();
		std::clog << "[INFO] Database was initialized." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Database initialization failed with the following error." << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	SettlersNeuralNet neuralNet("ai/neural_network.pt");

	/* Start background threads.
	* 1. Model watcher: Reload parameters for neural network if file of parameters has been updated.
	* 2. Continuous training: Run self play games to accumulate training data and update neural network.
	*/
	std::thread modelWatcherThread(modelWatcher, &neuralNet);
	std::thread trainingThread(trainingLoop, &db, &neuralNet);

    // Endpoint `/cities` allows getting a JSON list of cities.
    CROW_ROUTE(app, "/cities").methods("GET"_method)
    ([&db]() {
		return db.getCitiesJson();
    });

	// Endpoint `/next` allows transitioning game state.
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db, &neuralNet]() {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint next. The game state will be transitioned." << std::endl;
			GameState currentGameState = db.getGameState();
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db, neuralNet);
			db.updateGameState(currentGameState);
			return response;
		}
		catch (const std::exception& e) {
			response["error"] = std::string("The following error occurred while transitioning the game state. ") + e.what();
			std::cerr << "[ERROR] " << std::string("The following error occurred while transitioning the game state. ") + e.what() << std::endl;
			return response;
		}
	});

	// Reset game state and database.
	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db]() {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint reset. Game state and database will be reset." << std::endl;
			bool success = db.resetGame();
			response["message"] = success ? "Game has been reset to initial state." : "Resetting game failed.";
		}
		catch (const std::exception& e) {
			response["error"] = std::string("Resetting game failed with the following error.") + e.what();
			std::cerr << "[ERROR] " << std::string("Resetting game failed with the following error.") + e.what() << std::endl;
		}
		return response;
	});

	// Get a JSON list of roads.
	CROW_ROUTE(app, "/roads").methods("GET"_method)
	([&db]() {
		return db.getRoadsJson();
	});

	// Get a welcome message.
	CROW_ROUTE(app, "/")
	([]() {
		crow::json::wvalue result;
		result["message"] = "Welcome to the Settlers of Catan API!";
		return result;
	});

	// GET a JSON list of settlements.
	CROW_ROUTE(app, "/settlements").methods("GET"_method)
	([&db]() {
		return db.getSettlementsJson();
	});

	// Run the Crow app in its own thread.
	std::clog << "[INFO] The back end will be started on port 5000." << std::endl;
	std::thread appThread([&app]() {
		app.port(5000).multithreaded().run();
	});

	// On shutdown, join the Crow app thread with the main thread.
	appThread.join();

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