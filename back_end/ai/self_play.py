from ..board import Board
from back_end.game.game_state import GameState
from .mcts.node import MCTS_Node
from ..board import TOKEN_DOT_MAPPING
from ..board import TOKEN_MAPPING
from .strategy import backpropagate
import copy
from .strategy import expand_node
import logging
import math
import numpy as np
from .strategy import select_child
from .strategy import simulate_rollout
from back_end.logger import set_up_logging
from back_end.settings import settings


set_up_logging()
logger = logging.getLogger(__name__)


# Create a single Board instance.
board = Board()
# Build a mapping from vertex label to (x,y) from the board.
vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}


def available_road_moves(last_settlement, existing_roads):
    """Return available road moves using the Board’s method."""
    return board.get_available_road_moves(last_settlement, list(existing_roads))


def compute_game_outcome(settlements):
    strengths = {}
    for player, vertex in settlements.items():
        total_pips = 0
        hex_count = 0
        for hex_tile in board.hexes:
            for vx, vy in board.get_hex_vertices(hex_tile):
                if (math.isclose(vx, vertex_coords[vertex][0], abs_tol = settings.margin_of_error) and
                    math.isclose(vy, vertex_coords[vertex][1], abs_tol = settings.margin_of_error)):
                    hex_id = hex_tile["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        total_pips += TOKEN_DOT_MAPPING.get(token, 0)
                        hex_count += 1
                    break
        strengths[player] = total_pips
    if not strengths:
        return {}
    winner = max(strengths, key=lambda p: strengths[p])
    return {player: 1 if player == winner else -1 for player in strengths}


def run_mcts_for_move(
    state,
    move_type,
    list_of_labels_of_available_vertices_or_tuples_of_edge_information,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    number_of_simulations,
    c_puct,
    neural_network
):
    # Create a root node; pass the game state as a dictionary snapshot.
    copy_of_game_state = copy.deepcopy(state.get_state_snapshot())
    root = MCTS_Node(copy_of_game_state, move_type = move_type)
    expand_node(
        root,
        list_of_labels_of_available_vertices_or_tuples_of_edge_information,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        neural_network
    )
    for _ in range(0, number_of_simulations):
        node = root
        while not node.is_leaf():
            node = select_child(node, c_puct, tolerance = 1e-6)
        if node.N > 0:
            expand_node(
                node,
                list_of_labels_of_available_vertices_or_tuples_of_edge_information,
                dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
                neural_network
            )
        value = simulate_rollout(node, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, neural_network)
        backpropagate(node, value)
    total_visits = sum(child.N for child in root.children.values())
    policy = {move: (child.N / total_visits) for move, child in root.children.items()}
    chosen_move = max(root.children.items(), key=lambda item: item[1].N)[0]
    return chosen_move, policy


def simulate_self_play_game(
    neural_network,
    number_of_simulations = settings.number_of_simulations,
    c_puct = settings.c_puct
):
    # Use our GameState class rather than a raw dict.
    game_state = GameState()
    training_examples = []
    # Define move order for two rounds (players 1,2,3 then 3,2,1).
    first_round = [1, 2, 3]
    second_round = [3, 2, 1]
    # First round: settlement then road.
    for player in first_round:
        game_state.current_player = player
        game_state.phase = "settlement"
        list_of_labels_of_vertices_with_settlements = list(game_state.settlements.values())
        list_of_labels_of_vertices_with_cities = list(game_state.cities.values())
        list_of_labels_of_occupied_vertices = list_of_labels_of_vertices_with_settlements + list_of_labels_of_vertices_with_cities
        list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
        if not list_of_labels_of_available_vertices:
            logger.exception("There are no vertices on which a settlement can be built.")
            break
        chosen_settlement, policy = run_mcts_for_move(
            game_state,
            "settlement",
            list_of_labels_of_available_vertices,
            vertex_coords,
            number_of_simulations,
            settings.c_puct,
            neural_network
        )
        training_examples.append({
            "state": copy.deepcopy(game_state.get_state_snapshot()),
            "move_type": "settlement",
            "policy": policy,
            "player": player
        })
        game_state.place_settlement(player, chosen_settlement)
        last_settlement = chosen_settlement
        game_state.current_player = player
        game_state.phase = "road"
        available_roads = available_road_moves(last_settlement, game_state.roads.values())
        if not available_roads:
            print("No available roads!")
            break
        chosen_road, policy = run_mcts_for_move(game_state, "road", available_roads, vertex_coords, number_of_simulations, c_puct, neural_network)
        training_examples.append({
            "state": copy.deepcopy(game_state.get_state_snapshot()),
            "move_type": "road",
            "policy": policy,
            "player": player
        })
        if isinstance(chosen_road, tuple):
            game_state.roads[player] = chosen_road[1]
        else:
            game_state.roads[player] = chosen_road
    # Second round: city then road.
    for player in second_round:
        game_state.current_player = player
        game_state.phase = "city"
        list_of_labels_of_vertices_with_settlements = list(game_state.settlements.values())
        list_of_labels_of_vertices_with_cities = list(game_state.cities.values())
        list_of_labels_of_occupied_vertices = list_of_labels_of_vertices_with_settlements + list_of_labels_of_vertices_with_cities
        list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
        if not list_of_labels_of_available_vertices:
            logger.warning("No available cities!")
            break
        chosen_city, policy = run_mcts_for_move(game_state, "city", list_of_labels_of_available_vertices, vertex_coords, number_of_simulations, c_puct, neural_network)
        training_examples.append({
            "state": copy.deepcopy(game_state.get_state_snapshot()),
            "move_type": "city",
            "policy": policy,
            "player": player
        })
        game_state.place_city(player, chosen_city)
        last_city = chosen_city
        game_state.current_player = player
        game_state.phase = "road"
        available_roads = available_road_moves(last_city, game_state.roads.values())
        if not available_roads:
            logger.warning("No available roads!")
            break
        chosen_road, policy = run_mcts_for_move(game_state, "road", available_roads, vertex_coords, number_of_simulations, c_puct, neural_network)
        training_examples.append({
            "state": copy.deepcopy(game_state.get_state_snapshot()),
            "move_type": "road",
            "policy": policy,
            "player": player
        })
        if isinstance(chosen_road, tuple):
            game_state.roads[player] = chosen_road[1]
        else:
            game_state.roads[player] = chosen_road
    outcomes = compute_game_outcome(game_state.cities)
    for sample in training_examples:
        sample["value"] = outcomes.get(sample["player"], 0)
    return training_examples


def generate_training_data(num_games=10, num_simulations = settings.number_of_simulations, c_puct = settings.c_puct):
    all_examples = []
    for _ in range(num_games):
        examples = simulate_self_play_game(num_simulations=num_simulations, c_puct=c_puct)
        all_examples.extend(examples)
    npy_file = settings.training_data_path
    np.save(npy_file, all_examples)
    logger.info(f"Generated {len(all_examples)} training examples.")
    return all_examples


if __name__ == "__main__":
    generate_training_data(num_games=100, num_simulations = settings.number_of_simulations, c_puct = settings.c_puct)