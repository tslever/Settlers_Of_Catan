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
* Migrate MCTS, continuous self play, training, neural network evaluation, and logging from the Python back end.
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


// Atomic `stopModelWatcher` is a global flag to stop thread for reloading parameters for neural network.
// Atomic `stopTraining` is a global flag to top training neural network.
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
* continuously runs self play to accumulate training examples and
* triggers training when a threshold is reached.
*/
// TODO: Consider whether function `trainingLoop` belongs in another file.
static void trainingLoop(Database* db, SettlersNeuralNet* neuralNet) {
	std::vector<TrainingExample> trainingData;
	while (!stopTraining.load()) {
		// Run one self play game to generate training examples.
		// TODO: Consider whether running more than one game is important.
		auto trainingExamples = runSelfPlayGame(*neuralNet, *db);
		trainingData.insert(trainingData.end(), trainingExamples.begin(), trainingExamples.end());
		// If we have reached the training threshold, perform training and clear the accumulated examples.
		if (trainingData.size() >= 500) { // TODO: Use value from configuration file.
			trainNeuralNetworkIfNeeded(trainingData, neuralNet);
			trainingData.clear();
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

// TODO: Consider whether function `injectDirichletNoise` belongs in another file.
void injectDirichletNoise(std::shared_ptr<MCTSNode>& root, double epsilon = 0.25, double alpha = 0.03) {
	if (root->children.empty()) {
		return;
	}
	// Prepare noise: Sample gamma variables and normalize them.
	std::vector<double> noise;
	std::default_random_engine generator(std::random_device{}());
	std::gamma_distribution<double> gammaDist(alpha, 1.0);
	double sum = 0.0;
	for (size_t i = 0; i < root->children.size(); i++) {
		double n = gammaDist(generator);
		noise.push_back(n);
		sum += n;
	}
	for (auto& n : noise) {
		n /= sum;
	}
	size_t index = 0;
	for (auto& childPair : root->children) {
		auto child = childPair.second;
		// Adjust the prior using a weighted mix of the original prior and the injected noise.
		child->P = (1 - epsilon) * child->P + epsilon * noise[index++];
	}
}

/* Function `runMcts` runs MCTS by creating a root node from the current game state,
* running a number of simulations, and returning the best move.
*/
// TODO: Consider whether function `runMcts` belongs in another file.
std::pair<std::string, int> runMcts(
	GameState& currentState,
	Database& db,
	SettlersNeuralNet& neuralNet,
	int numberOfSimulations = 50, // TODO: Use a value in a configuration file.
	double c_puct = 1.0, // TODO: Use a value in a configuration file.
	double tolerance = 1e-6 // TODO: Use a value in a configuration file.
) {
	// Create root node for the current phase.
	// TODO: Avoid assuming settlement phase.
	auto root = std::make_shared<MCTSNode>(currentState, "", nullptr, "settlement");

	// Initially expand the root node based on available moves.
	expandNode(root, db, neuralNet);

	// Inject Dirichlet noise at the root to encourage exploration.
	injectDirichletNoise(root);

	// Run MCTS simulations.
	for (int i = 0; i < numberOfSimulations; i++) {
		auto node = root;
		// Selection: Descend / traverse down the tree until a leaf node is reached.
		while (!node->isLeaf()) {
			node = selectChild(node, c_puct, tolerance);
		}
		// Expansion: If this leaf node was visited before, expand the node.
		if (node->N > 0) {
			expandNode(node, db, neuralNet);
		}
		// Simulation: Evaluate the leaf node via a rollout.
		// TODO: Consider extending to multi-step rollouts.
		double value = simulateRollout(node, db, neuralNet);
		// Backpropagation: Update statistics up the tree / all nodes along the path.
		backpropagate(node, value);
	}

	// Select the child move with the highest visit count.
	// TODO: Consider whether selecting moves based on visit count and/or other criteria might be better.
	std::shared_ptr<MCTSNode> bestChild = nullptr;
	int bestVisits = -1;
	for (const auto& pair : root->children) {
		auto child = pair.second;
		if (child->N > bestVisits) {
			bestVisits = child->N;
			bestChild = child;
		}
	}
	if (bestChild) {
		return { bestChild->move, bestChild->N };
	}
	return { "", 0 };
}


/* Function main sets up the Crow app, database, neural network, routes, model watcher thread, and
* continuous training thread.
*/
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
		std::clog << "[INFO] Database was initialized successfully." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Database initialization failed with the following error." << e.what() << std::endl;
		return 1;
	}

	SettlersNeuralNet neuralNet("ai/neural_network.pt");

	// Start a thread to watch for updated parameters and reload parameters for neural network.
	// Start a thread to train neural network.
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
			std::string originalPhase = currentGameState.phase;

			// TODO: Consider moving into class `PlaceFirstSettlementState`.
			if (originalPhase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
				auto mctsResult = runMcts(currentGameState, db, neuralNet);
				if (mctsResult.first.empty()) {
					throw std::runtime_error("MCTS failed to determine a settlement move.");
				}
				currentGameState.placeSettlement(currentGameState.currentPlayer, mctsResult.first);
			}
			// TODO: Consider running Monte Carlo Tree Search to place first road, place first city, and place second road.

			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db);
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

    return 0;
}