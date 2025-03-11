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

	std::string dbName;
    std::string host;
	std::string password;
	unsigned int port;
	std::string username;

    Database(
        const std::string& dbName,
        const std::string& host,
        const std::string& password,
        unsigned int port,
        const std::string& username
	) : dbName(dbName), host(host), password(password), port(port), username(username)
    {
        // TODO: Consider setting additional session options if appropriate.
    }

    void initialize() {
        try {
            mysqlx::Session session(host, port, username, password, dbName);
			mysqlx::Schema schema = session.getSchema(dbName);

            session.sql(
                "CREATE TABLE IF NOT EXISTS cities ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS settlements ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS roads ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "edge VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS state ("
                "id INT PRIMARY KEY, "
                "current_player INT NOT NULL, "
                "phase VARCHAR(100) NOT NULL, "
                "last_building VARCHAR(50))"
            ).execute();
        }
        catch (const mysqlx::Error& err) {
            // TODO: Ensure that this is the appropriate error.
            std::cerr << "The following error occurred while the database was being initialized." << err.what() << std::endl;
            throw; // Rethrow to allow caller to handle the error.
        }
    }

    std::vector<City> getCities() {
        std::vector<City> cities;
		mysqlx::Session session(host, port, username, password, dbName);
		mysqlx::Schema schema = session.getSchema(dbName);
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
		mysqlx::Session session(host, port, username, password, dbName);
		mysqlx::Schema schema = session.getSchema(dbName);
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
		mysqlx::Session session(host, port, username, password, dbName);
		mysqlx::Schema schema = session.getSchema(dbName);
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
			mysqlx::Session session(host, port, username, password, dbName);
			mysqlx::Schema schema = session.getSchema(dbName);

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