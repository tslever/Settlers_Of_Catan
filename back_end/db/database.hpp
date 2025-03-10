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
        const std::string& host,
        const std::string& user,
        const std::string& password,
        const std::string& dbName
    ) {
        sql::Driver* driver = get_driver_instance();
        conn.reset(driver->connect(host, user, password));
        conn->setSchema(dbName);
    }

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

    // TODO: Implement similar functions for settlements, roads, etc.*/
};