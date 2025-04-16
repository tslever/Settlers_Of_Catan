#pragma once

#include "../game/game_state.hpp"
#include "models.hpp"
#include "query_builder.hpp"
#include <mysqlx/xdevapi.h>
/* Add to Additional Include Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\include;`.
* Add to Additional Library Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\vs14;`.
* Add to Additional Dependencies `mysqlcppconnx.lib;`.
* Add to Post Build Event the following string of newline separated commands.
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\debug\mysqlcppconnx-2-vs14.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libssl-3-x64.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64\libcrypto-3-x64.dll" "$(OutDir)"
*/


namespace DB {

    class WrapperOfSession {
    public:
        WrapperOfSession(
			const std::string& dbName,
            const std::string& host,
			const std::string& password,
			unsigned int port,
            const std::string& username
        ) : session(host, port, username, password, dbName) {
            // TODO: Consider performing additional configuration on the session.
        }

		mysqlx::Session& getSession() {
			return session;
		}

        ~WrapperOfSession() {
            session.close();
        }

    private:
        mysqlx::Session session;
    };

    class Database {
    public:
        std::string dbName;
        std::string host;
        std::string password;
        unsigned int port;
        std::string username;
        std::string tablePrefix;

        Database(
            const std::string& dbName,
            const std::string& host,
            const std::string& password,
            unsigned int port,
            const std::string& username,
            const std::string& tablePrefix
        ) : dbName(dbName),
            host(host),
            password(password),
            port(port),
            username(username),
            tablePrefix(tablePrefix)
        {
            // TODO: Consider performing additional configuration on the database.
        }

        void initialize() {
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);

            session.sql(
                "CREATE TABLE IF NOT EXISTS " + tablePrefix + "cities ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS " + tablePrefix + "settlements ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "vertex VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS " + tablePrefix + "roads ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "player INT NOT NULL, "
                "edge VARCHAR(50) NOT NULL)"
            ).execute();

            session.sql(
                "CREATE TABLE IF NOT EXISTS " + tablePrefix + "state ("
                "id INT PRIMARY KEY, "
                "current_player INT NOT NULL, "
                "phase VARCHAR(100) NOT NULL, "
                "last_building VARCHAR(50))"
            ).execute();
        }

