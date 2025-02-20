#!/usr/bin/env python3
"""
strategy.py

A prior probability is an assumed probability of a visit count in a policy distribution of visit counts.
TODO: Determine whether a policy is a distribution of prior probabilities.

Module strategy implements move selection using an AlphaGo Zero style Monte Carlo Tree Search (MCTS)
and a neural network that provides both a value estimate and a policy / prior for candidate moves.
This mododule supports both settlement and road moves.
TODO: Simulate multiple moves / deeper rollouts.
"""

from .mcts_node import MCTS_Node
import math
import numpy as np
from back_end.settings import settings


def backpropagate(node, value):
    '''
    Backpropagate the value estimated up the tree.
    '''
    while node is not None:
        node.N += 1
        node.W += value
        node.Q = node.W / node.N
        node = node.parent


def expand_node(
    node,
    list_of_labels_of_available_vertices_or_tuples_of_edge_information,
    vertex_coords,
    neural_network
):
    for label_or_tuple in list_of_labels_of_available_vertices_or_tuples_of_edge_information:
        label_of_available_vertex = None
        edge = None
        edge_key_of_available_edge = None
        if node.move_type == "road":
            the_tuple = label_or_tuple
            edge = the_tuple[0]
            edge_key_of_available_edge = the_tuple[1]
            if edge_key_of_available_edge in node.children:
                continue
        else:
            label_of_available_vertex = label_or_tuple
            if label_of_available_vertex in node.children:
                continue
        if node.move_type == "city":
            _, prior = neural_network.evaluate_city(label_of_available_vertex)
        elif node.move_type == "road":
            last_settlement = node.game_state.get("last_settlement")
            _, prior = neural_network.evaluate_road(edge, vertex_coords, last_settlement)
        elif node.move_type == "settlement":
            _, prior = neural_network.evaluate_settlement(label_of_available_vertex)
        else:
            prior = 1.0
        child = MCTS_Node(
            game_state = node.game_state,
            move = label_or_tuple,
            parent = node,
            move_type = node.move_type
        )
        child.P = prior
        if node.move_type == "road":
            node.children[edge_key_of_available_edge] = child
        else:
            node.children[label_of_available_vertex] = child


def select_child(node, c_puct, tolerance = 1e-6):
    '''
    Select a child node with the maximum PUCT score.
    If multiple children have scores within a small tolerance,
    choose the child with the lower visit count.
    If multiple children have scores within a small tolerance and the same visit count,
    choose the child with the higher prior probability.
    '''
    best_score = -float('inf')
    best_candidates = []
    for child in node.children.values():
        u = c_puct * child.P * math.sqrt(node.N) / (1 + child.N)
        score = child.Q + u # mean value Q + exploration bonus U
        if score > best_score + tolerance:
            best_score = score
            best_candidates = [child]
        elif abs(score - best_score) <= tolerance:
            best_candidates.append(child)
    if len(best_candidates) == 1:
        return best_candidates[0]
    best_candidates.sort(key = lambda c: (c.N, -c.P))
    return best_candidates[0]


def simulate_rollout(node, vertex_coords, neural_network):
    '''
    When a leaf node is reached, use the neural network to estimate the value.
    For now, we use a one step evaluation.
    TODO: Implement a deeper implemention that simulates further moves.
    '''
    if node.move_type == "settlement":
        value, _ = neural_network.evaluate_settlement(node.move)
    elif node.move_type == "city":
        value, _ = neural_network.evaluate_city(node.move)
    elif node.move_type == "road":
        last_settlement = node.game_state.get("last_settlement")
        value, _ = neural_network.evaluate_road(node.move[0], vertex_coords, last_settlement)
    else:
        value = 0.0
    return value


def monte_carlo_tree_search(
    root,
    list_of_labels_of_available_vertices_or_tuples_of_edge_information,
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
        if add_dirichlet_noise and root.parent is None:
            moves = list(root.children.keys())
            noise = np.random.dirichlet([alpha] * len(moves))
            for i, move in enumerate(moves):
                child = root.children[move]
                child.P = (1 - epsilon) * child.P + epsilon * noise[i]
    for _ in range(0, num_simulations):
        node = root
        # Complete selection by descending / traversing the tree until a leaf is reached.
        while not node.is_leaf():
            node = select_child(node, c_puct)
        # Complete expansion by expanding any non-terminal node that has been visited before.
        if node.N > 0:
            expand_node(node, list_of_labels_of_available_vertices_or_tuples_of_edge_information, vertex_coords, neural_network)
        # Complete evaluation by using the neural network to get the value estimate.
        value = simulate_rollout(node, vertex_coords, neural_network)
        # Complete backpropagation by updating node statistics.
        backpropagate(node, value)
    # Choose the move from the root with the highest visit count / that was most visited.
    best_key, best_child = max(root.children.items(), key = lambda item: item[1].N)
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