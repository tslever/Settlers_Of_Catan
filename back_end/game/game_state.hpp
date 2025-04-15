#pragma once

#include "phase.hpp"
#include <string>
#include <unordered_map>
#include <vector>


class GameState {
public:
    int currentPlayer;
    std::string phase;
    std::unordered_map<int, std::vector<std::string>> settlements;
    std::unordered_map<int, std::vector<std::string>> cities;
    std::unordered_map<int, std::vector<std::string>> roads;
    std::string lastBuilding;
    int winner;

    GameState()
        : currentPlayer(1), phase(Game::Phase::TO_PLACE_FIRST_SETTLEMENT), lastBuilding(""), winner(0)
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

    void updatePhase() {
        if (phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
            phase = Game::Phase::TO_PLACE_FIRST_ROAD;
        }
        else if (phase == Game::Phase::TO_PLACE_FIRST_ROAD) {
            if (currentPlayer < 3) {
                currentPlayer++;
                phase = Game::Phase::TO_PLACE_FIRST_SETTLEMENT;
            }
            else {
                phase = Game::Phase::TO_PLACE_FIRST_CITY;
            }
        }
        else if (phase == Game::Phase::TO_PLACE_FIRST_CITY) {
            phase = Game::Phase::TO_PLACE_SECOND_ROAD;
        }
        else if (phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
            if (currentPlayer > 1) {
                currentPlayer--;
                phase = Game::Phase::TO_PLACE_FIRST_CITY;
            }
            else {
                phase = Game::Phase::TURN;
            }
        }
        else if (phase == Game::Phase::TURN) {
            for (int i = 1; i <= 3; i++) {
                int numberOfVictoryPoints = settlements[i].size() + cities[i].size();
                if (numberOfVictoryPoints >= 5) {
                    currentPlayer = i;
                    winner = i;
                    phase = Game::Phase::DONE;
                    return;
                }
            }
            currentPlayer = (currentPlayer % 3) + 1;
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

        crow::json::wvalue settlementsJson(crow::json::type::Object);
        for (const auto& [player, vertices] : settlements) {
            crow::json::wvalue arr(crow::json::type::List);
            int i = 0;
            for (const auto& vertex : vertices) {
                arr[i++] = vertex;
            }
            settlementsJson["Player " + std::to_string(player)] = std::move(arr);
        }
        json["settlements"] = std::move(settlementsJson);

        crow::json::wvalue citiesJson(crow::json::type::Object);
        for (const auto& pair : cities) {
            crow::json::wvalue arr(crow::json::type::List);
            int i = 0;
            for (const auto& vertex : pair.second) {
                arr[i++] = vertex;
            }
            citiesJson["Player " + std::to_string(pair.first)] = std::move(arr);
        }
        json["cities"] = std::move(citiesJson);

        crow::json::wvalue roadsJson(crow::json::type::Object);
        for (const auto& pair : roads) {
            crow::json::wvalue arr(crow::json::type::List);
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