        int addStructure(
            const std::string& structureSuffix,
            int player,
            const std::string& location,
            const std::string& locationField
        ) {
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + structureSuffix);
            table.insert("player", locationField).values(player, location).execute();
            mysqlx::RowResult rowResult = table
                .select("id")
                .where("player = " + std::to_string(player) + " AND " + locationField + " = '" + location + "'")
                .execute();
            mysqlx::Row row = rowResult.fetchOne();
            if (row) {
                return row[0];
            }
            return -1;
        }

        std::vector<City> getCities() const {
            std::vector<City> cities;
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + "cities");
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

        crow::json::wvalue getCitiesJson() const {
            crow::json::wvalue jsonObject;
            try {
                std::vector<City> cities = getCities();
                jsonObject["cities"] = QueryJsonBuilder::convertVectorOfCitiesToJsonObject(cities);
            }
            catch (const std::exception& e) {
                jsonObject["error"] = std::string("The following error occurred while retrieving cities from the database. ") + e.what();
            }
            return jsonObject;
        }

        /* Method `getGameState` returns the current game state stored in the state table.
        * If no record exists, `getGameState` creates a new record with default values.
        */
        GameState getGameState() {
            GameState gameState;
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + "state");
            mysqlx::RowResult rowResult = table.select("current_player", "phase", "last_building").where("id = 1").execute();
            mysqlx::Row row = rowResult.fetchOne();
            if (row) {
                gameState.currentPlayer = row[0];
                gameState.phase = row[1].get<std::string>();
                gameState.lastBuilding = row[2].get<std::string>();
            }
            else {
                gameState = GameState();
                session.sql("REPLACE INTO " + tablePrefix + "state(id, current_player, phase, last_building) VALUES(1, ? , ? , ? )")
                    .bind(gameState.currentPlayer)
                    .bind(gameState.phase)
                    .bind(gameState.lastBuilding)
                    .execute();
            }
            mysqlx::Table settlementsTable = schema.getTable(tablePrefix + "settlements");
            mysqlx::RowResult settlementsResult = settlementsTable.select("player", "vertex").execute();
            for (mysqlx::Row sRow : settlementsResult) {
                int player = sRow[0];
                std::string vertex = sRow[1].get<std::string>();
                gameState.settlements[player].push_back(vertex);
            }
            mysqlx::Table citiesTable = schema.getTable(tablePrefix + "cities");
            mysqlx::RowResult citiesResult = citiesTable.select("player", "vertex").execute();
            for (mysqlx::Row cRow : citiesResult) {
                int player = cRow[0];
                std::string vertex = cRow[1].get<std::string>();
                gameState.cities[player].push_back(vertex);
            }
            mysqlx::Table roadsTable = schema.getTable(tablePrefix + "roads");
            mysqlx::RowResult roadsResult = roadsTable.select("player", "edge").execute();
            for (mysqlx::Row rRow : roadsResult) {
                int player = rRow[0];
                std::string edge = rRow[1].get<std::string>();
                gameState.roads[player].push_back(edge);
            }
            return gameState;
        }

        std::vector<Road> getRoads() const {
            std::vector<Road> roads;
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + "roads");
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

        crow::json::wvalue getRoadsJson() const {
            crow::json::wvalue jsonObject;
            try {
                std::vector<Road> roads = getRoads();
                jsonObject["roads"] = QueryJsonBuilder::convertVectorOfRoadsToJsonObject(roads);
            }
            catch (const std::exception& e) {
                jsonObject["error"] = std::string("The following error occurred while retrieving roads from the database. ") + e.what();
            }
            return jsonObject;
        }

        std::vector<Settlement> getSettlements() const {
            std::vector<Settlement> settlements;
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + "settlements");
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

        crow::json::wvalue getSettlementsJson() const {
            crow::json::wvalue jsonObject;
            try {
                std::vector<Settlement> settlements = getSettlements();
                jsonObject["settlements"] = QueryJsonBuilder::convertVectorOfSettlementsToJsonObject(settlements);
            }
            catch (const std::exception& e) {
                jsonObject["error"] = std::string("The following error occurred while retrieving cities from the database. ") + e.what();
            }
            return jsonObject;
        }

        // Reset game state (delete all settlements, cities, and roads and reset auto-increments).
        bool resetGame() {
            try {
                WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
                mysqlx::Session& session = wrapperOfSession.getSession();
                mysqlx::Schema schema = session.getSchema(dbName);

                // Clear settlements, cities, and roads.
                schema.getTable(tablePrefix + "settlements").remove().execute();
                schema.getTable(tablePrefix + "cities").remove().execute();
                schema.getTable(tablePrefix + "roads").remove().execute();
                session.sql("ALTER TABLE " + tablePrefix + "settlements AUTO_INCREMENT = 1").execute();
                session.sql("ALTER TABLE " + tablePrefix + "cities AUTO_INCREMENT = 1").execute();
                session.sql("ALTER TABLE " + tablePrefix + "roads AUTO_INCREMENT = 1").execute();

                // Reset the game state to its initial values.
                GameState initialState;
                mysqlx::Table stateTable = schema.getTable(tablePrefix + "state");
                stateTable.update()
                    .set("current_player", initialState.currentPlayer)
                    .set("phase", initialState.phase)
                    .set("last_building", initialState.lastBuilding)
                    .where("id = 1")
                    .execute();

                return true;
            }
            catch (const mysqlx::Error& e) {
                Logger::error("resetGame", e);
                return false;
            }
            catch (std::exception& e) {
                Logger::error("resetGame", e);
                return false;
            }
            catch (...) {
				Logger::error("resetGame", "An unknown error occurred.");
                return false;
            }
        }

        // Method `updateGameState` updates the state table with the current game state.
        void updateGameState(const GameState& gameState) {
            WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
            mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + "state");
            table
                .update()
                .set("current_player", gameState.currentPlayer)
                .set("phase", gameState.phase)
                .set("last_building", gameState.lastBuilding)
                .where("id = 1")
                .execute();
        }
    };

}