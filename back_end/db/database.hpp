#pragma once

#include "../game/game_state.hpp"
#include "models.hpp"
#include "query_builder.hpp"
#include <mysqlx/xdevapi.h>
/* Add to Additional Include Directories `$(SolutionDir)\dependencies\<debug or release>_version_of_MySQL_Connector_9_2_0\include;`.
* Add to Additional Library Directories `$(SolutionDir)\dependencies\debug_version_of_MySQL_Connector_9_2_0\lib64<\debug or nothing>\vs14;`.
* Add to Additional Dependencies `mysqlcppconnx.lib;`.
* Add to Post Build Event the following string of newline separated commands.
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_MySQL_Connector_9_2_0\lib64<\debug or nothing>\mysqlcppconnx-2-vs14.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_MySQL_Connector_9_2_0\lib64\libssl-3-x64.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_MySQL_Connector_9_2_0\lib64\libcrypto-3-x64.dll" "$(OutDir)"
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
				"CREATE TABLE IF NOT EXISTS " + tablePrefix + "resources ("
				"player INT PRIMARY KEY, "
				"brick INT NOT NULL DEFAULT 0, "
				"grain INT NOT NULL DEFAULT 0, "
				"lumber INT NOT NULL DEFAULT 0, "
				"ore INT NOT NULL DEFAULT 0, "
				"wool INT NOT NULL DEFAULT 0)"
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


            session.sql(
                "CREATE TABLE IF NOT EXISTS " + tablePrefix + "settings ("
				"`key` VARCHAR(50) PRIMARY KEY, "
				"`value` TEXT NOT NULL)"
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
        GameState getGameState() const {
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
			mysqlx::Table resourcesTable = schema.getTable(tablePrefix + "resources");
			mysqlx::RowResult resourcesResult = resourcesTable.select("player", "brick", "grain", "lumber", "ore", "wool").execute();
            if (resourcesResult.count() == 0) {
                for (int player = 1; player <= 3; player++) {
                    resourcesTable.insert("player").values(player).execute();
                }
            }
            else {
                for (mysqlx::Row row : resourcesResult) {
                    int player = row[0];
					gameState.resources[player]["brick"] = row[1];
					gameState.resources[player]["grain"] = row[2];
					gameState.resources[player]["lumber"] = row[3];
					gameState.resources[player]["ore"] = row[4];
					gameState.resources[player]["wool"] = row[5];
                }
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

        void upsertResources(const std::unordered_map<int, std::unordered_map<std::string, int>>& resources) {
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
            const std::string table = tablePrefix + "resources";
            for (const auto& [player, bag] : resources) {
                session.sql(
                    "INSERT INTO " + table + " (player, brick, grain, lumber, ore, wool) "
                    "VALUES(?, ?, ?, ?, ?, ?) " + "ON DUPLICATE KEY UPDATE "
                    "brick = VALUES(brick), "
                    "grain = VALUES(grain), "
                    "lumber = VALUES(lumber), "
                    "ore = VALUES(ore), "
                    "wool = VALUES(wool)"
                ).bind(
                    player,
					bag.at("brick"),
					bag.at("grain"),
					bag.at("lumber"),
					bag.at("ore"),
					bag.at("wool")
				).execute();
            }
        }


		void upsertSetting(const std::string& key, const std::string& value) {
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
			session.sql(
				"INSERT INTO " + tablePrefix + "settings (`key`, `value`) "
				"VALUES(?, ?) ON DUPLICATE KEY UPDATE `value` = VALUES(`value`)"
			).bind(key, value).execute();
		}


        std::string getSetting(const std::string& key) const {
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
			mysqlx::RowResult rowResult = session.sql(
				"SELECT `value` FROM " + tablePrefix + "settings WHERE `key` = ?"
			).bind(key).execute();
			mysqlx::Row row = rowResult.fetchOne();
            return row ? row[0].get<std::string>() : "";
        }


        crow::json::wvalue getResourcesJson() const {
            crow::json::wvalue out(crow::json::type::Object);
            try {
                GameState state = getGameState();
                for (const auto& [player, bag] : state.resources) {
                    crow::json::wvalue bagJson(crow::json::type::Object);
                    for (const auto& [kind, quantity] : bag) {
                        bagJson[kind] = quantity;
                    }
                    out["Player " + std::to_string(player)] = std::move(bagJson);
                }
            }
            catch (const std::exception& e) {
                out["error"] = std::string("The following error occurred while retrieving resources from the database. ") + e.what();
            }
            return out;
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

                mysqlx::Table resourcesTable = schema.getTable(tablePrefix + "resources");
                resourcesTable.remove().execute();
                session.sql("ALTER TABLE " + tablePrefix + "resources AUTO_INCREMENT = 1").execute();
                for (int player = 1; player <= 3; player++) {
                    resourcesTable.insert("player").values(player).execute();
                }

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
			upsertResources(gameState.resources);
        }
    };

}