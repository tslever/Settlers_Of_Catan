// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "db/database.hpp"

//#include "db/models.hpp"

//#include <memory>


int main() {

    crow::SimpleApp app;

    // Configure your database connection.
    std::string host = "tcp://127.0.0.1:3306";
    std::string user = "administrator";
    std::string password = "settlers_of_catan";
    std::string dbName = "game";
    Database db(host, user, password, dbName);

    // Example route: GET /cities
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

    // TODO: Define additional routes (/settlements, /roads, /next, /reset, etc.) similarly.

    // Start the server on port 5000.
    app.port(5000).multithreaded().run();
    return 0;
}