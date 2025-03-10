#pragma once

#include <mysqlx/xdevapi.h>
/* Add to Additional Include Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\include;`.
* Add to Additional Library Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\vs14;`.
* Add to Additional Dependencies `mysqlcppconnx.lib;`.
* Add to Post Build Event the following string of newline separated commands.
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\mysqlcppconnx-2-vs14.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libssl-3-x64.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libcrypto-3-x64.dll" "$(OutDir)"
*/

#include "models.hpp"


class Database {
public:

	mysqlx::Session session;
	mysqlx::Schema schema;

    Database(
        const std::string& dbName,
        const std::string& host,
        const std::string& password,
        unsigned int port,
        const std::string& username
	) : session(host, port, username, password, dbName),
        schema(session.getSchema(dbName))
    {
        // TODO: Consider setting additional session options if appropriate.
    }

	// TODO: Migrate method initialize to use MySQL X DevAPI.
    void initialize() {
        try {
            // Create cities table.
            session.sql(
                "CREATE TABLE IF NOT EXISTS cities ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            // Create settlements table.
            session.sql(
                "CREATE TABLE IF NOT EXISTS settlements ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            // Create roads table.
            session.sql(
                "CREATE TABLE IF NOT EXISTS roads ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "edge VARCHAR(50) NOT NULL)"
            ).execute();

            // Create state table.
            session.sql(
                "CREATE TABLE IF NOT EXISTS state ("
                "id INT PRIMARY KEY, "
                "current_player INT NOT NULL, "
                "phase VARCHAR(100) NOT NULL, "
                "last_building VARCHAR(50))"
            ).execute();
        }
        catch (const mysqlx::Error& err) {
            std::cerr << "The following error occurred while the database was being initialized." << err.what() << std::endl;
            throw; // Rethrow to allow caller to handle the error.
        }
    }

    /* With the front end running, when I start the back end and get `localhost:3000`, I get the following output.
       However, the front end displays, "Error: Failed to fetch" and the Chrome console outputs the following errors.
       TODO: Resolve the CORS issue.
       TODO: Address why there are so many requests.
    
    (2025 - 03 - 10 21:08 : 06)[INFO] Crow / 1.2.1 server is running at http ://0.0.0.0:5000 using 16 threads
    (2025 - 03 - 10 21:08 : 06)[INFO] Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs.
        (2025 - 03 - 10 21:08 : 10)[INFO] Request: 127.0.0.1 : 59506 0000023F337667D0 HTTP / 1.1 GET / roads
        (2025 - 03 - 10 21:08 : 10)[INFO] Request : 127.0.0.1 : 59505 0000023F33764ED0 HTTP / 1.1 GET / cities
        (2025 - 03 - 10 21:08 : 10)[INFO] Request : 127.0.0.1 : 59504 0000023F337605B0 HTTP / 1.1 GET / settlements
        (2025 - 03 - 10 21:08 : 10)[INFO] Response : 0000023F337605B0 / settlements 200 0
        (2025 - 03 - 10 21:08 : 10)[INFO] Response : 0000023F337667D0 / roads 200 0
        (2025 - 03 - 10 21:08 : 10)[INFO] Response : 0000023F33764ED0 / cities 200 0
        (2025 - 03 - 10 21:08 : 11)[INFO] Request : 127.0.0.1 : 59506 0000023F337667D0 HTTP / 1.1 GET / roads(2025 - 03 - 10 21:08 : 11)[INFO] Request : 127.0.0.1 : 59505 0000023F33764ED0 HTTP / 1.1 GET / settlements

        (2025 - 03 - 10 21:08 : 11)[INFO] Request : 127.0.0.1 : 59504 0000023F337605B0 HTTP / 1.1 GET / cities
        (2025 - 03 - 10 21:08 : 11)[INFO] Response : 0000023F337667D0 / roads 200 0
        (2025 - 03 - 10 21:08 : 11)[INFO] Response : 0000023F337605B0 / cities 200 0
        (2025 - 03 - 10 21:08 : 11)[INFO] Response : 0000023F33764ED0 / settlements 200 0/


    localhost/:1 Access to fetch at 'http://localhost:5000/settlements' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/settlements:1
           Failed to load resource: net::ERR_FAILED

localhost/:1 Access to fetch at 'http://localhost:5000/roads' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/roads:1
           Failed to load resource: net::ERR_FAILED

localhost/:1 Access to fetch at 'http://localhost:5000/cities' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/cities:1
           Failed to load resource: net::ERR_FAILED

localhost/:1 Access to fetch at 'http://localhost:5000/roads' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/roads:1
           Failed to load resource: net::ERR_FAILED

localhost/:1 Access to fetch at 'http://localhost:5000/cities' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/cities:1
           Failed to load resource: net::ERR_FAILED

localhost/:1 Access to fetch at 'http://localhost:5000/settlements' from origin 'http://localhost:3000' has been blocked by CORS policy: No 'Access-Control-Allow-Origin' header is present on the requested resource. If an opaque response serves your needs, set the request's mode to 'no-cors' to fetch the resource with CORS disabled.
:5000/settlements:1
           Failed to load resource: net::ERR_FAILED
    */

    std::vector<City> getCities() {
        std::vector<City> cities;
		mysqlx::Table table = schema.getTable("cities");
		mysqlx::RowResult rowResult = table.select("id", "player", "vertex").execute();
        for (mysqlx::Row row : rowResult) {
			City c;
			c.id = row[0];
			c.player = row[1];
			c.vertex = row[2].get<std::string>();
			cities.push_back(c);
        }
        return cities;
    }

    std::vector<Road> getRoads() {
        std::vector<Road> roads;
        mysqlx::Table table = schema.getTable("roads");
        mysqlx::RowResult rowResult = table.select("id", "player", "edge").execute();
        for (mysqlx::Row row : rowResult) {
            Road r;
            r.id = row[0];
            r.player = row[1];
            r.edge = row[2].get<std::string>();
            roads.push_back(r);
        }
        return roads;
    }

	std::vector<Settlement> getSettlements() {
        std::vector<Settlement> settlements;
        mysqlx::Table table = schema.getTable("settlements");
        mysqlx::RowResult rowResult = table.select("id", "player", "vertex").execute();
        for (mysqlx::Row row : rowResult) {
            Settlement s;
            s.id = row[0];
            s.player = row[1];
            s.vertex = row[2].get<std::string>();
            settlements.push_back(s);
        }
        return settlements;
	}

	// Reset game state (delete all settlements, cities, and roads and reset auto-increments).
    bool resetGame() {
        try {
			schema.getTable("settlements").remove().execute();
			schema.getTable("cities").remove().execute();
			schema.getTable("roads").remove().execute();
			session.sql("ALTER TABLE settlements AUTO_INCREMENT = 1").execute();
			session.sql("ALTER TABLE cities AUTO_INCREMENT = 1").execute();
			session.sql("ALTER TABLE roads AUTO_INCREMENT = 1").execute();
			return true;
		}
        catch (const mysqlx::Error& err) {
            std::cerr << "MySQL X DevAPI Error: " << err.what() << std::endl;
            return false;
        }
        catch (std::exception& ex) {
			std::cerr << "STD Exception: " << ex.what() << std::endl;
            return false;
        }
		catch (...) {
			std::cerr << "An unknown error occurred while resetting the game." << std::endl;
            return false;
		}
    }
};