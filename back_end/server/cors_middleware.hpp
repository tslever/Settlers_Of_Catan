#pragma once
 

#include "crow.h"


namespace Server {

    // Structure CorsMiddleware is a simple middleware to add CORS headers.
    struct CorsMiddleware {
        struct context {};

        void before_handle(crow::request&, crow::response&, context&) {
            // Do nothing.
        }

        // After handling, add CORS headers.
        template <typename AllContext>
        void after_handle(const crow::request&, crow::response& res, context&, AllContext&) {
            res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
            res.add_header("Access-Control-Allow-Headers", "Content-Type");
        }
    };

}