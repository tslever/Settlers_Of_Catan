#pragma once


#include "../db/database.hpp"
#include "crow/json.h"
#include "../ai/neural_network.hpp"
#include "../ai/strategy.hpp"


namespace Game {

	class Game {


	public:


		Game(
			DB::Database& dbToUse,
			AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetworkToUse,
			int numberOfSimulationsToUse,
			double cPuctToUse,
			double toleranceToUse,
			const GameState& gameStateToUse,
			double dirichletMixingWeightToUse,
			double dirichletShapeToUse
		) : db(dbToUse),
			wrapperOfNeuralNetwork(wrapperOfNeuralNetworkToUse),
			numberOfSimulations(numberOfSimulationsToUse),
			cPuct(cPuctToUse),
			tolerance(toleranceToUse),
			state(gameStateToUse),
			dirichletMixingWeight(dirichletMixingWeightToUse),
			dirichletShape(dirichletShapeToUse)
		{
			// Do nothing.
		}

		crow::json::wvalue handlePhase() {
			if (state.phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
				return handleFirstSettlement();
			}
			else if (state.phase == Phase::TO_PLACE_FIRST_ROAD) {
				return handleFirstRoad();
			}
			else if (state.phase == Phase::TO_PLACE_FIRST_CITY) {
				return handleFirstCity();
			}
			else if (state.phase == Phase::TO_PLACE_SECOND_ROAD) {
				return handleSecondRoad();
			}
			else if (state.phase == Phase::TO_ROLL_DICE) {
				return handleRollingDice();
			}
			else if (state.phase == Phase::TURN) {
				return handleTurn();
			}
			else if (state.phase == Phase::DONE) {
				return handleEnd();
			}
			else {
				crow::json::wvalue jsonObject;
				jsonObject["error"] = "Phase " + state.phase + " is unrecognized.";
				return jsonObject;
			}
		}

		const GameState& getState() const {
			return state;
		}


	private:


		GameState state;
		DB::Database& db;
		AI::WrapperOfNeuralNetwork& wrapperOfNeuralNetwork;
		int numberOfSimulations;
		double cPuct;
		double tolerance;
		double dirichletMixingWeight;
		double dirichletShape;


		crow::json::wvalue handleFirstSettlement() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestVertexAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			std::string labelOfChosenVertex = pairOfLabelOfBestVertexAndVisitCount.first;
			if (labelOfChosenVertex.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeSettlement(currentPlayer, labelOfChosenVertex);
			int settlementId = db.addStructure("settlements", currentPlayer, labelOfChosenVertex, "vertex");
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + labelOfChosenVertex + ".";
			crow::json::wvalue jsonObjectOfSettlementInformation;
			jsonObjectOfSettlementInformation["id"] = settlementId;
			jsonObjectOfSettlementInformation["player"] = currentPlayer;
			jsonObjectOfSettlementInformation["vertex"] = labelOfChosenVertex;
			jsonObjectOfMoveInformation["settlement"] = std::move(jsonObjectOfSettlementInformation);
			return jsonObjectOfMoveInformation;
		}


		crow::json::wvalue handleFirstRoad() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			std::string labelOfChosenEdge = pairOfLabelOfBestEdgeAndVisitCount.first;
			if (labelOfChosenEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeRoad(currentPlayer, labelOfChosenEdge);
			int roadId = db.addStructure("roads", currentPlayer, labelOfChosenEdge, "edge");
			Board board;
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + labelOfChosenEdge + ".";
			crow::json::wvalue jsonObjectOfRoadInformation;
			jsonObjectOfRoadInformation["id"] = roadId;
			jsonObjectOfRoadInformation["player"] = currentPlayer;
			jsonObjectOfRoadInformation["edge"] = labelOfChosenEdge;
			jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
			return jsonObjectOfMoveInformation;
		}


