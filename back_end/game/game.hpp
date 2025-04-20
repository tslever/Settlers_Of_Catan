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
			jsonObjectOfMoveInformation["moveType"] = "settlement";
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
			jsonObjectOfMoveInformation["moveType"] = "road";
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
			jsonObjectOfMoveInformation["moveType"] = "city";
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
			jsonObjectOfMoveInformation["moveType"] = "road";
			crow::json::wvalue jsonObjectOfRoadInformation;
			jsonObjectOfRoadInformation["id"] = roadId;
			jsonObjectOfRoadInformation["player"] = currentPlayer;
			jsonObjectOfRoadInformation["edge"] = labelOfChosenEdge;
			jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
			return jsonObjectOfMoveInformation;
		}

		crow::json::wvalue handleTurn() {
			crow::json::wvalue jsonObjectOfMoveInformation;

			state.rollDice();
			crow::json::wvalue jsonObjectOfDescriptionOfDiceAndRolls;
			jsonObjectOfDescriptionOfDiceAndRolls["yellowProductionDie"] = state.yellowProductionDie;
			jsonObjectOfDescriptionOfDiceAndRolls["redProductionDie"] = state.redProductionDie;
			jsonObjectOfDescriptionOfDiceAndRolls["whiteEventDie"] = state.whiteEventDie;
			jsonObjectOfMoveInformation["dice"] = std::move(jsonObjectOfDescriptionOfDiceAndRolls);

			std::pair<std::string, int> pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance,
				dirichletMixingWeight,
				dirichletShape
			);
			std::string labelOfChosenVertexOrEdge = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.first;
			if (labelOfChosenVertexOrEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			Board board;
			if (board.isLabelOfVertex(labelOfChosenVertexOrEdge)) {
				state.placeSettlement(currentPlayer, labelOfChosenVertexOrEdge);
				int settlementId = db.addStructure("settlements", currentPlayer, labelOfChosenVertexOrEdge, "vertex");
				jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + labelOfChosenVertexOrEdge + ".";
				jsonObjectOfMoveInformation["moveType"] = "turn";
				crow::json::wvalue jsonObjectOfSettlementInformation;
				jsonObjectOfSettlementInformation["id"] = settlementId;
				jsonObjectOfSettlementInformation["player"] = currentPlayer;
				jsonObjectOfSettlementInformation["vertex"] = labelOfChosenVertexOrEdge;
				jsonObjectOfMoveInformation["settlement"] = std::move(jsonObjectOfSettlementInformation);
			}
			else if (board.isLabelOfEdge(labelOfChosenVertexOrEdge)) {
				state.placeRoad(currentPlayer, labelOfChosenVertexOrEdge);
				int roadId = db.addStructure("roads", currentPlayer, labelOfChosenVertexOrEdge, "edge");
				jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + labelOfChosenVertexOrEdge + ".";
				jsonObjectOfMoveInformation["moveType"] = "turn";
				crow::json::wvalue jsonObjectOfRoadInformation;
				jsonObjectOfRoadInformation["id"] = roadId;
				jsonObjectOfRoadInformation["player"] = currentPlayer;
				jsonObjectOfRoadInformation["edge"] = labelOfChosenVertexOrEdge;
				jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
			}
			return jsonObjectOfMoveInformation;
		}

		crow::json::wvalue handleEnd() {
			crow::json::wvalue jsonObjectOfEndInformation;
			jsonObjectOfEndInformation["message"] = "Game is over. Thanks for playing!";
			return jsonObjectOfEndInformation;
		}
	};

}