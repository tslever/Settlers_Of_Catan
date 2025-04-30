#pragma once


#include "build_next_moves.hpp"
#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "../config.hpp"
#include "cors_middleware.hpp"
#include "data_routes.hpp"
#include "../db/database.hpp"
#include "../game/game.hpp"
#include "game_routes.hpp"
#include "meta_routes.hpp"
#include "../ai/neural_network.hpp"


namespace Server {


	void setUpRoutes(
		crow::App<CorsMiddleware>& app,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetwork,
		const Config::Config& config
	) {
		MetaRoutes::registerRoutes(app);
		DataRoutes::registerRoutes(app, db);
		GameRoutes::registerRoutes(app, db, wrapperOfNeuralNetwork, config);
	}

}