#pragma once


#include "phase.hpp"
#include <string>
#include <unordered_map>
#include <vector>


struct ResourceBag {
    int brick{ 0 };
    int grain{ 0 };
    int lumber{ 0 };
    int ore{ 0 };
    int wool{ 0 };
    int cloth{ 0 };
    int coin{ 0 };
    int paper{ 0 };
};


class GameState {


public:


    int currentPlayer;
    Game::Phase phase;
    std::unordered_map<int, std::vector<std::string>> settlements;
    std::unordered_map<int, std::vector<std::string>> cities;
    std::unordered_map<int, std::vector<std::string>> roads;
    std::unordered_map<int, std::vector<std::string>> walls;
    std::string lastBuilding;
    int redProductionDie;
    int yellowProductionDie;
    std::string whiteEventDie;
    std::array<ResourceBag, 4> resources;
    int winner;
    

    GameState() :
        currentPlayer(1),
        lastBuilding(""),
        phase(Game::Phase::FirstSettlement),
        redProductionDie(0),
        yellowProductionDie(0),
        whiteEventDie(""),
        winner(0)
    {
        settlements = {
            {1, {}},
            {2, {}},
            {3, {}}
        };
        cities = {
            {1, {}},
            {2, {}},
            {3, {}}
        };
        roads = {
            {1, {}},
            {2, {}},
            {3, {}}
        };
        walls = {
            {1, {}},
            {2, {}},
            {3, {}}
        };
    }
    

    void rollDice() {
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());
        std::uniform_int_distribution<int> distribution(1, 6);
        redProductionDie = distribution(generator);
        yellowProductionDie = distribution(generator);
        int indexOfFaceOfWhiteEventDie = distribution(generator);
        switch (indexOfFaceOfWhiteEventDie) {
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

        collectResources();
    }


    void updatePhase() {
        switch (phase) {
        case Game::Phase::FirstSettlement:
            phase = Game::Phase::FirstRoad;
            break;
        case Game::Phase::FirstRoad:
            if (currentPlayer < 3) {
                ++currentPlayer;
                phase = Game::Phase::FirstSettlement;
            }
            else {
                phase = Game::Phase::FirstCity;
            }
            break;
        case Game::Phase::FirstCity:
            phase = Game::Phase::SecondRoad;
            break;
        case Game::Phase::SecondRoad:
            if (currentPlayer > 1) {
                --currentPlayer;
                phase = Game::Phase::FirstCity;
            }
            else {
                phase = Game::Phase::RollDice;
            }
            break;
        case Game::Phase::RollDice:
            phase = Game::Phase::Turn;
            break;
        case Game::Phase::Turn:
            currentPlayer = (currentPlayer % 3) + 1;
            phase = Game::Phase::RollDice;
            break;
        case Game::Phase::Done:
            break;
        }
    }


    void placeSettlement(int player, const std::string& vertex) {
		bool isMainTurn = (phase == Game::Phase::Turn);
        if (isMainTurn) {
            auto& bag = resources[player];
            bag.brick--;
            bag.grain--;
            bag.lumber--;
            bag.wool--;
        }
        settlements[player].push_back(vertex);
        lastBuilding = vertex;
        if (checkForWinner()) {
            return;
        }
        if (!isMainTurn) {
            updatePhase();
        }
    }


    void placeCity(int player, const std::string& vertex) {
		bool isMainTurn = (phase == Game::Phase::Turn);
        if (isMainTurn) {
            auto& bag = resources[player];
            bag.grain -= 2;
            bag.ore -= 3;
            std::vector<std::string>& vectorOfLabelsOfSettlementsOfPlayer = settlements[player];
			vectorOfLabelsOfSettlementsOfPlayer.erase(
                std::remove(
                    vectorOfLabelsOfSettlementsOfPlayer.begin(),
                    vectorOfLabelsOfSettlementsOfPlayer.end(),
                    vertex
                ),
                vectorOfLabelsOfSettlementsOfPlayer.end()
            );
        }
		cities[player].push_back(vertex);

        lastBuilding = vertex;
        if (checkForWinner()) {
            return;
        }
        updatePhase();
    }


    bool placeCityWall(int player, const std::string& vertex) {
        bool isMainTurn = (phase == Game::Phase::Turn);
        if (isMainTurn) {
            resources[player].brick -= 2;
        }
        auto& playerWalls = walls[player];
		if (std::find(playerWalls.begin(), playerWalls.end(), vertex) == playerWalls.end()) {
            playerWalls.push_back(vertex);
            lastBuilding = vertex;
            return true;
		}
        return false;
    }


    void placeRoad(int player, const std::string& labelOfEdge) {
		bool isMainTurn = (phase == Game::Phase::Turn);
        if (isMainTurn) {
            auto& bag = resources[player];
            bag.brick--;
            bag.lumber--;
        }
        roads[player].push_back(labelOfEdge);
        lastBuilding = "";
        if (!isMainTurn) {
            updatePhase();
        }
    }


    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["currentPlayer"] = currentPlayer;
        json["phase"] = Game::toString(phase);
        json["lastBuilding"] = lastBuilding;

        crow::json::wvalue resourcesJson(crow::json::type::Object);
        for (int player = 1; player <= 3; ++player) {
            const auto& bag = resources[player];
            crow::json::wvalue bagJson(crow::json::type::Object);
            bagJson["brick"] = bag.brick;
            bagJson["grain"] = bag.grain;
            bagJson["lumber"] = bag.lumber;
            bagJson["ore"] = bag.ore;
            bagJson["wool"] = bag.wool;
            bagJson["cloth"] = bag.cloth;
            bagJson["coin"] = bag.coin;
            bagJson["paper"] = bag.paper;
            resourcesJson["Player " + std::to_string(player)] = std::move(bagJson);
        }
		json["resources"] = std::move(resourcesJson);

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

