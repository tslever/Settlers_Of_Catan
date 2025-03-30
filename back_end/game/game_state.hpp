#pragma once

#include <string>
#include <unordered_map>
#include <vector>



/* TODO: Keep or replace the following namespace with an enum class and change phase in `oss << "phase: " << phase << "\n";` below
* to a string representing the enumeration that is lowercase and without underscores as in the strings immediately below.
*/
namespace Phase {
	const std::string TO_PLACE_FIRST_SETTLEMENT = "phase to place first settlement";
	const std::string TO_PLACE_FIRST_ROAD = "phase to place first road";
    const std::string TO_PLACE_FIRST_CITY = "phase to place first city";
	const std::string TO_PLACE_SECOND_ROAD = "phase to place second road";
	const std::string TURN = "turn";
}


class GameState {
public:
    int currentPlayer;
    std::string phase;
    std::unordered_map<int, std::vector<std::string>> settlements;
    std::unordered_map<int, std::vector<std::string>> cities;
    std::unordered_map<int, std::vector<std::string>> roads;
    std::string lastBuilding;

    GameState()
        : currentPlayer(1), phase(Phase::TO_PLACE_FIRST_SETTLEMENT), lastBuilding("")
    {
        settlements[1] = {};
        settlements[2] = {};
        settlements[3] = {};
        cities[1] = {};
        cities[2] = {};
        cities[3] = {};
        roads[1] = {};
        roads[2] = {};
        roads[3] = {};
    }

    // TODO: Consider replacing function `updatePhase` by using the existing phase state machine.
    void updatePhase() {
        if (phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
            phase = Phase::TO_PLACE_FIRST_ROAD;
        }
        else if (phase == Phase::TO_PLACE_FIRST_ROAD) {
            if (currentPlayer < 3) {
                currentPlayer++;
                phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
            }
            else {
                phase = Phase::TO_PLACE_FIRST_CITY;
            }
        }
        else if (phase == Phase::TO_PLACE_FIRST_CITY) {
            phase = Phase::TO_PLACE_SECOND_ROAD;
        }
        else if (phase == Phase::TO_PLACE_SECOND_ROAD) {
            if (currentPlayer > 1) {
                currentPlayer--;
                phase = Phase::TO_PLACE_FIRST_CITY;
            }
            else {
                phase = Phase::TURN;
            }
        }
    }

    /* Add a settlement for the given player at the specified vertex.
    * Transition this game state to the next phase.
    */
	void placeSettlement(int player, const std::string& vertex) {
		settlements[player].push_back(vertex);
		lastBuilding = vertex;
        updatePhase();
	}

    /* Add a city for the given player at the specified vertex.
    * Transition this game state to the next phase.
    */
    void placeCity(int player, const std::string& vertex) {
        cities[player].push_back(vertex);
        lastBuilding = vertex;
        updatePhase();
    }

    /* Add a road for the given player at the specified edge.
    * After placing a road, clear `lastBuilding` so that subsequent moves don't reuse it.
    * Transition this game state to the next phase.
    */
    void placeRoad(int player, const std::string& edge) {
        roads[player].push_back(edge);
        lastBuilding = "";
        updatePhase();
    }

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["currentPlayer"] = currentPlayer;
        json["phase"] = phase;
        json["lastBuilding"] = lastBuilding;

        crow::json::wvalue settlementsJson(crow::json::type::List);
        for (const auto& pair : settlements) {
            crow::json::wvalue arr;
            int i = 0;
            for (const auto& vertex : pair.second) {
                arr[i++] = vertex;
            }
            settlementsJson["Player " + std::to_string(pair.first)] = std::move(arr);
        }
        json["settlements"] = std::move(settlementsJson);

        crow::json::wvalue citiesJson(crow::json::type::List);
        for (const auto& pair : cities) {
            crow::json::wvalue arr;
            int i = 0;
            for (const auto& vertex : pair.second) {
                arr[i++] = vertex;
            }
            citiesJson["Player " + std::to_string(pair.first)] = std::move(arr);
        }
        json["cities"] = std::move(citiesJson);

        crow::json::wvalue roadsJson(crow::json::type::List);
        for (const auto& pair : roads) {
            crow::json::wvalue arr;
            int i = 0;
            for (const auto& edge : pair.second) {
                arr[i++] = edge;
            }
            roadsJson["Player " + std::to_string(pair.first)] = std::move(arr);
        }
        json["roads"] = std::move(roadsJson);

        return json;
    }
};