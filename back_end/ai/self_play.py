'''
self_play.py

Module `self_play` implements a self play system to generate training data in the form of (state, move evaluation) pairs.
In each self play game the system simulates the initial setup phase of Settlers of Catan,
which consists of two rounds of settlement and road placements.
Simulation uses our existing Monte Carlo Tree Search and neural network functions.
For every move the system records:
- a snapshot of the current game state,
- the move type ("settlement" or "road"),
- the move policy (e.g., the normalized visit counts of available moves from MCTS), and
- the player making the move.
After the game simulation, a simple outcome is computed (using a heuristic based on settlement "strength"),
and attached to every training example.
Finally, all training examples are saved (here as a NumPy file) so they can later be used to train our neural network.
TODO: Replace heuristically determined outcomes with the results of self play.
'''


from ..utilities.board import MARGIN_OF_ERROR
from .mcts_node import MCTS_Node
from ..utilities.board import TOKEN_DOT_MAPPING
from ..utilities.board import TOKEN_MAPPING
from ..utilities.board import all_edges_of_all_hexes
from .strategy import backpropagate
import copy
from .strategy import expand_node
from ..utilities.board import get_edge_key
from ..utilities.board import get_hex_vertices
from ..utilities.board import hexes
import math
import numpy as np
import os
import random
from .strategy import simulate_rollout
from ..utilities.board import vertices_with_labels


vertex_coords = {v["label"]: (v["x"], v["y"]) for v in vertices_with_labels}


def compute_distance(v1, v2):
    return math.sqrt((v1[0] - v2[0]) ** 2 + (v1[1] - v2[1]) ** 2)


def available_settlement_moves(existing_settlements):
    '''
    Return the list of vertex labels available for settlement placement.
    A vertex is available if it is not already used and is not too close to any already placed settlement.
    Here we use a simple Euclidean distance threshold.
    '''
    used_vertices = set(existing_settlements)
    available = []
    threshold = 2.0
    for v in vertices_with_labels:
        label = v["label"]
        if label in used_vertices:
            continue
        too_close = False
        for used in used_vertices:
            if used in vertex_coords:
                if compute_distance(vertex_coords[label], vertex_coords[used]) < threshold:
                    too_close = True
                    break
        if not too_close:
            available.append(label)
    return available


