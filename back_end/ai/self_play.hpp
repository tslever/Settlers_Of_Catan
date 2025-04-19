#pragma once


#include "../db/Database.hpp"
#include "../game/game_state.hpp"
#include "neural_network.hpp"
#include "strategy.hpp"


namespace AI {

    constexpr int MAXIMUM_NUMBER_OF_MOVES = 100;

    // TODO: Consider recording additional data such as full game trajectory, move probabilities, and/or everything about the board and structures on the board.
    struct TrainingExample {
        int player; // player represents player who made move.
        GameState gameState; // `gameState` represents snapshot of game state after move.
        std::string move; // move represents label of vertex at which settlement or city is placed or edge at which road is placed.
		std::string moveType; // moveType represents type of move (e.g., settlement, city, road).
        double value; // value represents target value (e.g., game outcome from perspective of current player).
        double policy; // policy represents target prior probability derived from numbers of visits to nodes that occur during Monte Carlo Tree Search.
    };

    /* Function `runSelfPlayGame` simulates a complete game trajectory
    * by repeatedly using Monte Carlo Tree Search to select a move and by updating the game state
    * until the game reaches the done phase when setup is complete.
    */
    std::vector<TrainingExample> runSelfPlayGame(
        AI::WrapperOfNeuralNetwork& neuralNet,
        int numberOfSimulations,
        double cPuct,
        double tolerance,
		double dirichletMixingWeight,
		double dirichletShape
    ) {
        Logger::info("[SELF PLAY GAME] A self play game is running.");
        std::vector<TrainingExample> vectorOfTrainingExamples;
        // Initialize game state using default settings, including initial phase `Phase::TO_PLACE_FIRST_SETTLEMENT`.
        GameState gameState;
        // Simulate moves until phase becomes `Phase::DONE`, or up to a maximum number of moves to safeguard against infinite loops.
        int numberOfMovesSimulated = 0;
        while (gameState.phase != Game::Phase::DONE && numberOfMovesSimulated < MAXIMUM_NUMBER_OF_MOVES) {
            if (gameState.phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
                Logger::info("    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " placing their first settlement is being simulated.");
            }
            else if (gameState.phase == Game::Phase::TO_PLACE_FIRST_ROAD) {
                Logger::info("    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " placing their first road is being simulated.");
            }
            else if (gameState.phase == Game::Phase::TO_PLACE_FIRST_CITY) {
                Logger::info("    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " placing their first city is being simulated.");
            }
            else if (gameState.phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
                Logger::info("    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " placing their second road is being simulated.");
            }
            else if (gameState.phase == Game::Phase::TURN) {
                Logger::info("    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " playing their turn is being simulated.");
            }
            int currentPlayer = gameState.currentPlayer;
            std::pair<std::string, int> pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount = runMcts(
                gameState,
                neuralNet,
                numberOfSimulations,
                cPuct,
                tolerance,
                dirichletMixingWeight,
                dirichletShape
            );
            std::string labelOfVertexOrEdgeKey = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.first;
            if (labelOfVertexOrEdgeKey.empty()) {
                throw std::runtime_error("[SELF PLAY] MCTS did not return a valid move.");
            }

            std::string phase = gameState.phase;
            if (phase.find("settlement") != std::string::npos) {
                gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
            }
            else if (phase.find("city") != std::string::npos) {
                gameState.placeCity(currentPlayer, labelOfVertexOrEdgeKey);
            }
            else if (phase.find("road") != std::string::npos) {
                gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
            }
            else if (phase == "turn") {
                Board board;
                if (board.isLabelOfVertex(labelOfVertexOrEdgeKey)) {
                    gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
                }
                else if (board.isLabelOfEdge(labelOfVertexOrEdgeKey)) {
                    gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
                }
            }
            TrainingExample trainingExample;
            trainingExample.player = currentPlayer;
            trainingExample.gameState = gameState;
            trainingExample.move = labelOfVertexOrEdgeKey;
			if (phase.find("settlement") != std::string::npos) {
				trainingExample.moveType = "settlement";
			}
			else if (phase.find("city") != std::string::npos) {
				trainingExample.moveType = "city";
			}
			else if (phase.find("road") != std::string::npos) {
				trainingExample.moveType = "road";
			}
			else if (phase == "turn") {
				Board board;
				if (board.isLabelOfVertex(labelOfVertexOrEdgeKey)) {
					trainingExample.moveType = "settlement";
				}
				else if (board.isLabelOfEdge(labelOfVertexOrEdgeKey)) {
					trainingExample.moveType = "road";
				}
			}
            trainingExample.value = 0.0; // temporary dummy value that will be updated once game outcome is known
            int visitCount = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.second;
            double priorProbabilityOfVisit = static_cast<double>(visitCount) / static_cast<double>(numberOfSimulations);
            trainingExample.policy = priorProbabilityOfVisit;
            vectorOfTrainingExamples.push_back(trainingExample);
            numberOfMovesSimulated++;
        }
        for (TrainingExample& trainingExample : vectorOfTrainingExamples) {
            trainingExample.value = (trainingExample.player == gameState.winner) ? 1.0 : -1.0;
        }
        Logger::info(
            "[SELF PLAY] Game simulation completed in " + std::to_string(numberOfMovesSimulated) + " moves " +
            "with winner Player " + std::to_string(gameState.winner) + "."
        );

        return vectorOfTrainingExamples;
    }

}