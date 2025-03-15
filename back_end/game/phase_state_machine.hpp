#pragma once

#include "crow.h"
#include "game_state.hpp"
#include <random>


// Function `getOccupiedVertices` gets a list of occupied vertex labels by querying the database.
std::vector<std::string> getOccupiedVertices(Database& db) {
	std::vector<std::string> occupied;
	auto settlements = db.getSettlements();
	for (const auto& s : settlements) {
		occupied.push_back(s.vertex);
	}
	auto cities = db.getCities();
	for (const auto& c : cities) {
		occupied.push_back(c.vertex);
	}
	return occupied;
}


/* Function `getAvailableVertices` gets the available vertices given a list of already occupied vertices.
* `getAvailableVertices` reads the board geometry and filters out any vertex that is either occupied
* or is too close (i.e. adjacent) to any occupied vertex.
*/
std::vector<std::string> getAvailableVertices(const std::vector<std::string>& occupied) {
	std::ifstream file("../board_geometry.json");
	if (!file.is_open()) {
		throw std::runtime_error("Board geometry file could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	auto boardGeometry = crow::json::load(buffer.str());
	if (!boardGeometry) {
		throw std::runtime_error("Board geometry file could not be parsed.");
	}
	auto verticesJson = boardGeometry["vertices"];
	if (!verticesJson || verticesJson.size() == 0) {
		throw std::runtime_error("Board geometry file does not contain vertices.");
	}

	// Build a map of vertex label to its (x,y) coordinates.
	std::unordered_map<std::string, std::pair<double, double>> vertexCoords;
	for (size_t i = 0; i < verticesJson.size(); i++) {
		auto v = verticesJson[i];
		std::string label = v["label"].s();
		double x = v["x"].d();
		double y = v["y"].d();
		vertexCoords[label] = { x, y };
	}

	// Set up board constants.
	double width_of_board_in_vmin = 100.0;
	double number_of_hexes_that_span_board = 6.0;
	double width_of_hex = width_of_board_in_vmin / number_of_hexes_that_span_board;
	double length_of_side_of_hex = width_of_hex * std::tan(M_PI / 6);
	double margin_of_error = 0.01;

	std::vector<std::string> available;
	// For each vertex in the board geometry:
	for (const auto& pair : vertexCoords) {
		const std::string& label = pair.first;
		double x1 = pair.second.first;
		double y1 = pair.second.second;

		// Skip if the vertex is already occupied.
		if (std::find(occupied.begin(), occupied.end(), label) != occupied.end())
			continue;

		bool tooClose = false;
		// Check the distance to every occupied vertex.
		for (const auto& occLabel : occupied) {
			auto it = vertexCoords.find(occLabel);
			if (it != vertexCoords.end()) {
				double x2 = it->second.first;
				double y2 = it->second.second;
				double distance = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
				// If the vertex is within or exactly at the forbidden distance, mark as too close.
				if (distance <= length_of_side_of_hex + margin_of_error) {
					tooClose = true;
					break;
				}
			}
		}
		if (!tooClose) {
			available.push_back(label);
		}
	}
	return available;
}


std::string getRandomEdgeKeyAdjacentTo(const std::string& lastBuilding) {
	std::ifstream file("../board_geometry.json");
	if (!file.is_open()) {
		throw std::runtime_error("Board geometry file could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	auto jsonVal = crow::json::load(buffer.str());
	if (!jsonVal) {
		throw std::runtime_error("Board geometry file could not be parsed.");
	}

	// Look up vertex with label equal to given vertex of last building.
	auto vertices = jsonVal["vertices"];
	if (!vertices || vertices.size() == 0) {
		throw std::runtime_error("Board geometry file does not contain vertices.");
	}
	double last_x = 0.0;
	double last_y = 0.0;
	bool found = false;
	for (size_t i = 0; i < vertices.size(); i++) {
		auto vertex = vertices[i];
		std::string label = vertex["label"].s();
		if (label == lastBuilding) {
			last_x = vertex["x"].d();
			last_y = vertex["y"].d();
			found = true;
			break;
		}
	}
	if (!found) {
		throw std::runtime_error("Last building vertex was not found in board geometry.");
	}

	// Filter edges that are adjacent to vertex with label of last building.
	auto edges = jsonVal["edges"];
	if (!edges || edges.size() == 0) {
		throw std::runtime_error("Board geometry file does not contain edges.");
	}
	std::vector<crow::json::rvalue> adjacentEdges;
	const double tolerance = 1e-2;
	for (size_t i = 0; i < edges.size(); i++) {
		auto edge = edges[i];
		double x1 = edge["x1"].d();
		double y1 = edge["y1"].d();
		double x2 = edge["x2"].d();
		double y2 = edge["y2"].d();
		if ((std::abs(x1 - last_x) < tolerance && std::abs(y1 - last_y) < tolerance) ||
			(std::abs(x2 - last_x) < tolerance && std::abs(y2 - last_y) < tolerance)) {
			adjacentEdges.push_back(edge);
		}
	}
	if (adjacentEdges.empty()) {
		throw std::runtime_error("No edges are adjacent to last building found.");
	}

	// Randomly select one of the adjacent edges.
	/* TODO: Let an edge terminate at vertex with label `lastBuilding` and vertex with label `label_of_vertex_opposite_last_building`.
	* Chose the edge with the strongest vertex with label `label_of_vertex_opposite_last_building`.
	*/
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<> dist(0, adjacentEdges.size() - 1);
	int index = dist(rng);
	auto chosenEdge = adjacentEdges[index];

	double x1 = chosenEdge["x1"].d();
	double y1 = chosenEdge["y1"].d();
	double x2 = chosenEdge["x2"].d();
	double y2 = chosenEdge["y2"].d();

	// Format the edge key so that the order of endpoints is consistent.
	char buf[50];
	if ((x1 < x2) || (std::abs(x1 - x2) < tolerance && y1 <= y2)) {
		std::snprintf(buf, sizeof(buf), "%.2f-%.2f_%.2f-%.2f", x1, y1, x2, y2);
	}
	else {
		std::snprintf(buf, sizeof(buf), "%.2f-%.2f_%.2f-%.2f", x2, y2, x1, y1);
	}
	return std::string(buf);
}


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(GameState& state, Database& db) = 0;
};


/* Class PlaceFirstSettlementState is a concrete class
* that represents a handler of the phase / state to place the first settlement.
*/
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		// Get the up-to-date list of occupied vertices from the database.
		std::vector<std::string> occupied = getOccupiedVertices(db);

		// Determine which vertices are available (not occupied and not adjacent to any occupied vertex).
		auto vectorOfLabelsOfAvailableVertices = getAvailableVertices(occupied);
		if (vectorOfLabelsOfAvailableVertices.empty()) {
			throw std::runtime_error("No vertices are available for placing settlement.");
		}

		// Randomly select one available vertex.
		// TODO: Replace with AI / board logic.
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<> dist(0, vectorOfLabelsOfAvailableVertices.size() - 1);
		std::string chosenVertex = vectorOfLabelsOfAvailableVertices.at(dist(rng));

		int player = state.currentPlayer;
		state.placeSettlement(player, chosenVertex);
		// Transition to road phase.
		state.phase = Phase::TO_PLACE_FIRST_ROAD;
		// Persist settlement in database.
		int settlementId = db.addSettlement(player, chosenVertex);

		result["message"] = "Player " + std::to_string(player) + " placed a settlement at " + chosenVertex + ".";
		result["moveType"] = "settlement";
		crow::json::wvalue settlementJson;
		settlementJson["id"] = settlementId;
		settlementJson["player"] = player;
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
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		if (state.lastBuilding.empty()) {
			throw std::runtime_error("No last building was set for road placement.");
		}
		std::string chosenEdge = getRandomEdgeKeyAdjacentTo(state.lastBuilding);

		int player = state.currentPlayer;
		state.placeRoad(player, chosenEdge);
		state.lastBuilding = "";
		// For players 1 and 2, move to next settlement; for player 3, transition to city.
		if (player < 3) {
			state.currentPlayer = player + 1;
			state.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
		}
		else {
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		int roadId = db.addRoad(player, chosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = chosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};


/* Class PlaceFirstCityState is a concrete class
* that represents a handler of the phase / state to place the first city.
*/
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		// Get the up-to-date list of occupied vertices from the database.
		std::vector<std::string> occupied = getOccupiedVertices(db);

		// Determine which vertices are available (not occupied and not adjacent to any occupied vertex).
		auto vectorOfLabelsOfAvailableVertices = getAvailableVertices(occupied);
		if (vectorOfLabelsOfAvailableVertices.empty()) {
			throw std::runtime_error("No vertices are available for placing settlement.");
		}

		// Randomly select one available vertex.
		// TODO: Replace with AI / board logic.
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<> dist(0, vectorOfLabelsOfAvailableVertices.size() - 1);
		std::string chosenVertex = vectorOfLabelsOfAvailableVertices.at(dist(rng));

		int player = state.currentPlayer;
		state.placeCity(player, chosenVertex);
		// Transition to the second road phase.
		state.phase = Phase::TO_PLACE_SECOND_ROAD;
		// Persist the city in the database.
		int cityId = db.addCity(player, chosenVertex);

		result["message"] = "Player " + std::to_string(player) + " placed a city at " + chosenVertex + ".";
		result["moveType"] = "city";
		crow::json::wvalue cityJson;
		cityJson["id"] = cityId;
		cityJson["player"] = player;
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
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		if (state.lastBuilding.empty()) {
			throw std::runtime_error("No last building was set for road placement.");
		}
		std::string chosenEdge = getRandomEdgeKeyAdjacentTo(state.lastBuilding);

		int player = state.currentPlayer;
		state.placeRoad(player, chosenEdge);
		state.lastBuilding = "";
		/* Transition logic:
		* If player number is greater than 1, decrement player number and reset phase for city.
		* If player number is 1, transition to turn.
		*/
		if (player > 1) {
			state.currentPlayer = player - 1;
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		else {
			state.phase = Phase::TURN;
		}
		int roadId = db.addRoad(player, chosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = chosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};


// Class PlaceFirstCityState is a concrete class that represents a handler of a turn.
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
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
		stateHandlers[Phase::TO_PLACE_FIRST_SETTLEMENT] = std::make_shared<PlaceFirstSettlementState>();
		stateHandlers[Phase::TO_PLACE_FIRST_ROAD] = std::make_shared<PlaceFirstRoadState>();
		stateHandlers[Phase::TO_PLACE_FIRST_CITY] = std::make_shared<PlaceFirstCityState>();
		stateHandlers[Phase::TO_PLACE_SECOND_ROAD] = std::make_shared<PlaceSecondRoadState>();
		stateHandlers[Phase::TURN] = std::make_shared<TurnState>();
	}

	crow::json::wvalue handle(GameState& state, Database& db) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state, db);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};