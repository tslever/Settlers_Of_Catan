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
        while (gameState.phase != Game::Phase::Done && numberOfMovesSimulated < MAXIMUM_NUMBER_OF_MOVES) {

            if (gameState.phase == Game::Phase::RollDice) {
                gameState.rollDice();
                Logger::info(
                    "    [SELF PLAY PHASE] Player " + std::to_string(gameState.currentPlayer) + " rolling the dice was simulated."
                    /*"    Yellow and red production and white event dice had values " +
					std::to_string(gameState.yellowProductionDie) + ", " +
                    std::to_string(gameState.redProductionDie) + ", and " +
                    gameState.whiteEventDie + "."*/
                );
                gameState.updatePhase();
                continue;
            }

            int currentPlayer = gameState.currentPlayer;
            auto [labelOfVertexOrEdgeKey, moveType, visitCount] = runMcts(
                gameState,
                neuralNet,
                numberOfSimulations,
                cPuct,
                tolerance,
                dirichletMixingWeight,
                dirichletShape
            );
            if (labelOfVertexOrEdgeKey.empty()) {
                throw std::runtime_error("[SELF PLAY] MCTS did not return a valid move.");
            }

            if (moveType == "pass") {
                gameState.updatePhase();
            }
            else if (moveType == "settlement") {
                gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
            }
			else if (moveType == "city") {
				gameState.placeCity(currentPlayer, labelOfVertexOrEdgeKey);
			}
			else if (moveType == "road") {
				gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
			}
            else if (moveType == "wall") {
				gameState.placeCityWall(currentPlayer, labelOfVertexOrEdgeKey);
            }
			else {
				throw std::runtime_error("[SELF PLAY] " + moveType + " is not a valid move type.");
            }
            TrainingExample trainingExample;
            trainingExample.player = currentPlayer;
            trainingExample.gameState = gameState;
            trainingExample.move = labelOfVertexOrEdgeKey;
            trainingExample.moveType = moveType;
            trainingExample.value = 0.0; // temporary dummy value that will be updated once game outcome is known
            trainingExample.policy = static_cast<double>(visitCount) / static_cast<double>(numberOfSimulations); // prior probability of visit
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