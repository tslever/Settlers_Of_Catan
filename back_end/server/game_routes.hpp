#pragma once


#include "build_next_moves.hpp"
#include "../config.hpp"
#include "cors_middleware.hpp"
#include "crow.h"
#include "../db/database.hpp"
#include "../game/game.hpp"
#include "../logger.hpp"
#include "../ai/neural_network.hpp"


namespace Server {

    struct GameRoutes {

        static void registerRoutes(
            crow::App<CorsMiddleware>& app,
            DB::Database& db,
            AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetwork,
            const Config::Config& config
        ) {

            CROW_ROUTE(app, "/automateMove").methods("POST"_method)(
                [&db, &wrapperOfNeuralNetwork, &config]() -> crow::json::wvalue {
					crow::json::wvalue response;

					try {
						Logger::info("A user posted to endpoint automateMove. The game state will be transitioned.");
						GameState currentGameState = db.getGameState();
						Game::Game game(
							db,
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
						db.updateGameState(currentGameState);

						auto save_if_present = [&](const char* field, const char* setting) {
							const auto& val = response[field];
							if (val.t() != crow::json::type::Null) {
								db.upsertSetting(setting, val.dump());
							}
							};
						std::string message = crow::json::load(response["message"].dump()).s();
						db.upsertSetting("lastMessage", message);
						save_if_present("dice", "lastDice");
						save_if_present("gainedResources", "lastGainedResources");
						save_if_present("totalResources", "lastTotalResources");

						buildNextMoves(db, response);
						response["phase"] = Game::toString(db.getGameState().phase);
					}
					catch (const std::exception& e) {
						response["error"] = std::string("The following error occurred while transitioning the game state. ") + e.what();
						Logger::error("automateMove", e);
					}
					return response;
                }
            );

            CROW_ROUTE(app, "/makeMove").methods("POST"_method)(
                [&db, &wrapperOfNeuralNetwork, &config](const crow::request& request) -> crow::json::wvalue {
					crow::json::wvalue response;
					try {
						auto bodyOfRequest = crow::json::load(request.body);
						std::string move = bodyOfRequest["move"].s();
						std::string moveType = bodyOfRequest["moveType"].s();
						Logger::info("User requested move " + move + " of type " + moveType);

						GameState currentGameState = db.getGameState();
						auto resourcesBeforeMove = currentGameState.resources;
						int player = currentGameState.currentPlayer;

						if (moveType == "road") {
							currentGameState.placeRoad(player, move);
							int id = db.addStructure("roads", player, move, "edge");
							response["road"]["id"] = id;
							response["road"]["player"] = player;
							response["road"]["edge"] = move;
							response["message"] = "Player " + std::to_string(player) + " placed a road at " + move + ".";
						}
						else if (moveType == "settlement") {
							currentGameState.placeSettlement(player, move);
							int id = db.addStructure("settlements", player, move, "vertex");
							response["settlement"]["id"] = id;
							response["settlement"]["player"] = player;
							response["settlement"]["vertex"] = move;
							response["message"] = "Player " + std::to_string(player) + " placed a settlement at " + move + ".";
						}
						else if (moveType == "city") {
							currentGameState.placeCity(player, move);
							DB::WrapperOfSession wrapperOfSession(db.dbName, db.host, db.password, db.port, db.username);
							mysqlx::Session& session = wrapperOfSession.getSession();
							session.startTransaction();
							try {
								mysqlx::Schema schema = session.getSchema(db.dbName);
								mysqlx::Table settlements = schema.getTable(db.tablePrefix + "settlements");
								settlements.remove().where("player = :p AND vertex = :v").bind("p", player).bind("v", move).execute();
								mysqlx::Table cities = schema.getTable(db.tablePrefix + "cities");
								mysqlx::abi2::r0::Result result = cities.insert("player", "vertex").values(player, move).execute();
								int id = static_cast<int>(result.getAutoIncrementValue());
								session.commit();
								response["city"]["id"] = id;
								response["city"]["player"] = player;
								response["city"]["vertex"] = move;
								response["message"] = "Player " + std::to_string(player) + " upgraded to a city at " + move + ".";
							}
							catch (const std::exception& e) {
								session.rollback();
								throw;
							}
						}
						else if (moveType == "wall") {
							bool ok = currentGameState.placeCityWall(player, move);
							if (!ok) {
								throw std::runtime_error("A wall cannot be placed at " + move);
							}
							int id = db.addStructure("walls", player, move, "vertex");
							response["wall"]["id"] = id;
							response["wall"]["player"] = player;
							response["wall"]["vertex"] = move;
							response["message"] = "Player " + std::to_string(player) + " placed a wall at " + move + ".";
						}
						else if (moveType == "pass") {
							if (currentGameState.phase == Game::Phase::Turn) {
								currentGameState.updatePhase();
								response["message"] = "Player " + std::to_string(player) + " passed.";
							}
							else {
								response["message"] = "Phase is not turn.";
							}
						}
						else {
							throw std::runtime_error("Unknown move type: " + moveType);

						}
						db.updateGameState(currentGameState);

						crow::json::wvalue gainedAll(crow::json::type::Object);
						for (int player = 1; player <= 3; ++player) {
							const auto& newBag = currentGameState.resources[player];
							const auto& oldBag = resourcesBeforeMove[player];
							crow::json::wvalue bagJson(crow::json::type::Object);
							bagJson["brick"] = newBag.brick - oldBag.brick;
							bagJson["grain"] = newBag.grain - oldBag.grain;
							bagJson["lumber"] = newBag.lumber - oldBag.lumber;
							bagJson["ore"] = newBag.ore - oldBag.ore;
							bagJson["wool"] = newBag.wool - oldBag.wool;
							bagJson["cloth"] = newBag.cloth - oldBag.cloth;
							bagJson["coin"] = newBag.coin - oldBag.coin;
							bagJson["paper"] = newBag.paper - oldBag.paper;
							gainedAll["Player " + std::to_string(player)] = std::move(bagJson);
						}
						response["gainedResources"] = std::move(gainedAll);
						db.upsertSetting("lastGainedResources", response["gainedResources"].dump());

						crow::json::wvalue totalAll(crow::json::type::Object);
						for (int player = 1; player <= 3; ++player) {
							const auto& bag = currentGameState.resources[player];
							crow::json::wvalue bagJson(crow::json::type::Object);
							bagJson["brick"] = bag.brick;
							bagJson["grain"] = bag.grain;
							bagJson["lumber"] = bag.lumber;
							bagJson["ore"] = bag.ore;
							bagJson["wool"] = bag.wool;
							bagJson["cloth"] = bag.cloth;
							bagJson["coin"] = bag.coin;
							bagJson["paper"] = bag.paper;
							totalAll["Player " + std::to_string(player)] = std::move(bagJson);
						}
						response["totalResources"] = std::move(totalAll);
						db.upsertSetting("lastTotalResources", response["totalResources"].dump());

						buildNextMoves(db, response);
						response["phase"] = Game::toString(currentGameState.phase);
						db.upsertSetting("lastPossibleNextMoves", response["possibleNextMoves"].dump());
						std::string lastMessage = crow::json::load(response["message"].dump()).s();
						db.upsertSetting("lastMessage", lastMessage);
					}
					catch (const std::exception& e) {
						response["error"] = std::string("Making move failed with error ") + e.what();
						Logger::error("makeMove", e);
					}
					return response;
				}
            );

            CROW_ROUTE(app, "/reset").methods("POST"_method)(
                [&db]() -> crow::json::wvalue {
					crow::json::wvalue response;
					try {
						Logger::info("A user posted to endpoint reset. Game state and database will be reset.");
						bool success = db.resetGame();
						response["message"] = success ? "Game has been reset to initial state." : "Resetting game failed.";
						std::string messageWithQuotes = response["message"].dump();
						std::string message = (messageWithQuotes.front() == '"' && messageWithQuotes.back() == '"') ? messageWithQuotes.substr(1, messageWithQuotes.size() - 2) : messageWithQuotes;
						db.upsertSetting("lastMessage", message);
						db.upsertSetting("lastDice", {});

						auto makeZeroBag = []() {
							crow::json::wvalue bag(crow::json::type::Object);
							bag["brick"] = 0;
							bag["grain"] = 0;
							bag["lumber"] = 0;
							bag["ore"] = 0;
							bag["wool"] = 0;
							bag["cloth"] = 0;
							bag["coin"] = 0;
							bag["paper"] = 0;
							return bag;
							};
						crow::json::wvalue zeroTotals(crow::json::type::Object);
						crow::json::wvalue zeroGained(crow::json::type::Object);
						for (int player = 1; player <= 3; ++player) {
							const std::string key = "Player " + std::to_string(player);
							zeroTotals[key] = makeZeroBag();
							zeroGained[key] = makeZeroBag();
						}
						db.upsertSetting("lastGainedResources", zeroGained.dump());
						db.upsertSetting("lastTotalResources", zeroTotals.dump());

						db.upsertSetting("lastPossibleNextMoves", {});
					}
					catch (const std::exception& e) {
						response["error"] = std::string("Resetting game failed with the following error. ") + e.what();
						Logger::error("setUpRoutes", e);
					}
					return response;
				}
            );

            CROW_ROUTE(app, "/recommendMove").methods("GET"_method)(
                [&db, &wrapperOfNeuralNetwork, &config]() -> crow::json::wvalue {
					crow::json::wvalue response;
					try {
						GameState state = db.getGameState();
						auto [move, moveType, visitCount] = runMcts(
							state,
							wrapperOfNeuralNetwork,
							config.numberOfSimulations,
							config.cPuct,
							config.tolerance,
							config.dirichletMixingWeight,
							config.dirichletShape
						);
						std::string message = "Recommended move: " + moveType + " at " + move + ".";
						response["message"] = message;
						db.upsertSetting("lastMessage", message);
					}
					catch (const std::exception& e) {
						response["error"] = std::string("Recommendation failed with error ") + e.what();
						Logger::error("recommendMove", e);
					}
					return response;
				}
            );
        }
    };

}