		crow::json::wvalue handleFirstCity() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestVertexAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			std::string labelOfChosenVertex = pairOfLabelOfBestVertexAndVisitCount.first;
			if (labelOfChosenVertex.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeCity(currentPlayer, labelOfChosenVertex);
			state.phase = Phase::TO_PLACE_SECOND_ROAD;
			int cityId = db.addStructure("cities", currentPlayer, labelOfChosenVertex, "vertex");
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a city at " + labelOfChosenVertex + ".";
			crow::json::wvalue jsonObjectOfCityInformation;
			jsonObjectOfCityInformation["id"] = cityId;
			jsonObjectOfCityInformation["player"] = currentPlayer;
			jsonObjectOfCityInformation["vertex"] = labelOfChosenVertex;
			jsonObjectOfMoveInformation["city"] = std::move(jsonObjectOfCityInformation);
			return jsonObjectOfMoveInformation;
		}


		crow::json::wvalue handleSecondRoad() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			std::string labelOfChosenEdge = pairOfLabelOfBestEdgeAndVisitCount.first;
			if (labelOfChosenEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeRoad(currentPlayer, labelOfChosenEdge);
			int roadId = db.addStructure("roads", currentPlayer, labelOfChosenEdge, "edge");
			Board board;
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + labelOfChosenEdge + ".";
			crow::json::wvalue jsonObjectOfRoadInformation;
			jsonObjectOfRoadInformation["id"] = roadId;
			jsonObjectOfRoadInformation["player"] = currentPlayer;
			jsonObjectOfRoadInformation["edge"] = labelOfChosenEdge;
			jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
			return jsonObjectOfMoveInformation;
		}


		crow::json::wvalue handleRollingDice() {
			std::unordered_map<int, std::unordered_map<std::string, int>> resourcesBeforeRoll = state.resources;

			state.rollDice();
			crow::json::wvalue jsonObject;
			jsonObject["message"] = "Player " + std::to_string(state.currentPlayer) + " rolled the dice.";
			crow::json::wvalue jsonObjectOfDescriptionOfDiceAndRolls;
			jsonObjectOfDescriptionOfDiceAndRolls["yellowProductionDie"] = state.yellowProductionDie;
			jsonObjectOfDescriptionOfDiceAndRolls["redProductionDie"] = state.redProductionDie;
			jsonObjectOfDescriptionOfDiceAndRolls["whiteEventDie"] = state.whiteEventDie;
			jsonObject["dice"] = std::move(jsonObjectOfDescriptionOfDiceAndRolls);

			crow::json::wvalue gainedAll(crow::json::type::Object);
			for (const auto& [player, newBag] : state.resources) {
				crow::json::wvalue bagJson(crow::json::type::Object);
				const auto& oldBag = resourcesBeforeRoll.at(player);
				for (const auto& [kind, newQuantity] : newBag) {
					int oldQuantity = oldBag.at(kind);
					int gainedQuantity = newQuantity - oldQuantity;
					bagJson[kind] = gainedQuantity;
				}
				gainedAll["Player " + std::to_string(player)] = std::move(bagJson);
			}
			jsonObject["gainedResources"] = std::move(gainedAll);

			jsonObject["totalResources"] = makeTotalsJson();

			state.phase = Phase::TURN;
			return jsonObject;
		}