def available_road_moves(last_settlement, existing_roads):
    '''
    Return a list of available road moves as tuples (edge, edge_key) adjacent to the last settlement.
    '''
    if last_settlement is None or last_settlement not in vertex_coords:
        return []
    settlement_coord = vertex_coords[last_settlement]
    adjacent_edges = []
    for edge in all_edges_of_all_hexes:
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        if (
            (
                math.isclose(v1[0], settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and
                math.isclose(v1[1], settlement_coord[1], abs_tol = MARGIN_OF_ERROR)
            ) or
            (
                math.isclose(v2[0], settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and
                math.isclose(v2[1], settlement_coord[1], abs_tol = MARGIN_OF_ERROR)
            )
        ):
            edge_key = get_edge_key(v1, v2)
            if edge_key not in existing_roads:
                adjacent_edges.append((edge, edge_key))
    return adjacent_edges


def compute_game_outcome(settlements):
    '''
    Compute a simple outcome based on settlement "strengths".
    For each player, sum the pip counts (using TOKEN_DOT_MAPPING) of hexes adjacent to the settlement.
    The player with the highest total gets an outcome of +1; all others get -1.
    TODO: Use final game result.
    '''
    strengths = {}
    for player, vertex in settlements.items():
        total_pips = 0
        hex_count = 0
        for hex in hexes:
            for vx, vy in get_hex_vertices(hex):
                if (
                    math.isclose(vx, vertex_coords[vertex][0], abs_tol = MARGIN_OF_ERROR) and
                    math.isclose(vy, vertex_coords[vertex][1], abs_tol = MARGIN_OF_ERROR)
                ):
                    hex_id = hex["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        total_pips += TOKEN_DOT_MAPPING.get(token, 0)
                        hex_count += 1
                    break
        strengths[player] = total_pips
    if not strengths:
        return {}
    winner = max(strengths, key = lambda p: strengths[p])
    outcomes = {}
    for player in strengths:
        outcomes[player] = 1 if player == winner else -1
    return outcomes


# Helper function to run MCTS on a given move type.
def run_mcts_for_move(state, move_type, available_moves, num_simulations):
    # Create a root node (note: we pass a copy of state so that simulation does not alter our "real" state)
    root = MCTS_Node(game_state = copy.deepcopy(state), move_type = move_type)
    # Expand the node with all available moves.
    expand_node(root, available_moves, vertex_coords)
    # Optionally add Dirichlet noise at the root if desired.
    # Run a number of MCTS simulations.
    for _ in range(num_simulations):
        node = root
        # Selection: descend until a leaf is reached.
        while not node.is_leaf():
            # Use your existing selection function (here we simply pick the child with highest UCB).
            # For simplicity we choose a random child if more than one is available.
            node = random.choice(list(node.children.values()))
        # If the node has been visited before, expand it.
        if node.N > 0:
            expand_node(node, available_moves, vertex_coords)
        # Rollout: use NN evaluation.
        value = simulate_rollout(node, vertex_coords)
        # Backpropagate the rollout value.
        backpropagate(node, value)
    # Now compute the policy distribution from the root's children.
    total_visits = sum(child.N for child in root.children.values())
    policy = {move: (child.N / total_visits) for move, child in root.children.items()}
    # Choose the move with the highest visit count.
    chosen_move = max(root.children.items(), key = lambda item: item[1].N)[0]
    return chosen_move, policy


def simulate_self_play_game(num_simulations = 50, c_puct = 1.0):
    '''
    Simulate one self play game for the initial setup phase.
    For simplicity we simulate for 3 players and 2 rounds of moves:
    - Round 1: players 1, 2, 3 each place a settlement then a road.
    - Round 2: players 3, 2, 1 each place a settlement then a road.
    At each move the code runs MCTS for a fixed number of simulations using our existing routines
    so that the root's children have visit counts that we normalize to a probability distribution.
    For each move we record a training example with:
    - state: a snapshot dictionary of the game state,
    - move_type: "settlement" or "road",
    - policy: the MCTS derived move distribution
    - player: the player who made the move
    At the end the outcome (a win / loss value for each player) is computed and attached to each example.
    '''
    # Initialize a simple in memory game state.
    # TODO: Extend the game state to include additional game state information.
    game_state = {
        "current_player": None, # will be set for each move
        "phase": None, # "settlement" or "road"
        "settlements": {}, # mapping from player to chosen vertex
        "roads": {} # mapping from player to chosen road
    }
    training_examples = []
    # Define the order in which moves occur.
    # For the first round settlement, players 1, 2, 3; then for he road, same order;
    # then for the second round settlement, reverse order 3, 2, 1; then the road likewise.
    first_round = [1, 2, 3]
    second_round = [3, 2, 1]
    # First round: settlement then road
    for player in first_round:
        # Settlement move
        game_state["current_player"] = player
        game_state["phase"] = "settlement"
        available_settlements = available_settlement_moves(game_state["settlements"].values())
        if not available_settlements:
            print("No available settlements!")
            break
        chosen_settlement, policy = run_mcts_for_move(game_state, "settlement", available_settlements, num_simulations)
        training_examples.append({
            "state": copy.deepcopy(game_state),
            "move_type": "settlement",
            "policy": policy,
            "player": player
        })
        # Update game state
        game_state["settlements"][player] = chosen_settlement
        last_settlement = chosen_settlement # for road placement
        # Road move
        game_state["current_player"] = player
        game_state["phase"] = "road"
        available_roads = available_road_moves(last_settlement, game_state["roads"].values())
        if not available_roads:
            print("No available roads!")
            break
        chosen_road, policy = run_mcts_for_move(game_state, "road", available_roads, num_simulations)
        training_examples.append({
            "state": copy.deepcopy(game_state),
            "move_type": "road",
            "policy": policy,
            "player": player
        })
        # Update game state: record the chosen road.
        # For roads the move may be stored as a tuple; here we record the road's unique key.
        if isinstance(chosen_road, tuple):
            game_state["roads"][player] = chosen_road[1]
        else:
            game_state["roads"][player] = chosen_road
    # Second round: settlement then road (reverse order)
    for player in second_round:
        game_state["current_player"] = player
        game_state["phase"] = "settlement"
        available_settlements = available_settlement_moves(game_state["settlements"].values())
        if not available_settlements:
            print("No available settlements!")
            break
        chosen_settlement, policy = run_mcts_for_move(game_state, "settlement", available_settlements, num_simulations)
        training_examples.append({
            "state": copy.deepcopy(game_state),
            "move_type": "settlement",
            "policy": policy,
            "player": player
        })
        game_state["settlements"][player] = chosen_settlement
        last_settlement = chosen_settlement
        # Road move
        game_state["current_player"] = player
        game_state["phase"] = "road"
        available_roads = available_road_moves(last_settlement, game_state["roads"].values())
        if not available_roads:
            print("No available roads!")
            break
        chosen_road, policy = run_mcts_for_move(game_state, "road", available_roads, num_simulations)
        training_examples.append({
            "state": copy.deepcopy(game_state),
            "move_type": "road",
            "policy": policy,
            "player": player
        })
        if isinstance(chosen_road, tuple):
            game_state["roads"][player] = chosen_road[1]
        else:
            game_state["roads"][player] = chosen_road
    # Compute a simple outcome (e.g., which player's settlements are strongest)
    outcomes = compute_game_outcome(game_state["settlements"])
    # Attach outcome value to each training sample (from the perspective of the player who made the move)
    for sample in training_examples:
        sample["value"] = outcomes.get(sample["player"], 0)
    return training_examples


def generate_training_data(num_games = 10, num_simulations = 100, c_puct = 1.0):
    '''
    Run many self play games and collect the training examples.
    The data is saved as a NumPy binary file for later use in training.
    '''
    all_examples = []
    for _ in range(num_games):
        examples = simulate_self_play_game(num_simulations = num_simulations, c_puct = c_puct)
        all_examples.extend(examples)
    current_dir = os.path.dirname(os.path.abspath(__file__))
    npy_file = os.path.join(current_dir, "self_play_training_data.npy")
    np.save(npy_file, all_examples)
    print(f"Generated {len(all_examples)} training examples.")
    return all_examples


if __name__ == "__main__":
    generate_training_data(num_games = 100, num_simulations = 100, c_puct = 1.0)