#pragma once

#include "board.hpp"
#include "crow.h"
#include "game_state.hpp"
#include "../ai/strategy.hpp"
#include <random>


/* TODO: Ensure that the various phase state classes for placing settlements, roads, and cities use Monte Carlo Tree Search
* so that all move decisions are driven by AI logic.
*/


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) = 0;
};

/* Class PlaceFirstSettlementState is a concrete class
* that represents a handler of the phase / state to place the first settlement.
*/
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string chosenVertex = mctsResult.first;
		state.placeSettlement(currentPlayer, chosenVertex);

		state.phase = Phase::TO_PLACE_FIRST_ROAD;
		int settlementId = db.addSettlement(currentPlayer, chosenVertex);

		result["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + chosenVertex + ".";
		result["moveType"] = "settlement";
		crow::json::wvalue settlementJson;
		settlementJson["id"] = settlementId;
		settlementJson["player"] = currentPlayer;
		settlementJson["vertex"] = chosenVertex;
		result["settlement"] = std::move(settlementJson);

		return result;
	}
};


/* Class PlaceFirstRoadState is a concrete class
* that represents a handler of the phase / state to place the first road.
*/
class PlaceFirstRoadState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string keyOfChosenEdge = mctsResult.first;
		state.placeRoad(currentPlayer, keyOfChosenEdge);
		state.lastBuilding = "";

		// For players 1 and 2, move to next settlement; for player 3, transition to city.
		if (currentPlayer < 3) {
			state.currentPlayer = currentPlayer + 1;
			state.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
		}
		else {
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		int roadId = db.addRoad(currentPlayer, keyOfChosenEdge);

		result["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + keyOfChosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = currentPlayer;
		roadJson["edge"] = keyOfChosenEdge;
		result["road"] = std::move(roadJson);

		return result;
	}
};

/* Class PlaceFirstCityState is a concrete class
* that represents a handler of the phase / state to place the first city.
*/
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string chosenVertex = mctsResult.first;
		state.placeCity(currentPlayer, chosenVertex);

		state.phase = Phase::TO_PLACE_SECOND_ROAD;
		int cityId = db.addCity(currentPlayer, chosenVertex);

		result["message"] = "Player " + std::to_string(currentPlayer) + " placed a city at " + chosenVertex + ".";
		result["moveType"] = "city";
		crow::json::wvalue cityJson;
		cityJson["id"] = cityId;
		cityJson["player"] = currentPlayer;
		cityJson["vertex"] = chosenVertex;
		result["city"] = std::move(cityJson);

		return result;
	}
};

/* Class PlaceSecondRoadState is a concrete class
* that represents a handler of the phase / state to place the second road.
*/
class PlaceSecondRoadState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int player = state.currentPlayer;
		std::string keyOfChosenEdge = mctsResult.first;
		state.placeRoad(player, keyOfChosenEdge);
		state.lastBuilding = "";
		if (player > 1) {
			state.currentPlayer = player - 1;
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		else {
			state.phase = Phase::TURN;
		}
		int roadId = db.addRoad(player, keyOfChosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + keyOfChosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = keyOfChosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};

// Class PlaceFirstCityState is a concrete class that represents a handler of a turn.
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string chosenLabelOfVertexOrEdgeKey = mctsResult.first;
		if (isLabelOfVertex(chosenLabelOfVertexOrEdgeKey)) {
			state.placeSettlement(currentPlayer, chosenLabelOfVertexOrEdgeKey);
			int settlementId = db.addSettlement(currentPlayer, chosenLabelOfVertexOrEdgeKey);
			result["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + chosenLabelOfVertexOrEdgeKey + ".";
			result["moveType"] = "turn";
			crow::json::wvalue settlementJson;
			settlementJson["id"] = settlementId;
			settlementJson["player"] = currentPlayer;
			settlementJson["vertex"] = chosenLabelOfVertexOrEdgeKey;
			result["settlement"] = std::move(settlementJson);
		}
		else if (isEdgeKey(chosenLabelOfVertexOrEdgeKey)) {
			state.placeRoad(currentPlayer, chosenLabelOfVertexOrEdgeKey);
			int roadId = db.addRoad(currentPlayer, chosenLabelOfVertexOrEdgeKey);
			result["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + chosenLabelOfVertexOrEdgeKey + ".";
			result["moveType"] = "turn";
			crow::json::wvalue roadJson;
			roadJson["id"] = roadId;
			roadJson["player"] = currentPlayer;
			roadJson["vertex"] = chosenLabelOfVertexOrEdgeKey;
			result["road"] = std::move(roadJson);
		}
		return result;
	}
};

class DoneState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;
		result["message"] = "Game over. Thanks for playing!";
		return result;
	}
};

/* Class PhaseStateMachine represents a phase state machine
* that dispatches the current phase to the appropriate handler.
*/
class PhaseStateMachine {
private:
	std::unordered_map<std::string, std::shared_ptr<PhaseState>> stateHandlers;
public:
	PhaseStateMachine() {
		stateHandlers[Phase::TO_PLACE_FIRST_SETTLEMENT] = std::make_shared<PlaceFirstSettlementState>();
		stateHandlers[Phase::TO_PLACE_FIRST_ROAD] = std::make_shared<PlaceFirstRoadState>();
		stateHandlers[Phase::TO_PLACE_FIRST_CITY] = std::make_shared<PlaceFirstCityState>();
		stateHandlers[Phase::TO_PLACE_SECOND_ROAD] = std::make_shared<PlaceSecondRoadState>();
		stateHandlers[Phase::TURN] = std::make_shared<TurnState>();
		stateHandlers[Phase::DONE] = std::make_shared<DoneState>();
	}

	crow::json::wvalue handle(
		GameState& state,
		DB::Database& db,
		AI::WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};