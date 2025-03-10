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

	// In-memory game state instance.
	// TODO: Save game state in database.
	GameState gameState;
	gameState.phase = "phase to place first settlement";
	gameState.currentPlayer = 1;
	gameState.lastBuilding = "";

    // GET /cities - return a JSON list of cities.
	/* TODO: Resolve the following error.
	thoma@DESKTOP - DEO3T6M MINGW64 ~
		$ curl - X GET http ://localhost:5000/cities
	500 Internal Server Error
	
	(2025-03-10 03:29:46) [INFO    ] Request: 127.0.0.1:55418 000001C2109BA0E0 HTTP/1.1 GET /cities
(2025-03-10 03:29:46) [ERROR   ] An uncaught exception occurred: Table 'game.cities' doesn't exist
(2025-03-10 03:29:46) [INFO    ] Response: 000001C2109BA0E0 /cities 500 0
	*/
    CROW_ROUTE(app, "/cities")
    ([&db]() {
        auto cities = db.getCities();
        crow::json::wvalue result;
        crow::json::wvalue citiesJson;
        for (size_t i = 0; i < cities.size(); ++i) {
            citiesJson[i]["id"] = cities[i].id;
            citiesJson[i]["player"] = cities[i].player;
            citiesJson[i]["vertex"] = cities[i].vertex;
        }
        result["cities"] = std::move(citiesJson);
        return result;
    });

	// POST /next - transition the game state using phase state machine.
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&gameState]() {
		PhaseStateMachine phaseStateMachine;
		// TODO: Resolve the error 'incomplete type "PhaseStateMachine" is not allowed'.
		auto result = phaseStateMachine.handle(gameState);
		return result;
	});

	// POST /reset - reset both the database and in-memory game state.
	// TODO: Save game state in database.
	/* TODO: Resolve the following error.
	thoma@DESKTOP - DEO3T6M MINGW64 ~
		$ curl - X POST http ://localhost:5000/reset
	{"error":"Resetting game failed."}

	(2025-03-10 03:30:59) [INFO    ] Request: 127.0.0.1:55423 000001C2109BA0E0 HTTP/1.1 POST /reset
Error: Table 'game.settlements' doesn't exist
(2025-03-10 03:30:59) [INFO    ] Response: 000001C2109BA0E0 /reset 200 0
	*/
	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db, &gameState]() {
		bool success = db.resetGame();
		gameState = GameState();
		gameState.phase = "phase to place first settlement";
		gameState.currentPlayer = 1;
		gameState.lastBuilding = "";
		crow::json::wvalue result;
		if (success) {
			result["message"] = "Game has been reset to the initial state.";
		} else {
			result["error"] = "Resetting game failed.";
		}
		return result;
	});

	// GET /roads - return a JSON list of roads.
	/* TODO: Resolve the following error.
	thoma@DESKTOP - DEO3T6M MINGW64 ~
		$ curl - X GET http ://localhost:5000/roads
	500 Internal Server Error

	(2025-03-10 03:31:32) [INFO    ] Request: 127.0.0.1:55425 000001C2109BF640 HTTP/1.1 GET /roads
	(2025-03-10 03:31:32) [ERROR   ] An uncaught exception occurred: Table 'game.roads' doesn't exist
	(2025-03-10 03:31:32) [INFO    ] Response: 000001C2109BF640 /roads 500 0
	*/
	CROW_ROUTE(app, "/roads")
	([&db]() {
		auto roads = db.getRoads();
		crow::json::wvalue result;
		crow::json::wvalue roadsJson;
		for (size_t i = 0; i < roads.size(); ++i) {
			roadsJson[i]["id"] = roads[i].id;
			roadsJson[i]["player"] = roads[i].player;
			roadsJson[i]["edge"] = roads[i].edge;
		}
		result["roads"] = std::move(roadsJson);
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
	/* TODO: Resolve the following error.
	thoma@DESKTOP - DEO3T6M MINGW64 ~
		$ curl - X GET http ://localhost:5000/settlements
	500 Internal Server Error

	(2025-03-10 03:32:13) [INFO    ] Request: 127.0.0.1:55431 000001C2109BF640 HTTP/1.1 GET /settlements
	(2025-03-10 03:32:13) [ERROR   ] An uncaught exception occurred: Table 'game.settlements' doesn't exist
	(2025-03-10 03:32:13) [INFO    ] Response: 000001C2109BF640 /settlements 500 0
	*/
	CROW_ROUTE(app, "/settlements")
	([&db]() {
		auto settlements = db.getSettlements();
		crow::json::wvalue result;
		crow::json::wvalue settlementsJson;
		for (size_t i = 0; i < settlements.size(); ++i) {
			settlementsJson[i]["id"] = settlements[i].id;
			settlementsJson[i]["player"] = settlements[i].player;
			settlementsJson[i]["vertex"] = settlements[i].vertex;
		}
		result["settlements"] = std::move(settlementsJson);
		return result;
	});

    app.port(5000).multithreaded().run();
    return 0;
}