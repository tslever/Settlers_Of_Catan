#pragma once

#include "crow.h"
#include "game_state.hpp"


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(GameState& state) = 0;
};


/* Class PlaceFirstSettlementState is a concrete class
* that represents the state for placing the first settlement.
*/
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For demonstration, we use a hardcoded vertex label.
		// TODO: Replace with AI / board logic.
		std::string chosen_vertex = "V01";
		int player = state.currentPlayer;
		state.placeSettlement(player, chosen_vertex);
		state.lastBuilding = chosen_vertex;
		// Transition to road phase.
		state.phase = "phase to place first road";
		result["message"] = "Player " + std::to_string(player) + " placed a settlement at " + chosen_vertex + ".";
		result["moveType"] = "settlement";
		return result;
	}
};


/* Class PlaceFirstRoadState is a concrete class
* that represents the state for placing the first road.
*/
class PlaceFirstRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded edge.
		// TODO: Replace with AI / board logic.
		std::string chosen_edge = "E01";
		int player = state.currentPlayer;
		state.placeRoad(player, chosen_edge);
		/* Transition logic:
		* If player is Player 1 or Player 2, finish road and move to next player's settlement.
		* If player is Player 3, transition to the city phase.
		*/
		if (player < 3) {
			state.currentPlayer = player + 1;
			state.phase = "phase to place first settlement";
		}
		else {
			state.phase = "phase to place first city";
		}
		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosen_edge + ".";
		result["moveType"] = "road";
		return result;
	}
};


/* Class PlaceFirstCityState is a concrete class
that represents the state for placing the first city. 
*/
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded vertex.
		// TODO: Replace with AI / board logic.
		std::string chosen_vertex = "V02";
		int player = state.currentPlayer;
		state.placeCity(player, chosen_vertex);
		state.lastBuilding = chosen_vertex;
		/* After placing the first city,
		* remain with the same player and move to the second road phase.
		*/
		state.phase = "phase to place second road";
		result["message"] = "Player " + std::to_string(player) + " placed a city at " + chosen_vertex + ".";
		result["moveType"] = "city";
		return result;
	}
};


/* Class PlaceSecondRoadState is a concrete class
* that represents the state for placing the second road
* during the second road in which player order descends.
*/
class PlaceSecondRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded edge.
		// TODO: Replace with AI / board logic.
		std::string chosen_edge = "E02";
		int player = state.currentPlayer;
		state.placeRoad(player, chosen_edge);
		/* If not yet at Player 1, decrement player number and transition to the city phase.
		* Otherwise, if Player 1 finished their city, transition to turn.
		*/
		if (player > 1) {
			state.currentPlayer = player - 1;
			state.phase = "phase to place first city";
		}
		else {
			state.phase = "turn";
		}
		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosen_edge + ".";
		result["moveType"] = "road";
		return result;
	}
};


/* Class TurnState is a concrete class that represents
* the state for when the game has finished the setup and
* the active player takes their turn.
*/
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		int player = state.currentPlayer;
		result["message"] = "Player " + std::to_string(player) + " is taking their turn.";
		result["moveType"] = "turn";
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
		stateHandlers["phase to place first settlement"] = std::make_shared<PlaceFirstSettlementState>();
		stateHandlers["phase to place first road"] = std::make_shared<PlaceFirstRoadState>();
		stateHandlers["phase to place first city"] = std::make_shared<PlaceFirstCityState>();
		stateHandlers["phase to place second road"] = std::make_shared<PlaceSecondRoadState>();
		stateHandlers["turn"] = std::make_shared<TurnState>();
	}

	crow::json::wvalue handle(GameState& state) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};