        crow::json::wvalue wallsJson(crow::json::type::Object);
        for (const auto& pairOfNumberOfPlayerAndArrayOfLabelsOfVerticesWithWalls : walls) {
            int numberOfPlayer = pairOfNumberOfPlayerAndArrayOfLabelsOfVerticesWithWalls.first;
            std::vector<std::string> vectorOfLabelsOfVerticesWithWalls = pairOfNumberOfPlayerAndArrayOfLabelsOfVerticesWithWalls.second;
            crow::json::wvalue arr(crow::json::type::List);
            int i = 0;
            for (auto& labelOfVertexWithWall : vectorOfLabelsOfVerticesWithWalls) {
                arr[i++] = labelOfVertexWithWall;
            }
            wallsJson["Player " + std::to_string(numberOfPlayer)] = std::move(arr);
        }
        json["walls"] = std::move(wallsJson);

		crow::json::wvalue jsonObjectOfDescriptionOfDiceAndRolls(crow::json::type::Object);
        jsonObjectOfDescriptionOfDiceAndRolls["yellowProductionDie"] = yellowProductionDie;
        jsonObjectOfDescriptionOfDiceAndRolls["redProductionDie"] = redProductionDie;
        jsonObjectOfDescriptionOfDiceAndRolls["whiteEventDie"] = whiteEventDie;
		json["dice"] = std::move(jsonObjectOfDescriptionOfDiceAndRolls);

        return json;
    }


private:


	inline static crow::json::rvalue isometricCoordinatesCache;
	inline static std::once_flag onceFlag;


    static void loadIsometricCoordinates() {
		std::call_once(onceFlag, [] {
            std::ifstream f("../generate_board_geometry/isometric_coordinates.json");
			if (!f.is_open()) {
				throw std::runtime_error("Isometric coordinates file could not be opened.");
			}
			std::stringstream buffer;
			buffer << f.rdbuf();
			isometricCoordinatesCache = crow::json::load(buffer.str());
			if (!isometricCoordinatesCache) {
				throw std::runtime_error("Isometric coordinates file could not be parsed.");
			}
		});
    }


    bool checkForWinner() {
        for (int player = 1; player <= 3; player++) {
			int numberOfBuildings = static_cast<int>(settlements[player].size() + cities[player].size());
            if (numberOfBuildings >= 5) {
                winner = player;
                phase = Game::Phase::Done;
                return true;
            }
        }
        return false;
    }


    void collectResources() {
		const int rolledNumber = redProductionDie + yellowProductionDie;
        loadIsometricCoordinates();

        const crow::json::rvalue& jsonObjectOfIdsOfHexesAndNamesOfResources = isometricCoordinatesCache["objectOfIdsOfHexesAndNamesOfResources"];
		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndNumbersOfTokens = isometricCoordinatesCache["objectOfIdsOfHexesAndNumbersOfTokens"];
        const crow::json::rvalue& jsonObjectOfIdsOfHexesAndArraysOfLabelsOfVertices = isometricCoordinatesCache["objectOfIdsOfHexesAndArraysOfIdsOfVertices"];

        for (const crow::json::rvalue& jsonObjectWithNumberOfToken : jsonObjectOfIdsOfHexesAndNumbersOfTokens) {
			std::string idOfHex = jsonObjectWithNumberOfToken.key();
			int numberOfToken = static_cast<int>(jsonObjectWithNumberOfToken.d());
			if (numberOfToken != rolledNumber) {
				continue;
			}
			std::string resource = jsonObjectOfIdsOfHexesAndNamesOfResources[idOfHex].s();
			if (resource == "nothing") {
				continue;
			}
            const crow::json::rvalue& jsonArrayOfLabelsOfVertices = jsonObjectOfIdsOfHexesAndArraysOfLabelsOfVertices[idOfHex];
            for (int player = 1; player <= 3; ++player) {
                auto& bag = resources[player];
                for (const std::string& labelOfVertex : settlements[player]) {
                    for (const crow::json::rvalue& keyValuePair : jsonArrayOfLabelsOfVertices) {
                        if (keyValuePair.s() == labelOfVertex) {
                            if (resource == "brick") { bag.brick++; }
                            else if (resource == "grain") { bag.grain++; }
                            else if (resource == "lumber") { bag.lumber++; }
                            else if (resource == "ore") { bag.ore++; }
                            else if (resource == "wool") { bag.wool++; }
                            else if (resource == "cloth") { bag.cloth++; }
                            else if (resource == "coin") { bag.coin++; }
                            else if (resource == "paper") { bag.paper++; }
                            break;
                        }
                    }
                }
                for (const std::string& labelOfVertex : cities[player]) {
                    for (const crow::json::rvalue& keyValuePair : jsonArrayOfLabelsOfVertices) {
                        if (keyValuePair.s() == labelOfVertex) {
                            if (resource == "brick") {
                                bag.brick += 2;
                            }
                            else if (resource == "grain") {
                                bag.grain += 2;
                            }
                            else if (resource == "lumber") {
                                bag.lumber += 1;
                                bag.paper += 1;
                            }
                            else if (resource == "ore") {
                                bag.ore += 1;
                                bag.coin += 1;
                            }
                            else if (resource == "wool") {
                                bag.wool += 1;
                                bag.cloth += 1;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

};