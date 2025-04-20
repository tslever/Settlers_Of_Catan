#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "ai/neural_network.hpp"
#include "db/database.hpp"
#include "game/game.hpp"


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

	void setUpRoutes(
		crow::App<CorsMiddleware>& app,
		DB::Database& liveDb,
		AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetwork,
		const Config::Config& config
	) {
		CROW_ROUTE(app, "/cities").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			try {
				return liveDb.getCitiesJson();
			}
			catch (const std::exception& e) {
				crow::json::wvalue error;
				error["error"] = std::string("Getting cities failed with the following error. ") + e.what();
				return error;
			}
		});

		CROW_ROUTE(app, "/message").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			crow::json::wvalue response;
			response["message"] = liveDb.getSetting("lastMessage");
			return response;
		});

		CROW_ROUTE(app, "/next").methods("POST"_method)
			([&liveDb, &wrapperOfNeuralNetwork, &config]() -> crow::json::wvalue {
			crow::json::wvalue response;
			try {
				Logger::info("A user posted to endpoint next. The game state will be transitioned.");
				GameState currentGameState = liveDb.getGameState();
				Game::Game game(
					liveDb,
					wrapperOfNeuralNetwork,
					config.numberOfSimulations,
					config.cPuct,
					config.tolerance,
					currentGameState,
					config.dirichletMixingWeight,
					config.dirichletShape
				);
				response = game.handlePhase();
				liveDb.updateGameState(game.getState());
				std::string messageWithQuotes = response["message"].dump();
				std::string message;
				if (messageWithQuotes.size() >= 2 && messageWithQuotes.front() == '"' && messageWithQuotes.back() == '"') {
					message = messageWithQuotes.substr(1, messageWithQuotes.size() - 2);
				}
				else {
					message = messageWithQuotes;
				}
				liveDb.upsertSetting("lastMessage", message);
			}
			catch (const std::exception& e) {
				response["error"] = std::string("The following error occurred while transitioning the game state. ") + e.what();
				Logger::error("setUpRoutes", e);
			}
			return response;
		});

		CROW_ROUTE(app, "/reset").methods("POST"_method)
			([&liveDb]() -> crow::json::wvalue {
			crow::json::wvalue response;
			try {
				Logger::info("A user posted to endpoint reset. Game state and database will be reset.");
				bool success = liveDb.resetGame();
				response["message"] = success ? "Game has been reset to initial state." : "Resetting game failed.";
				std::string messageWithQuotes = response["message"].dump();
				std::string message;
				if (messageWithQuotes.size() >= 2 && messageWithQuotes.front() == '"' && messageWithQuotes.back() == '"') {
					message = messageWithQuotes.substr(1, messageWithQuotes.size() - 2);
				}
				else {
					message = messageWithQuotes;
				}
				liveDb.upsertSetting("lastMessage", message);
			}
			catch (const std::exception& e) {
				response["error"] = std::string("Resetting game failed with the following error. ") + e.what();
				Logger::error("setUpRoutes", e);
			}
			return response;
		});

		CROW_ROUTE(app, "/resources").methods("GET"_method)
			([&liveDb]() {
				return liveDb.getResourcesJson();
			});

		CROW_ROUTE(app, "/roads").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			try {
				return liveDb.getRoadsJson();
			}
			catch (const std::exception& e) {
				crow::json::wvalue error;
				error["error"] = std::string("Getting roads failed with the following error. ") + e.what();
				return error;
			}
		});

		CROW_ROUTE(app, "/")
			([]() -> crow::json::wvalue {
			crow::json::wvalue result;
			result["message"] = "Welcome to the Settlers of Catan API!";
			return result;
		});

		CROW_ROUTE(app, "/settlements").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			try {
				return liveDb.getSettlementsJson();
			}
			catch (const std::exception& e) {
				crow::json::wvalue error;
				error["error"] = std::string("Getting settlements failed with the following error. ") + e.what();
				return error;
			}
		});
	}

}