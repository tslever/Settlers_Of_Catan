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


	crow::json::wvalue loadBlob(const DB::Database& db, const std::string& key) {
		std::string setting = db.getSetting(key);
		return setting.empty() ? crow::json::wvalue(crow::json::type::Object) : crow::json::load(setting);
	};


	void setUpRoutes(
		crow::App<CorsMiddleware>& app,
		DB::Database& liveDb,
		AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetwork,
		const Config::Config& config
	) {
		CROW_ROUTE(app, "/")
			([]() -> crow::json::wvalue {
			crow::json::wvalue result;
			result["message"] = "Welcome to the Settlers of Catan API!";
			return result;
				});


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
				currentGameState = game.getState();
				liveDb.updateGameState(currentGameState);
				std::string messageWithQuotes = response["message"].dump();
				std::string message = (messageWithQuotes.front() == '"' && messageWithQuotes.back() == '"') ? messageWithQuotes.substr(1, messageWithQuotes.size() - 2) : messageWithQuotes;
				liveDb.upsertSetting("lastMessage", message);
				liveDb.upsertSetting("lastDice", response["dice"].dump());
				liveDb.upsertSetting("lastGainedResources", response["gainedResources"].dump());
				liveDb.upsertSetting("lastTotalResources", response["totalResources"].dump());

				GameState nextState = liveDb.getGameState();
				const int nextPlayer = nextState.currentPlayer;
				const std::string nextPhase = nextState.phase;
				Board board;
				std::vector<std::string> vectorOfLabelsOfVerticesToHighlight;
				std::vector<std::string> vectorOfLabelsOfEdgesToHighlight;
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = board.getVectorOfLabelsOfOccupiedVertices(nextState);
				std::vector<std::string> vectorOfLabelsOfOccupiedEdges = board.getVectorOfLabelsOfOccupiedEdges(nextState);
				std::vector<std::string> vectorOfLabelsOfAvailableVertices = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
				std::vector<std::string> vectorOfLabelsOfAvailableEdges = board.getVectorOfLabelsOfAvailableEdges(vectorOfLabelsOfOccupiedEdges);
				bool nextPlayerWillRollDice = false;

				if (nextPhase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
					for (auto& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
						vectorOfLabelsOfVerticesToHighlight.push_back(labelOfAvailableVertex);
					}
				}
				else if (nextPhase == Game::Phase::TO_PLACE_FIRST_ROAD) {
					for (auto& labelOfAvailableEdge : vectorOfLabelsOfAvailableEdges) {
						vectorOfLabelsOfEdgesToHighlight.push_back(labelOfAvailableEdge);
					}
				}
				else if (nextPhase == Game::Phase::TO_PLACE_FIRST_CITY) {
					for (auto& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
						vectorOfLabelsOfVerticesToHighlight.push_back(labelOfAvailableVertex);
					}
				}
				else if (nextPhase == Game::Phase::TO_PLACE_SECOND_ROAD) {
					for (auto& labelOfAvailableEdge : vectorOfLabelsOfAvailableEdges) {
						vectorOfLabelsOfEdgesToHighlight.push_back(labelOfAvailableEdge);
					}
				}
				else if (nextPhase == Game::Phase::TO_ROLL_DICE) {
					nextPlayerWillRollDice = true;
				}
				else if (nextPhase == Game::Phase::TURN) {
					std::unordered_map<std::string, int>& unorderedMapOfNamesAndNumbersOfResources = nextState.resources.at(nextPlayer);
					std::unordered_set<std::string> unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer;
					std::vector<std::string>& vectorOfLabelsOfEdgesWithRoadsOfPlayer = nextState.roads.at(nextPlayer);
					for (const std::string& labelOfEdge : vectorOfLabelsOfEdgesWithRoadsOfPlayer) {
						auto [firstLabelOfVertex, secondLabelOfVertex] = board.getVerticesOfEdge(labelOfEdge);
						unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(firstLabelOfVertex);
						unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(secondLabelOfVertex);
					}
					for (std::string& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
						if (
							unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(labelOfAvailableVertex) &&
							unorderedMapOfNamesAndNumbersOfResources["brick"] >= 1 &&
							unorderedMapOfNamesAndNumbersOfResources["grain"] >= 1 &&
							unorderedMapOfNamesAndNumbersOfResources["lumber"] >= 1 &&
							unorderedMapOfNamesAndNumbersOfResources["wool"] >= 1
						) {
							vectorOfLabelsOfVerticesToHighlight.push_back(labelOfAvailableVertex);
						}
					}
					std::vector<std::string> vectorOfLabelsOfVerticesWithSettlementOfPlayer = nextState.settlements.at(nextPlayer);
					for (std::string& labelOfVertex : vectorOfLabelsOfVerticesWithSettlementOfPlayer) {
						if (unorderedMapOfNamesAndNumbersOfResources["grain"] >= 2 && unorderedMapOfNamesAndNumbersOfResources["ore"] >= 3) {
							vectorOfLabelsOfVerticesToHighlight.push_back(labelOfVertex);
						}
					}
					std::vector<std::string> vectorOfLabelsOfVerticesWithCityOfPlayer = nextState.cities.at(nextPlayer);
					for (std::string& labelOfVertex : vectorOfLabelsOfVerticesWithCityOfPlayer) {
						if (
							unorderedMapOfNamesAndNumbersOfResources["brick"] >= 2 &&
							std::find(nextState.walls.at(nextPlayer).begin(), nextState.walls.at(nextPlayer).end(), labelOfVertex) == nextState.walls.at(nextPlayer).end()
						) {
							vectorOfLabelsOfVerticesToHighlight.push_back(labelOfVertex);
						}
					}
					for (auto& labelOfEdge : vectorOfLabelsOfAvailableEdges) {
						auto [firstLabelOfVertex, secondLabelOfVertex] = board.getVerticesOfEdge(labelOfEdge);
						if (
							(unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(firstLabelOfVertex) || unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(secondLabelOfVertex)) &&
							unorderedMapOfNamesAndNumbersOfResources["brick"] >= 1 &&
							unorderedMapOfNamesAndNumbersOfResources["lumber"] >= 1
						) {
							vectorOfLabelsOfEdgesToHighlight.push_back(labelOfEdge);
						}
					}
				}
				crow::json::wvalue jsonObjectOfMovesToHighlight(crow::json::type::Object);
				jsonObjectOfMovesToHighlight["player"] = nextPlayer;
				jsonObjectOfMovesToHighlight["verticesToHighlight"] = crow::json::wvalue(crow::json::type::List);
				for (size_t i = 0; i < vectorOfLabelsOfVerticesToHighlight.size(); ++i) {
					jsonObjectOfMovesToHighlight["verticesToHighlight"][i] = vectorOfLabelsOfVerticesToHighlight[i];
				}
				jsonObjectOfMovesToHighlight["edgesToHighlight"] = crow::json::wvalue(crow::json::type::List);
				for (size_t i = 0; i < vectorOfLabelsOfEdgesToHighlight.size(); ++i) {
					jsonObjectOfMovesToHighlight["edgesToHighlight"][i] = vectorOfLabelsOfEdgesToHighlight[i];
				}
				jsonObjectOfMovesToHighlight["nextPlayerWillRollDice"] = nextPlayerWillRollDice;
				response["movesToHighlight"] = std::move(jsonObjectOfMovesToHighlight);
				liveDb.upsertSetting("lastMovesToHighlight", response["movesToHighlight"].dump());
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
				std::string message = (messageWithQuotes.front() == '"' && messageWithQuotes.back() == '"') ? messageWithQuotes.substr(1, messageWithQuotes.size() - 2) : messageWithQuotes;
				liveDb.upsertSetting("lastMessage", message);
				liveDb.upsertSetting("lastDice", {});
				liveDb.upsertSetting("lastGainedResources", {});
				liveDb.upsertSetting("lastTotalResources", {});
				liveDb.upsertSetting("lastMovesToHighlight", {});
			}
			catch (const std::exception& e) {
				response["error"] = std::string("Resetting game failed with the following error. ") + e.what();
				Logger::error("setUpRoutes", e);
			}
			return response;
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


		CROW_ROUTE(app, "/walls").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			try {
				return liveDb.getWallsJson();
			}
			catch (const std::exception& e) {
				crow::json::wvalue error;
				error["error"] = std::string("Getting walls failed with the following error. ") + e.what();
				return error;
			}
		});


		CROW_ROUTE(app, "/state").methods("GET"_method)
			([&liveDb]() -> crow::json::wvalue {
			crow::json::wvalue result;
			try {
				result["message"] = liveDb.getSetting("lastMessage");
				result["dice"] = loadBlob(liveDb, "lastDice");
				result["gainedResources"] = loadBlob(liveDb, "lastGainedResources");
				result["totalResources"] = loadBlob(liveDb, "lastTotalResources");
				result["movesToHighlight"] = loadBlob(liveDb, "lastMovesToHighlight");
			}
			catch (const std::exception& e) {
				result["error"] = std::string("Getting game state failed with the following error. ") + e.what();
				Logger::error("state", e);
			}
			return result;
		});
	}

}