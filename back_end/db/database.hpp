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
				"wool INT NOT NULL DEFAULT 0, "
				"cloth INT NOT NULL DEFAULT 0, "
				"coin INT NOT NULL DEFAULT 0, "
				"paper INT NOT NULL DEFAULT 0)"
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

            session.sql(
				"CREATE TABLE IF NOT EXISTS " + tablePrefix + "walls ("
				"id INT AUTO_INCREMENT PRIMARY KEY, "
				"player INT NOT NULL, "
				"vertex VARCHAR(50) NOT NULL, "
				"UNIQUE KEY unique_wall (player, vertex))"
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
            mysqlx::abi2::r0::Result result = table.insert("player", locationField).values(player, location).execute();
            return static_cast<int>(result.getAutoIncrementValue());
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

        std::vector<Wall> getWalls() const {
            std::vector<Wall> walls;
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
			mysqlx::Schema schema = session.getSchema(dbName);
			mysqlx::Table table = schema.getTable(tablePrefix + "walls");
			mysqlx::RowResult rowResult = table.select("id", "player", "vertex").execute();
			for (mysqlx::Row row : rowResult) {
                Wall wall{ row[0], row[1], row[2].get<std::string>() };
				walls.push_back(wall);
			}
			return walls;
        }

		crow::json::wvalue getWallsJson() const {
			crow::json::wvalue jsonObject;
			try {
				std::vector<Wall> walls = getWalls();
				jsonObject["walls"] = QueryJsonBuilder::convertVectorOfWallsToJsonObject(walls);
			}
			catch (const std::exception& e) {
				jsonObject["error"] = std::string("The following error occurred while retrieving walls from the database. ") + e.what();
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
                if (!row[1].isNull()) {
                    gameState.phase = Game::fromString(row[1].get<std::string>());
                }
                gameState.lastBuilding = row[2].isNull() ? "" : row[2].get<std::string>();
            }
            else {
                gameState = GameState();
                session
                    .sql(
                        "REPLACE INTO " + tablePrefix + "state(id, current_player, phase, last_building) " +
                        "VALUES(1, ?, ?, ?)"
                    )
                    .bind(gameState.currentPlayer, Game::toString(gameState.phase), gameState.lastBuilding.empty() ? mysqlx::nullvalue : gameState.lastBuilding)
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
			mysqlx::Table wallsTable = schema.getTable(tablePrefix + "walls");
			mysqlx::RowResult wallsResult = wallsTable.select("player", "vertex").execute();
			for (mysqlx::Row wRow : wallsResult) {
				int player = wRow[0];
				std::string vertex = wRow[1].get<std::string>();
				gameState.walls[player].push_back(vertex);
			}
			mysqlx::Table resourcesTable = schema.getTable(tablePrefix + "resources");
			mysqlx::RowResult resourcesResult = resourcesTable
                .select("player", "brick", "grain", "lumber", "ore", "wool", "cloth", "coin", "paper")
                .execute();
            if (resourcesResult.count() == 0) {
                for (int player = 1; player <= 3; player++) {
                    resourcesTable.insert("player").values(player).execute();
                }
            }
            else {
                for (mysqlx::Row row : resourcesResult) {
                    int player = row[0];
                    auto& bag = gameState.resources[player];
                    bag.brick = row[1];
                    bag.grain = row[2];
                    bag.lumber = row[3];
                    bag.ore = row[4];
                    bag.wool = row[5];
                    bag.cloth = row[6];
                    bag.coin = row[7];
                    bag.paper = row[8];
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


        void removeStructure(const std::string& structureSuffix, int player, const std::string& location, const std::string& locationField) {
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
            mysqlx::Schema schema = session.getSchema(dbName);
            mysqlx::Table table = schema.getTable(tablePrefix + structureSuffix);
            table.remove().where("player = " + std::to_string(player) + " AND " + locationField + " = " + location).execute();
        }


        void upsertResources(const std::array<ResourceBag, 4>& resources) {
			WrapperOfSession wrapperOfSession(dbName, host, password, port, username);
			mysqlx::Session& session = wrapperOfSession.getSession();
            const std::string table = tablePrefix + "resources";
            for (int player = 1; player <= 3; ++player) {
                const auto& bag = resources[player];
                session.sql(
                    "INSERT INTO " + table + " (player, brick, grain, lumber, ore, wool, cloth, coin, paper) "
                    "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE "
                    "brick = VALUES(brick), "
                    "grain = VALUES(grain), "
                    "lumber = VALUES(lumber), "
                    "ore = VALUES(ore), "
                    "wool = VALUES(wool), "
                    "cloth = VALUES(cloth), "
					"coin = VALUES(coin), "
					"paper = VALUES(paper)"
                ).bind(
                    player,
					bag.brick,
					bag.grain,
					bag.lumber,
					bag.ore,
					bag.wool,
					bag.cloth,
                    bag.coin,
					bag.paper
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
				schema.getTable(tablePrefix + "walls").remove().execute();
                session.sql("ALTER TABLE " + tablePrefix + "settlements AUTO_INCREMENT = 1").execute();
                session.sql("ALTER TABLE " + tablePrefix + "cities AUTO_INCREMENT = 1").execute();
                session.sql("ALTER TABLE " + tablePrefix + "roads AUTO_INCREMENT = 1").execute();
				session.sql("ALTER TABLE " + tablePrefix + "walls AUTO_INCREMENT = 1").execute();

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
                    .set("phase", Game::toString(initialState.phase))
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
                .set("phase", Game::toString(gameState.phase))
                .set("last_building", gameState.lastBuilding.empty() ? mysqlx::nullvalue : gameState.lastBuilding)
                .where("id = 1")
                .execute();
			upsertResources(gameState.resources);
        }
    };

}