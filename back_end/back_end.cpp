// Solution `back_end` is a C++ back end for playing Settlers of Catan.

// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "ai/mcts/backpropagation.hpp"
#include "ai/mcts/expansion.hpp"
#include "ai/mcts/node.hpp"
#include "ai/mcts/selection.hpp"
#include "ai/mcts/simulation.hpp"
#include "ai/neural_network.hpp"
#include "ai/self_play.hpp"
#include "ai/training.hpp"

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


/* TODO: Align this back end with the Python back end.
* Migrate MCTS, continuous self play, training, and neural network evaluation from the Python back end.
*/

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


// Atomic `stopModelWatcher` is a global flag to signal the model reloader thread to stop.
std::atomic<bool> stopModelWatcher{ false };

// Atomic `stopTraining` is a global flag to signal model training to stop.
std::atomic<bool> stopTraining{ false };

/* Function `modelWatcher` is a background thread function
* to periodically check and reload updated model weights.
*/
// TODO: Consider whether function `modelWatcher` belongs in another file.
static void modelWatcher(SettlersNeuralNet* neuralNet) {
	while (!stopModelWatcher.load()) {
		neuralNet->reloadIfUpdated();
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

/* Function `trainingLoop` is a background thread function
* to continuously run self play and trigger training updates.
*/
// TODO: Consider whether function `trainingLoop` belongs in another file.
static void trainingLoop(Database* db, SettlersNeuralNet* neuralNet) {
	while (!stopTraining.load()) {
		// Run one self play game to generate training examples.
		// TODO: Consider whether running more than one game is important.
		auto trainingExamples = runSelfPlayGame(*neuralNet, *db);
		// TODO: Implement `runSelfPlayGame`.
		// If enough new examples have been generated, trigger a training update.
		trainNeuralNetworkIfNeeded(trainingExamples, neuralNet);
		// TODO: Implement `trainNeuralNetworkIfNeeded`.
		// Wait a short interval before starting the next self play game.
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

// Function `runMcts` runs MCTS by creating a root node from the current game state, running a number of simulations, and returning the best move.
// TODO: Consider whether function `runMcts` belongs in another file.
std::pair<std::string, double> runMcts(GameState& currentState, Database& db, SettlersNeuralNet& neuralNet) {
	// Create a root node based on the current game state.
	auto root = std::make_shared<MCTSNode>(currentState, "", nullptr, "settlement");
	const int numberOfSimulations = 50; // Use a value in a configuration file.

	// Initially expand the root node using the new expansion function.
	expandNode(root, db, neuralNet);

	// If no children were generated, try a fallback strategy.
	/* TODO: Determine whether it should never be the case that no children are generated
	* when running MCTS to place the first settlement.
	* If so, ensure that this case never occurs and delete this block of code.
	*/
	if (root->children.empty()) {
		std::vector<std::string> occupied = getOccupiedVertices(db);
		auto available = getAvailableVertices(occupied);
		if (!available.empty()) {
			// Choose a random available move as fallback.
			int randomIndex = std::rand() % available.size();
			return { available[randomIndex], 0.0 };
		}
		else {
			// No moves are available at all.
			return { "", 0.0 };
		}
	}

	// Run MCTS simulations.
	for (int i = 0; i < numberOfSimulations; i++) {
		auto node = root;
		// Selection: Descend / traverse down the tree until a leaf node is reached.
		while (!node->isLeaf()) {
			node = selectChild(node);
		}
		// Expansion: If the node was visited before, expand the node.
		if (node->N > 0) {
			expandNode(node, db, neuralNet);
		}
		// Simulation: Evaluate the leaf node via a rollout.
		double value = simulateRollout(node, db, neuralNet);
		// Backpropagation: Update statistics up the tree.
		backpropagate(node, value);
	}

	// Select the child move with the highest visit count.
	// TODO: Consider whether selecting moves based on visit count and/or other properties might be better.
	std::shared_ptr<MCTSNode> bestChild = nullptr;
	int bestVisits = -1;
	for (auto& pair : root->children) {
		auto child = pair.second;
		if (child->N > bestVisits) {
			bestVisits = child->N;
			bestChild = child;
		}
	}
	if (bestChild) {
		return { bestChild->move, bestChild->N };
	}
	return { "", 0.0 };
}


/* Function main sets up
* the Crow app, database, neural network, routes, model watcher thread, and continuous training thread.
*/
int main() {

	// Create Crow app with CORS middleware.
    crow::App<CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);

    // Configure database.
	std::string dbName = "game";
    std::string host = "localhost";
	std::string password = "settlers_of_catan";
	unsigned int port = 33060;
    std::string username = "administrator";
    Database db(dbName, host, password, port, username);

	// Initialize the database schema.
	try {
		db.initialize();
		std::clog << "[INFO] Database was initialized successfully." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Database initialization failed with the following error." << e.what() << std::endl;
		return 1;
	}

	// Create a neural network using libtorch.
	SettlersNeuralNet neuralNet("ai/neural_network.pt");

	// Start a thread to watch for updated model weights.
	std::thread modelWatcherThread(modelWatcher, &neuralNet);

	// Start a thread to train.
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
			
			// Retrieve current game state from database.
			GameState currentGameState = db.getGameState();
			
			// Capture the original phase before any transition.
			std::string originalPhase = currentGameState.phase;

			// TODO: Consider moving into class `PlaceFirstSettlementState`.
			if (originalPhase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
				auto mctsResult = runMcts(currentGameState, db, neuralNet);
				if (mctsResult.first.empty()) {
					throw std::runtime_error("MCTS failed to determine a settlement move.");
				}
				// Apply the chosen settlement move.
				currentGameState.placeSettlement(currentGameState.currentPlayer, mctsResult.first);
			}
			// TODO: Consider running Monte Carlo Tree Search to
			// place first settlement, placing first road, place first city, and placing second road.

			// Create phase state machine to transition state.
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db);

			// Update game state in database.
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

	/* On shutdown, signal the model watcher thread and the training thread to stop and
	* join these threads with the main thread.
	*/
	stopModelWatcher.store(true);
	stopTraining.store(true);
	if (modelWatcherThread.joinable()) {
		modelWatcherThread.join();
	}
	if (trainingThread.joinable()) {
		trainingThread.join();
	}

    return 0;
}