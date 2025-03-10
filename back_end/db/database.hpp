#pragma once

#include <cppconn/connection.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\include\jdbc;`.

#include <cppconn/driver.h>
/* Add to Additional Library Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\vs14;`.

Add to Additional Dependencies `mysqlcppconn.lib;`.

Add to Post-Build Event the string of newline separated commands
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\mysqlcppconn-10-vs14.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libcrypto-3-x64.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libssl-3-x64.dll" "$(OutDir)"
*/

#include <cppconn/statement.h>

#include "models.hpp"


class Database {
public:
    std::unique_ptr<sql::Connection> conn;

    Database(
        const std::string& dbName,
        const std::string& host,
        const std::string& password,
        const std::string& username
    ) {
        sql::Driver* driver = get_driver_instance();
        conn.reset(driver->connect(host, username, password));
        conn->setSchema(dbName);
    }

    void initialize();

    std::vector<City> getCities() {
        std::vector<City> cities;
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT id, player, vertex FROM cities"));
        while (res->next()) {
            City c;
            c.id = res->getInt("id");
            c.player = res->getInt("player");
            c.vertex = res->getString("vertex");
            cities.push_back(c);
        }
        return cities;
    }

    std::vector<Road> getRoads() {
        std::vector<Road> roads;
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT id, player, edge FROM roads"));
        while (res->next()) {
            Road r;
            r.id = res->getInt("id");
            r.player = res->getInt("player");
            r.edge = res->getString("edge");
            roads.push_back(r);
        }
        return roads;
    }

	std::vector<Settlement> getSettlements() {
		std::vector<Settlement> settlements;
		std::unique_ptr<sql::Statement> stmt(conn->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT id, player, vertex FROM settlements"));
		while (res->next()) {
			Settlement s;
			s.id = res->getInt("id");
			s.player = res->getInt("player");
			s.vertex = res->getString("vertex");
			settlements.push_back(s);
		}
		return settlements;
	}

	// Reset game state (delete all settlements, cities, and roads and reset auto-increments).
    bool resetGame() {
        try {
			std::unique_ptr<sql::Statement> stmt(conn->createStatement());
			stmt->execute("DELETE FROM settlements");
			stmt->execute("DELETE FROM cities");
			stmt->execute("DELETE FROM roads");
			// MySQL-specific code to reset auto-increments.
			stmt->execute("ALTER TABLE settlements AUTO_INCREMENT = 1");
			stmt->execute("ALTER TABLE cities AUTO_INCREMENT = 1");
			stmt->execute("ALTER TABLE roads AUTO_INCREMENT = 1");
			return true;
		}
        catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }
};