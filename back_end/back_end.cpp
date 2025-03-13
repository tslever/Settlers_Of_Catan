// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


/* TODO: Align this back end with the Python back end.
* Migrate continuous training, model reloading, MCTS, and neural network evaluation.
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

    // Get a JSON list of cities.
    CROW_ROUTE(app, "/cities").methods("GET"_method)
    ([&db]() {
		return db.getCitiesJson();
    });

	// Transition game state.
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db]() {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint next. The game state will be transitioned." << std::endl;
			GameState currentGameState = db.getGameState();
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db);
			db.updateGameState(currentGameState);
			return response;
		}
		catch (const std::exception& e) {
			response["error"] = std::string("The following error occurred while transitioning the game state.") + e.what();
			std::cerr << "[ERROR] " << std::string("The following error occurred while transitioning the game state.") + e.what() << std::endl;
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
			GameState defaultGameState = GameState();
			defaultGameState.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
			defaultGameState.currentPlayer = 1;
			defaultGameState.lastBuilding = "";
			db.updateGameState(defaultGameState);
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

	std::clog << "[INFO] The back end will be started on port 5000." << std::endl;
    app.port(5000).multithreaded().run();
    return 0;
}