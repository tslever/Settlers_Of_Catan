#!/usr/bin/env python3
'''
strategy.py

A prior probability is an assumed probability of a visit count in a policy distribution of visit counts.
A policy is a distribution over moves of prior probabilities based on normalized MCTS visit counts.

Module strategy implements move selection using an AlphaGo Zero style Monte Carlo Tree Search (MCTS)
and a neural network that provides both a value estimate and a policy.
The simulation currently uses a single step rollout (i.e., one move evaluation) for value estimation;
future improvements may include multi-step rollouts for deeper evaluation.
TODO: Include multi-step rollouts for deeper evaluation.
'''

from .mcts.node import MCTS_Node
from back_end.ai.mcts.backpropagation import backpropagate
from back_end.ai.mcts.expansion import expand_node
import numpy as np
from back_end.ai.mcts.selection import select_child
from back_end.settings import settings
from back_end.ai.mcts.simulation import simulate_rollout


def inject_dirichlet_noise(root, epsilon: float = 0.25, alpha: float = 0.03) -> None:
    '''
    Inject Dirichlet noise into the children of the root node.
    This is used to encourage exploration at the root during MCTS.
    '''
    # Only inject if the root has no parent (i.e., the root is the true root).
    if root.parent is None:
        moves = list(root.children.keys())
        noise = np.random.dirichlet([alpha] * len(moves))
        for i, move in enumerate(moves):
            child = root.children[move]
            child.P = (1 - epsilon) * child.P + epsilon * noise[i]


def monte_carlo_tree_search(
    root,
    list_of_labels_of_available_vertices_or_tuples_of_edge_information, # representing available moves
    vertex_coords,
    num_simulations,
    c_puct,
    neural_network,
    add_dirichlet_noise = True,
    epsilon = 0.25,
    alpha = 0.03
):
    '''
    Run MCTS simulations starting at the root node.
    If add_dirichlet_noise is True and we are at the root with no parent node,
    then add Dirichlet noise to the prior probabilities to encourage exploration.
    First, expand the root with all available moves. Then, for each simulation:
    1. Complete selection by descending the tree using the PUCT formula /
       choosing the child with highest UCB until a leaf is reached.
    2. Complete expansion by expanding the leaf if the leaf is visited but not already expanded.
    3. Complete evaluation by using the neural network to evaluate the leaf.
    4. Complete backpropagation by propagating the evaluation value back up the tree.
    Finally, return the move from the root with the highest visit count. 
    '''
    if root.is_leaf():
        expand_node(root, list_of_labels_of_available_vertices_or_tuples_of_edge_information, vertex_coords, neural_network)
        if add_dirichlet_noise:
            inject_dirichlet_noise(root, epsilon, alpha)
    for _ in range(0, num_simulations):
        node = root
        # Complete selection by descending / traversing the tree until a leaf is reached.
        while not node.is_leaf():
            node = select_child(node, c_puct, tolerance = 1e-6)
        # Complete expansion by expanding any non-terminal node that has been visited before.
        if node.N > 0:
            expand_node(node, list_of_labels_of_available_vertices_or_tuples_of_edge_information, vertex_coords, neural_network)
        # Complete evaluation by using the neural network to get the value estimate.
        value = simulate_rollout(node, vertex_coords, neural_network)
        # Complete backpropagation by updating node statistics.
        backpropagate(node, value)
    # Choose the move from the root with the highest visit count / that was most visited.
    _, best_child = max(root.children.items(), key = lambda item: item[1].N)
    return best_child.move


def predict_best_settlement(
    game_state,
    list_of_labels_of_available_vertices,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    neural_network,
    number_of_simulations = settings.number_of_simulations,
    c_puct = settings.c_puct
):
    '''
    Run a full MCTS for settlement moves and return the best vertex.
    game_state: dictionary containing details of the current state
    available_vertices: list of labels (e.g., "V01", "V02", ...) of vertices available for settlement
    vertex_coords: dictionary mapping vertex label to (x, y) coordinates / positions
    '''
    root = MCTS_Node(game_state = game_state, move_type = "settlement")
    best_vertex = monte_carlo_tree_search(
        root,
        list_of_labels_of_available_vertices,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        number_of_simulations,
        c_puct,
        neural_network,
        add_dirichlet_noise = True
    )
    return best_vertex


def predict_best_city(
    game_state,
    list_of_labels_of_available_vertices,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    neural_network,
    number_of_simulations = settings.number_of_simulations,
    c_puct = settings.c_puct
):
    '''
    Run a full MCTS for city moves and return the best vertex.
    game_state: dictionary containing details of the current state
    available_vertices: list of vertex labels (e.g., "V01", "V02", ...) available for city
    vertex_coords: dictionary mapping vertex label to (x, y) coordinates / positions
    '''
    root = MCTS_Node(game_state = game_state, move_type = "city")
    best_vertex = monte_carlo_tree_search(
        root,
        list_of_labels_of_available_vertices,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        number_of_simulations,
        c_puct,
        neural_network,
        add_dirichlet_noise = True
    )
    return best_vertex


def predict_best_road(
    game_state,
    list_of_tuples_of_edge_information,
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    neural_network,
    number_of_simulations = settings.number_of_simulations,
    c_puct = settings.c_puct
):
    '''
    Run a full MCTS for road moves and return the best edge and its key.
    available_edges: list of tuples (edge, edge_key) available for road placement
    game_state: dictionary that must include `last_settlement` for road evaluation
    '''
    label_of_vertex_of_last_settlement = game_state.get("last_settlement")
    if label_of_vertex_of_last_settlement is None:
        return None, None
    root = MCTS_Node(game_state, move_type = "road")
    tuple_of_information_re_best_edge = monte_carlo_tree_search(
        root,
        list_of_tuples_of_edge_information,
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
        number_of_simulations,
        c_puct,
        neural_network,
        add_dirichlet_noise = True
    )
    return tuple_of_information_re_best_edge