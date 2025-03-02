from back_end.board import Board
from back_end.game.game_state import GameState
from back_end.ai.mcts.node import MCTS_Node
from back_end.board import TOKEN_DOT_MAPPING
from back_end.board import TOKEN_MAPPING
from back_end.ai.strategy import backpropagate
import copy
from back_end.ai.strategy import expand_node
import logging
import math
import numpy as np
from back_end.ai.strategy import select_child
from back_end.ai.strategy import simulate_rollout
from back_end.logger import set_up_logging
from back_end.settings import settings


set_up_logging()
logger = logging.getLogger(__name__)


# Create a single Board instance.
board = Board()
# Build a mapping from vertex label to (x,y) from the board.
dictionary_of_labels_of_vertices_and_tuples_of_coordinates = {v["label"]: (v["x"], v["y"]) for v in board.vertices}


def simulate_settlement_move(
    game_state: GameState,
    neural_network,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    num_simulations,
    c_puct
):
    list_of_vertices_of_settlements = list(game_state.settlements.values())
    list_of_vertices_of_cities = list(game_state.cities.values())
    list_of_labels_of_occupied_vertices = list_of_vertices_of_settlements + list_of_vertices_of_cities
    list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
    if not list_of_labels_of_available_vertices:
        logger.exception("No vertices are available for settlement placement.")
        return None, None
    label_of_chosen_vertex, policy = run_mcts_for_move(
        game_state,
        "settlement",
        list_of_labels_of_available_vertices,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        num_simulations,
        c_puct,
        neural_network
    )
    game_state.place_settlement(game_state.current_player, label_of_chosen_vertex)
    training_example = {
        "state": copy.deepcopy(game_state.get_state_snapshot()),
        "move_type": "settlement",
        "policy": policy,
        "player": game_state.current_player
    }
    return label_of_chosen_vertex, training_example


def simulate_road_move(
    game_state: GameState,
    neural_network,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    num_simulations,
    c_puct,
    last_move
):
    list_of_edge_keys = list(game_state.roads.values())
    list_of_tuples_of_information_re_available_edges = board.get_available_road_moves(last_move, list_of_edge_keys)
    if not list_of_tuples_of_information_re_available_edges:
        logger.exception("No roads are available for road placement.")
        return None, None
    tuple_of_information_re_chosen_edge, policy = run_mcts_for_move(
        game_state,
        "road",
        list_of_tuples_of_information_re_available_edges,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        num_simulations,
        c_puct,
        neural_network
    )
    key_of_chosen_edge = tuple_of_information_re_chosen_edge[1] if isinstance(tuple_of_information_re_chosen_edge, tuple) else tuple_of_information_re_chosen_edge
    game_state.place_road(game_state.current_player, key_of_chosen_edge)
    training_example = {
        "state": copy.deepcopy(game_state.get_state_snapshot()),
        "move_type": "road",
        "policy": policy,
        "player": game_state.current_player
    }
    return key_of_chosen_edge, training_example


def simulate_city_move(game_state: GameState, neural_network, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, num_simulations, c_puct):
    list_of_vertices_of_settlements = list(game_state.settlements.values())
    list_of_vertices_of_cities = list(game_state.cities.values())
    list_of_labels_of_occupied_vertices = list_of_vertices_of_settlements + list_of_vertices_of_cities
    list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
    if not list_of_labels_of_available_vertices:
        logger.exception("No vertices are available for city placement.")
        return None, None
    label_of_chosen_vertex, policy = run_mcts_for_move(
        game_state,
        "city",
        list_of_labels_of_available_vertices,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        num_simulations,
        c_puct,
        neural_network
    )
    game_state.place_city(game_state.current_player, label_of_chosen_vertex)
    training_example = {
        "state": copy.deepcopy(game_state.get_state_snapshot()),
        "move_type": "city",
        "policy": policy,
        "player": game_state.current_player
    }
    return label_of_chosen_vertex, training_example


def simulate_self_play_game(neural_network, number_of_simulations = settings.number_of_simulations, c_puct = settings.c_puct):
    game_state = GameState()
    training_examples = []
    for player in [1, 2, 3]:
        game_state.current_player = player
        game_state.phase = "settlement"
        label_of_chosen_vertex, example = simulate_settlement_move(
            game_state,
            neural_network,
            dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
            number_of_simulations,
            c_puct
        )
        if not label_of_chosen_vertex:
            break
        training_examples.append(example)

        game_state.phase = "road"
        key_of_chosen_edge, example = simulate_road_move(
            game_state,
            neural_network,
            dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
            number_of_simulations,
            c_puct,
            label_of_chosen_vertex
        )
        if not key_of_chosen_edge:
            break
        training_examples.append(example)
    
    for player in [3, 2, 1]:
        game_state.current_player = player
        game_state.phase = "city"
        label_of_chosen_vertex, example = simulate_city_move(
            game_state,
            neural_network,
            dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
            number_of_simulations,
            c_puct
        )
        if not label_of_chosen_vertex:
            break
        training_examples.append(example)

        game_state.phase = "road"
        key_of_chosen_edge, example = simulate_road_move(
            game_state,
            neural_network,
            dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
            number_of_simulations,
            c_puct,
            label_of_chosen_vertex
        )
        if not key_of_chosen_edge:
            break
        training_examples.append(example)
    
    outcomes = compute_game_outcome(game_state.cities)
    for example in training_examples:
        example["value"] = outcomes.get(example["player"], 0)
    return training_examples


def compute_game_outcome(settlements):
    strengths = {}
    for player, vertex in settlements.items():
        total_pips = 0
        hex_count = 0
        for hex_tile in board.hexes:
            for vx, vy in board.get_hex_vertices(hex_tile):
                if (math.isclose(vx, dictionary_of_labels_of_vertices_and_tuples_of_coordinates[vertex][0], abs_tol = settings.margin_of_error) and
                    math.isclose(vy, dictionary_of_labels_of_vertices_and_tuples_of_coordinates[vertex][1], abs_tol = settings.margin_of_error)):
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


def generate_training_data(num_games=10, num_simulations = settings.number_of_simulations, c_puct = settings.c_puct):
    all_examples = []
    for _ in range(num_games):
        examples = simulate_self_play_game(num_simulations=num_simulations, c_puct=c_puct)
        all_examples.extend(examples)
    npy_file = settings.training_data_path
    np.save(npy_file, all_examples)
    logger.info(f"Generated {len(all_examples)} training examples.")
    return all_examples