		crow::json::wvalue handleTurn() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			auto resourcesBeforeMove = state.resources;
			auto [labelOfChosenVertexOrEdge, visitCount] = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			if (labelOfChosenVertexOrEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			if (labelOfChosenVertexOrEdge == "pass") {
				state.updatePhase();
				jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " passed.";
			}
			else {
				Board board;
				if (board.isLabelOfVertex(labelOfChosenVertexOrEdge)) {
					std::vector<std::string>& vectorOfLabelsOfSettlementsOfPlayer = state.settlements[currentPlayer];
					bool hasSettlement = std::find(
						vectorOfLabelsOfSettlementsOfPlayer.begin(),
						vectorOfLabelsOfSettlementsOfPlayer.end(),
						labelOfChosenVertexOrEdge
					) != vectorOfLabelsOfSettlementsOfPlayer.end();
					bool canUpgrade = state.resources[currentPlayer]["grain"] >= 2 && state.resources[currentPlayer]["ore"] >= 3;

					if (hasSettlement && canUpgrade) {
						state.placeCity(currentPlayer, labelOfChosenVertexOrEdge);
						db.removeStructure("settlements", currentPlayer, labelOfChosenVertexOrEdge, "vertex");
						int cityId = db.addStructure("cities", currentPlayer, labelOfChosenVertexOrEdge, "vertex");
						jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " upgraded a settlement to a city at " + labelOfChosenVertexOrEdge + ".";
						crow::json::wvalue jsonObjectOfCityInformation;
						jsonObjectOfCityInformation["id"] = cityId;
						jsonObjectOfCityInformation["player"] = currentPlayer;
						jsonObjectOfCityInformation["vertex"] = labelOfChosenVertexOrEdge;
						jsonObjectOfMoveInformation["city"] = std::move(jsonObjectOfCityInformation);
					}
					else {
						state.placeSettlement(currentPlayer, labelOfChosenVertexOrEdge);
						int settlementId = db.addStructure("settlements", currentPlayer, labelOfChosenVertexOrEdge, "vertex");
						jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + labelOfChosenVertexOrEdge + ".";
						crow::json::wvalue jsonObjectOfSettlementInformation;
						jsonObjectOfSettlementInformation["id"] = settlementId;
						jsonObjectOfSettlementInformation["player"] = currentPlayer;
						jsonObjectOfSettlementInformation["vertex"] = labelOfChosenVertexOrEdge;
						jsonObjectOfMoveInformation["settlement"] = std::move(jsonObjectOfSettlementInformation);
					}
				}
				else if (board.isLabelOfEdge(labelOfChosenVertexOrEdge)) {
					state.placeRoad(currentPlayer, labelOfChosenVertexOrEdge);
					int roadId = db.addStructure("roads", currentPlayer, labelOfChosenVertexOrEdge, "edge");
					jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + labelOfChosenVertexOrEdge + ".";
					crow::json::wvalue jsonObjectOfRoadInformation;
					jsonObjectOfRoadInformation["id"] = roadId;
					jsonObjectOfRoadInformation["player"] = currentPlayer;
					jsonObjectOfRoadInformation["edge"] = labelOfChosenVertexOrEdge;
					jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
				}
			}
			crow::json::wvalue gainedAll(crow::json::type::Object);
			for (const auto& [player, newBag] : state.resources) {
				crow::json::wvalue bagJson(crow::json::type::Object);
				const auto& oldBag = resourcesBeforeMove.at(player);
				for (const auto& [kind, newQuantity] : newBag) {
					int oldQuantity = oldBag.at(kind);
					bagJson[kind] = newQuantity - oldQuantity;
				}
				gainedAll["Player " + std::to_string(player)] = std::move(bagJson);
			}
			jsonObjectOfMoveInformation["gainedResources"] = std::move(gainedAll);
			jsonObjectOfMoveInformation["totalResources"] = makeTotalsJson();
			return jsonObjectOfMoveInformation;
		}


		crow::json::wvalue handleEnd() {
			crow::json::wvalue jsonObjectOfEndInformation;
			jsonObjectOfEndInformation["message"] = "Game is over. Thanks for playing!";
			return jsonObjectOfEndInformation;
		}

		crow::json::wvalue makeTotalsJson() const {
			crow::json::wvalue all(crow::json::type::Object);
			for (const auto& [player, bag] : state.resources) {
				crow::json::wvalue bagJson(crow::json::type::Object);
				for (const auto& [kind, quantity] : bag) {
					bagJson[kind] = quantity;
				}
				all["Player " + std::to_string(player)] = std::move(bagJson);
			}
			return all;
		}
	};

}