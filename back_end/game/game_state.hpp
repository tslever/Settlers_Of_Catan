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
    int redProductionDie;
    std::string whiteEventDie;
    int winner;
    int yellowProductionDie;

    GameState() :
        currentPlayer(1),
        lastBuilding(""),
        phase(Game::Phase::TO_PLACE_FIRST_SETTLEMENT),
        redProductionDie(0),
        whiteEventDie(""),
        winner(0),
        yellowProductionDie(0)
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


    void rollDice() {
        std::random_device randomDevice;
		std::mt19937 generator(randomDevice());
		std::uniform_int_distribution<int> distribution(1, 6);
		redProductionDie = distribution(generator);
		yellowProductionDie = distribution(generator);
        int valueOfRoll = distribution(generator);
        switch (valueOfRoll) {
		case 1:
			whiteEventDie = "yellow";
			break;
        case 2:
			whiteEventDie = "green";
            break;
        case 3:
			whiteEventDie = "blue";
			break;
        default:
			whiteEventDie = "black";
            break;
        }
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
            rollDice();
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


	void placeSettlement(int player, const std::string& vertex) {
		settlements[player].push_back(vertex);
		lastBuilding = vertex;
        updatePhase();
	}


    void placeCity(int player, const std::string& vertex) {
        cities[player].push_back(vertex);
        lastBuilding = vertex;
        updatePhase();
    }


    // TODO: Rename to placeRoad.
	void placeRoad(int player, const std::string& labelOfEdge) {
		roads[player].push_back(labelOfEdge);
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

		crow::json::wvalue jsonObjectOfDescriptionOfDiceAndRolls(crow::json::type::Object);
        jsonObjectOfDescriptionOfDiceAndRolls["yellowProductionDie"] = yellowProductionDie;
        jsonObjectOfDescriptionOfDiceAndRolls["redProductionDie"] = redProductionDie;
        jsonObjectOfDescriptionOfDiceAndRolls["whiteEventDie"] = whiteEventDie;
		json["dice"] = std::move(jsonObjectOfDescriptionOfDiceAndRolls);

        return json;
    }
};