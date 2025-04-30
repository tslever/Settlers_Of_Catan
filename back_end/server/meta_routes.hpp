#pragma once


#include "crow.h"
#include "../db/database.hpp"
#include "cors_middleware.hpp"


namespace Server {

    struct MetaRoutes {

        static void registerRoutes(crow::App<CorsMiddleware>& app) {

            CROW_ROUTE(app, "/").methods("GET"_method)(
                []() -> crow::json::wvalue {
                    crow::json::wvalue result;
                    result["message"] = "Welcome to the Settlers of Catan API!";
                    return result;
                }
            );

        }
    };

}