#pragma once


#include "../game/board.hpp"
#include "crow.h"
#include "../db/database.hpp"


namespace Server {


	void buildNextMoves(DB::Database liveDb, crow::json::wvalue& response) {
		GameState nextState = liveDb.getGameState();
		const int nextPlayer = nextState.currentPlayer;
		const Game::Phase nextPhase = nextState.phase;
		Board board;
		std::unordered_map<std::string, std::vector<std::string>> unorderedMapOfLabelsOfVerticesAndMoveTypes;
		std::vector<std::string> vectorOfLabelsOfEdges;
		bool nextPlayerWillRollDice = false;
		std::vector<std::string> vectorOfLabelsOfOccupiedVertices = board.getVectorOfLabelsOfOccupiedVertices(nextState);
		std::vector<std::string> vectorOfLabelsOfOccupiedEdges = board.getVectorOfLabelsOfOccupiedEdges(nextState);
		std::vector<std::string> vectorOfLabelsOfAvailableVertices = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);

		if (nextPhase == Game::Phase::FirstSettlement) {
			for (auto& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
				unorderedMapOfLabelsOfVerticesAndMoveTypes[labelOfAvailableVertex].push_back("settlement");
			}
		}
		else if (nextPhase == Game::Phase::FirstRoad) {
			auto adjacentEdges = board.getVectorOfLabelsOfAvailableEdgesExtendingFromLastBuilding(nextState.lastBuilding, vectorOfLabelsOfOccupiedEdges);
			vectorOfLabelsOfEdges.insert(vectorOfLabelsOfEdges.end(), adjacentEdges.begin(), adjacentEdges.end());
		}
		else if (nextPhase == Game::Phase::FirstCity) {
			for (auto& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
				unorderedMapOfLabelsOfVerticesAndMoveTypes[labelOfAvailableVertex].push_back("city");
			}
		}
		else if (nextPhase == Game::Phase::SecondRoad) {
			auto adjacentEdges = board.getVectorOfLabelsOfAvailableEdgesExtendingFromLastBuilding(nextState.lastBuilding, vectorOfLabelsOfOccupiedEdges);
			vectorOfLabelsOfEdges.insert(vectorOfLabelsOfEdges.end(), adjacentEdges.begin(), adjacentEdges.end());
		}
		else if (nextPhase == Game::Phase::RollDice) {
			nextPlayerWillRollDice = true;
		}
		else if (nextPhase == Game::Phase::Turn) {
			const ResourceBag& resources = nextState.resources[nextPlayer];
			std::unordered_set<std::string> unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer;
			std::vector<std::string>& vectorOfLabelsOfEdgesWithRoadsOfPlayer = nextState.roads.at(nextPlayer);
			for (const std::string& labelOfEdge : vectorOfLabelsOfEdgesWithRoadsOfPlayer) {
				auto [firstLabelOfVertex, secondLabelOfVertex] = board.getVerticesOfEdge(labelOfEdge);
				unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(firstLabelOfVertex);
				unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(secondLabelOfVertex);
			}
			for (std::string& labelOfAvailableVertex : vectorOfLabelsOfAvailableVertices) {
				if (
					unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(labelOfAvailableVertex) &&
					resources.brick >= 1 &&
					resources.grain >= 1 &&
					resources.lumber >= 1 &&
					resources.wool >= 1
					) {
					unorderedMapOfLabelsOfVerticesAndMoveTypes[labelOfAvailableVertex].push_back("settlement");
				}
			}
			std::vector<std::string> vectorOfLabelsOfVerticesWithSettlementOfPlayer = nextState.settlements[nextPlayer];
			for (std::string& labelOfVertex : vectorOfLabelsOfVerticesWithSettlementOfPlayer) {
				if (resources.grain >= 2 && resources.ore >= 3) {
					unorderedMapOfLabelsOfVerticesAndMoveTypes[labelOfVertex].push_back("city");
				}
			}
			std::vector<std::string> vectorOfLabelsOfVerticesWithCityOfPlayer = nextState.cities[nextPlayer];
			for (std::string& labelOfVertex : vectorOfLabelsOfVerticesWithCityOfPlayer) {
				if (
					resources.brick >= 2 &&
					std::find(nextState.walls[nextPlayer].begin(), nextState.walls[nextPlayer].end(), labelOfVertex) == nextState.walls[nextPlayer].end()
					) {
					unorderedMapOfLabelsOfVerticesAndMoveTypes[labelOfVertex].push_back("wall");
				}
			}
			std::vector<std::string> vectorOfLabelsOfAvailableEdges = board.getVectorOfLabelsOfAvailableEdges(vectorOfLabelsOfOccupiedEdges);
			for (auto& labelOfEdge : vectorOfLabelsOfAvailableEdges) {
				auto [firstLabelOfVertex, secondLabelOfVertex] = board.getVerticesOfEdge(labelOfEdge);
				if (
					(unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(firstLabelOfVertex) || unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(secondLabelOfVertex)) &&
					resources.brick >= 1 &&
					resources.lumber >= 1
					) {
					vectorOfLabelsOfEdges.push_back(labelOfEdge);
				}
			}
		}
		crow::json::wvalue jsonObjectOfPossibleNextMoves(crow::json::type::Object);
		jsonObjectOfPossibleNextMoves["player"] = nextPlayer;
		jsonObjectOfPossibleNextMoves["nextPlayerWillRollDice"] = nextPlayerWillRollDice;

		crow::json::wvalue jsonObjectOfLabelsOfVerticesAndMoveTypes(crow::json::type::Object);
		for (auto& [labelOfVertex, vectorOfMoveTypes] : unorderedMapOfLabelsOfVerticesAndMoveTypes) {
			crow::json::wvalue arrayOfMoveTypes(crow::json::type::List);
			for (size_t i = 0; i < vectorOfMoveTypes.size(); i++) {
				arrayOfMoveTypes[i] = vectorOfMoveTypes[i];
			}
			jsonObjectOfLabelsOfVerticesAndMoveTypes[labelOfVertex] = std::move(arrayOfMoveTypes);
		}
		jsonObjectOfPossibleNextMoves["vertices"] = std::move(jsonObjectOfLabelsOfVerticesAndMoveTypes);

		crow::json::wvalue jsonArrayOfLabelsOfEdges(crow::json::type::List);
		for (size_t i = 0; i < vectorOfLabelsOfEdges.size(); i++) {
			jsonArrayOfLabelsOfEdges[i] = vectorOfLabelsOfEdges[i];
		}
		jsonObjectOfPossibleNextMoves["edges"] = std::move(jsonArrayOfLabelsOfEdges);

		response["possibleNextMoves"] = std::move(jsonObjectOfPossibleNextMoves);
		liveDb.upsertSetting("lastPossibleNextMoves", response["possibleNextMoves"].dump());
	}

};