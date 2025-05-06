#pragma once


#include "../game/board.hpp"
#include "build_next_moves.hpp"
#include "cors_middleware.hpp"
#include "crow.h"
#include "../db/database.hpp"


namespace Server {


	crow::json::wvalue loadBlob(const DB::Database& db, const std::string& key) {
		std::string setting = db.getSetting(key);
        if (setting.empty()) {
            return crow::json::wvalue(crow::json::type::Object);
        }
        auto parsed = crow::json::load(setting);
        if (!parsed) {
            Logger::warn("/state", "Setting \"" + key + "\" contained invalid JSON - returning empty object.");
            return crow::json::wvalue(crow::json::type::Object);
        }
        crow::json::wvalue object;
        object = parsed;
        return object;
	};


    struct DataRoutes {

        static void registerRoutes(crow::App<CorsMiddleware>& app, DB::Database& db) {

            CROW_ROUTE(app, "/cities").methods("GET"_method)(
                [&db]() -> crow::json::wvalue {
                    return db.getCitiesJson();
                }
            );

            CROW_ROUTE(app, "/roads").methods("GET"_method)(
                [&db]() -> crow::json::wvalue {
                    return db.getRoadsJson();
                }
            );

            CROW_ROUTE(app, "/settlements").methods("GET"_method)(
                [&db]() -> crow::json::wvalue {
                    return db.getSettlementsJson();
                }
            );

            CROW_ROUTE(app, "/walls").methods("GET"_method)(
                [&db]() -> crow::json::wvalue {
                    return db.getWallsJson();
                }
            );

            CROW_ROUTE(app, "/state").methods("GET"_method)(
                [&db]() -> crow::response {
                    try {
                        crow::json::wvalue result;
                        result["message"] = db.getSetting("lastMessage");
                        result["dice"] = loadBlob(db, "lastDice");
                        result["gainedResources"] = loadBlob(db, "lastGainedResources");
                        result["totalResources"] = loadBlob(db, "lastTotalResources");
                        buildNextMoves(db, result);
                        result["phase"] = Game::toString(db.getGameState().phase);
                        return crow::response{ 200, result.dump() };
                    }
                    catch (const std::exception& e) {
                        Logger::error("/state", e);
                        crow::json::wvalue err;
                        err["error"] = std::string("Failed to build /state: ") + e.what();
                        return crow::response{ 500, err.dump() };
                    }
                }
            );
        }

    };
    
}