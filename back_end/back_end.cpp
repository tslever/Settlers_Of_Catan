// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


int main() {

    crow::SimpleApp app;

    // Configure your database connection.
	std::string dbName = "game";
    std::string host = "tcp://127.0.0.1:3306";
	std::string password = "settlers_of_catan";
    std::string username = "administrator";
    Database db(dbName, host, password, username);

	// Initialize the database schema (ensure tables exist).
	try {
		db.initialize();
	}
	catch (const std::exception& e) {
		std::cerr << "Database initialized failed with the following error." << e.what() << std::endl;
		return 1;
	}

	// In-memory game state instance.
	// TODO: Save game state in database.
	GameState gameState;
	gameState.phase = "phase to place first settlement";
	gameState.currentPlayer = 1;
	gameState.lastBuilding = "";

    // GET /cities - return a JSON list of cities.
	// TODO: Resolve the error that occurs after a city is placed and this endpoint is queried where the response is `{"cities":null}`.
    CROW_ROUTE(app, "/cities")
    ([&db]() {
		crow::json::wvalue result;
		try {
			auto cities = db.getCities();
			crow::json::wvalue citiesJson;
			for (size_t i = 0; i < cities.size(); ++i) {
				citiesJson[i]["id"] = cities[i].id;
				citiesJson[i]["player"] = cities[i].player;
				citiesJson[i]["vertex"] = cities[i].vertex;
			}
			result["cities"] = std::move(citiesJson);
		}
		catch (const std::exception& e) {
			result["error"] = std::string("The following error occurred while fetching cities.") + e.what();
		}
        return result;
    });

	// POST /next - transition the game state using phase state machine.
	// TODO: Resolve the error where querying next repeatedly doesn't result in the progression of players and phases in the Python back end.
	// Player 1 places their first settlement, then their first road.
	// Player 2 places their first settlement, then their first road.
	// Player 3 places their first settlement, then their first road.
	// Player 3 places their first city, then their second road.
	// Player 2 places their first city, then their second road.
	// Player 1 places their first city, then their second road.
	// Player 1 takes their turn.
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&gameState]() {
		PhaseStateMachine phaseStateMachine;
		auto result = phaseStateMachine.handle(gameState);
		return result;
	});

	// POST /reset - reset both the database and in-memory game state.
	// TODO: Save game state in database.
	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db, &gameState]() {
		crow::json::wvalue result;
		try {
			bool success = db.resetGame();
			gameState = GameState();
			gameState.phase = "phase to place first settlement";
			gameState.currentPlayer = 1;
			gameState.lastBuilding = "";
			if (success) {
				result["message"] = "Game has been reset to the initial state.";
			}
			else {
				result["error"] = "Resetting game failed.";
			}
		}
		catch (const std::exception& e) {
			result["error"] = std::string("Resetting game failed with the following error.") + e.what();
		}
		return result;
	});

	// GET /roads - return a JSON list of roads.
	// TODO: Resolve the error that occurs after a road is placed and this endpoint is queried where the response is `{"roads":null}`.
	CROW_ROUTE(app, "/roads")
	([&db]() {
		crow::json::wvalue result;
		try {
			auto roads = db.getRoads();
			crow::json::wvalue roadsJson;
			for (size_t i = 0; i < roads.size(); ++i) {
				roadsJson[i]["id"] = roads[i].id;
				roadsJson[i]["player"] = roads[i].player;
				roadsJson[i]["edge"] = roads[i].edge;
			}
			result["roads"] = std::move(roadsJson);
		}
		catch (const std::exception& e) {
			result["error"] = std::string("The following error occurred while fetching roads.") + e.what();
		}
		return result;
	});

	// GET / - simple route endpoint
	CROW_ROUTE(app, "/")
	([]() {
		crow::json::wvalue result;
		result["message"] = "Welcome to the Settlers of Catan API!";
		return result;
	});

	// GET /settlements - return a JSON list of settlements.
	// TODO: Resolve the error that occurs after a settlement is placed and this endpoint is queried where the response is `{"settlements":null}`.
	CROW_ROUTE(app, "/settlements")
	([&db]() {
		crow::json::wvalue result;
		try {
			auto settlements = db.getSettlements();
			crow::json::wvalue settlementsJson;
			for (size_t i = 0; i < settlements.size(); ++i) {
				settlementsJson[i]["id"] = settlements[i].id;
				settlementsJson[i]["player"] = settlements[i].player;
				settlementsJson[i]["vertex"] = settlements[i].vertex;
			}
			result["settlements"] = std::move(settlementsJson);
		}
		catch (const std::exception& e) {
			result["error"] = std::string("The following error occurred while getting settlements.") + e.what();
		}
		return result;
	});

    app.port(5000).multithreaded().run();
    return 0;
}