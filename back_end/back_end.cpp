// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


// TODO: Align this back end with the Python back end.


struct CorsMiddleware {
	struct context { };

	void before_handle(crow::request&, crow::response&, context&) {
		// Do nothing.
	}

	template <typename AllContext>
	void after_handle(const crow::request&, crow::response& res, context&, AllContext&) {
		res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
		res.add_header("Access-Control-Allow-Headers", "Content-Type");
	}
};


int main() {

    crow::App<CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);

    // Configure database connection.
	std::string dbName = "game";
    std::string host = "localhost"; // Host address
	std::string password = "settlers_of_catan";
	unsigned int port = 33060; // Default port for MySQL X DevAPI
    std::string username = "administrator";
    Database db(dbName, host, password, port, username);

	// Initialize the database schema (ensure tables exist).
	try {
		db.initialize();
	}
	catch (const std::exception& e) {
		std::cerr << "Database initialized failed with the following error." << e.what() << std::endl;
		return 1;
	}

	static GameState globalGameState = db.getGameState();

    // GET /cities - return a JSON list of cities.
    CROW_ROUTE(app, "/cities").methods("GET"_method)
    ([&db]() {
		return db.getCitiesJson();
    });

	// POST /next - transition the game state using phase state machine.
	/* TODO: Resolve the error where querying next results in
	* a random settlement of Player 1's color being displayed properly but no message,
	* a random settlement being displayed instead of an edge of Player 1's color and no message,
	* a random settlement being displayed of Player 1's color instead of Player 2's color and no message,
	* a random settlement of Player 1's color being displayed instead of an edge of Player 2's color and no message,
	* a random settlement being displayed of Player 1's color instead of Player 3's color and no message,
	* a random settlement being displayed of Player 1's color instead of an edge of Player 3's color and no message,
	* et cetera.
	*/
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db]() {
		crow::json::wvalue response;
		try {
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(globalGameState, db);
			db.updateGameState(globalGameState);
			return response;
		}
		catch (const std::exception& e) {
			response["error"] = std::string("The following error occurred while transitioning the game state.") + e.what();
			return response;
		}
	});

	// POST /reset - reset both database and game state.
	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db]() {
		crow::json::wvalue result;
		try {
			bool success = db.resetGame();
			globalGameState = GameState();
			globalGameState.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
			globalGameState.currentPlayer = 1;
			globalGameState.lastBuilding = "";
			db.updateGameState(globalGameState);
			result["message"] = success
				? "Game has been reset to the initial state."
				: "Resetting game failed.";
		}
		catch (const std::exception& e) {
			result["error"] = std::string("Resetting game failed with the following error.") + e.what();
		}
		return result;
	});

	// GET /roads - return a JSON list of roads.
	CROW_ROUTE(app, "/roads").methods("GET"_method)
	([&db]() {
		return db.getRoadsJson();
	});

	// GET / - simple route endpoint
	CROW_ROUTE(app, "/")
	([]() {
		crow::json::wvalue result;
		result["message"] = "Welcome to the Settlers of Catan API!";
		return result;
	});

	// GET /settlements - return a JSON list of settlements.
	CROW_ROUTE(app, "/settlements").methods("GET"_method)
	([&db]() {
		return db.getSettlementsJson();
	});

    app.port(5000).multithreaded().run();
    return 0;
}