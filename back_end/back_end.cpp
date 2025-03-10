// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "db/database.hpp"


int main() {

    crow::SimpleApp app;

    // Configure your database connection.
	std::string dbName = "game";
    std::string host = "tcp://127.0.0.1:3306";
	std::string password = "settlers_of_catan";
    std::string username = "administrator";
    Database db(dbName, host, password, username);

    // GET /cities - return a JSON list of cities.
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

	// POST /next - a placeholder route to transition game state.
	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db]() {
		// TODO: Migrate `phase_state_machine.py` from Python to C++.
		// TODO: Migrate C++ phase state logic here.
		crow::json::wvalue result;
		result["message"] = "Game state transitioned (placeholder)";
		return result;
	});

	// POST /reset - reset the game to the initial state.
	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db]() {
		bool success = db.resetGame();
		crow::json::wvalue result;
		if (success) {
			result["message"] = "Game has been reset to the initial state.";
		} else {
			result["error"] = "Resetting game failed.";
		}
		return result;
	});

	// GET /roads - return a JSON list of roads.
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