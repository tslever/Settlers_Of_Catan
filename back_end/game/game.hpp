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
			const GameState& gameStateToUse
		) : db(dbToUse),
			wrapperOfNeuralNetwork(wrapperOfNeuralNetworkToUse),
			numberOfSimulations(numberOfSimulationsToUse),
			cPuct(cPuctToUse),
			tolerance(toleranceToUse),
			state(gameStateToUse)
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

		crow::json::wvalue handleFirstSettlement() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestVertexAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance
			);
			std::string labelOfChosenVertex = pairOfLabelOfBestVertexAndVisitCount.first;
			if (labelOfChosenVertex.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeSettlement(currentPlayer, labelOfChosenVertex);
			int settlementId = db.addSettlement(currentPlayer, labelOfChosenVertex);
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
			std::pair<std::string, int> pairOfKeyOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance
			);
			std::string keyOfChosenEdge = pairOfKeyOfBestEdgeAndVisitCount.first;
			if (keyOfChosenEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeRoad(currentPlayer, keyOfChosenEdge);
			int roadId = db.addRoad(currentPlayer, keyOfChosenEdge);
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + keyOfChosenEdge + ".";
			jsonObjectOfMoveInformation["moveType"] = "road";
			crow::json::wvalue jsonObjectOfRoadInformation;
			jsonObjectOfRoadInformation["id"] = roadId;
			jsonObjectOfRoadInformation["player"] = currentPlayer;
			jsonObjectOfRoadInformation["edge"] = keyOfChosenEdge;
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
				tolerance
			);
			std::string labelOfChosenVertex = pairOfLabelOfBestVertexAndVisitCount.first;
			if (labelOfChosenVertex.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeCity(currentPlayer, labelOfChosenVertex);
			state.phase = Phase::TO_PLACE_SECOND_ROAD;
			int cityId = db.addCity(currentPlayer, labelOfChosenVertex);
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
			std::pair<std::string, int> pairOfKeyOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance
			);
			std::string keyOfChosenEdge = pairOfKeyOfBestEdgeAndVisitCount.first;
			if (keyOfChosenEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			state.placeRoad(currentPlayer, keyOfChosenEdge);
			int roadId = db.addRoad(currentPlayer, keyOfChosenEdge);
			jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + keyOfChosenEdge + ".";
			jsonObjectOfMoveInformation["moveType"] = "road";
			crow::json::wvalue jsonObjectOfRoadInformation;
			jsonObjectOfRoadInformation["id"] = roadId;
			jsonObjectOfRoadInformation["player"] = currentPlayer;
			jsonObjectOfRoadInformation["edge"] = keyOfChosenEdge;
			jsonObjectOfMoveInformation["road"] = std::move(jsonObjectOfRoadInformation);
			return jsonObjectOfMoveInformation;
		}

		crow::json::wvalue handleTurn() {
			crow::json::wvalue jsonObjectOfMoveInformation;
			std::pair<std::string, int> pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount = runMcts(
				state,
				wrapperOfNeuralNetwork,
				numberOfSimulations,
				cPuct,
				tolerance
			);
			std::string representationOfChosenVertexOrEdge = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.first;
			if (representationOfChosenVertexOrEdge.empty()) {
				throw std::runtime_error("MCTS failed to determine a move.");
			}
			int currentPlayer = state.currentPlayer;
			Board board;
			if (board.isLabelOfVertex(representationOfChosenVertexOrEdge)) {
				state.placeSettlement(currentPlayer, representationOfChosenVertexOrEdge);
				int settlementId = db.addSettlement(currentPlayer, representationOfChosenVertexOrEdge);
				jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + representationOfChosenVertexOrEdge + ".";
				jsonObjectOfMoveInformation["moveType"] = "turn";
				crow::json::wvalue jsonObjectOfSettlementInformation;
				jsonObjectOfSettlementInformation["id"] = settlementId;
				jsonObjectOfSettlementInformation["player"] = currentPlayer;
				jsonObjectOfSettlementInformation["vertex"] = representationOfChosenVertexOrEdge;
				jsonObjectOfMoveInformation["settlement"] = std::move(jsonObjectOfSettlementInformation);
			}
			else if (board.isEdgeKey(representationOfChosenVertexOrEdge)) {
				state.placeRoad(currentPlayer, representationOfChosenVertexOrEdge);
				int roadId = db.addRoad(currentPlayer, representationOfChosenVertexOrEdge);
				jsonObjectOfMoveInformation["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + representationOfChosenVertexOrEdge + ".";
				jsonObjectOfMoveInformation["moveType"] = "turn";
				crow::json::wvalue jsonObjectOfRoadInformation;
				jsonObjectOfRoadInformation["id"] = roadId;
				jsonObjectOfRoadInformation["player"] = currentPlayer;
				jsonObjectOfRoadInformation["edge"] = representationOfChosenVertexOrEdge;
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