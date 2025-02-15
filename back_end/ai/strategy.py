#!/usr/bin/env python3
"""
strategy.py

A prior probability is an assumed probability of a visit count in a policy distribution of visit counts.

Module strategy implements move selection using an AlphaGo Zero style Monte Carlo Tree Search (MCTS)
and a neural network that provides both a value estimate and a policy / prior for candidate moves.
This mododule supports both settlement and road moves.
TODO: Simulate multiple moves / deeper rollouts.
"""

from .mcts_node import MCTS_Node
import math
from .neural_network import neural_network
import numpy as np
from back_end.settings import settings


def select_child(node, c_puct):
    '''
    Select a child node with the maximum PUCT score.
    '''
    best_score = -float('inf')
    best_child = None
    for child in node.children.values():
        u = c_puct * child.P * math.sqrt(node.N) / (1 + child.N)
        score = child.Q + u # mean value Q + exploration bonus U
        if score > best_score:
            best_score = score
            best_child = child
    return best_child


def expand_node(node, available_moves, vertex_coords):
    '''
    Expand the node by adding one child for every available move.
    For settlements, `available_moves` is a list of vertex labels.
    For roads, `available_moves` is a list of tuples (edge, edge_key).
    '''
    for move in available_moves:
        # For settlements, use the vertex label as the move.
        # For roads, use move[1] as the unique key.
        key = move[1] if node.move_type == "road" else move
        if key in node.children:
            continue
        if node.move_type == "settlement":
            value, prior = neural_network.predict_settlement(node.game_state, move, vertex_coords)
        elif node.move_type == "road":
            last_settlement = node.game_state.get("last_settlement")
            value, prior = neural_network.predict_road(node.game_state, move[0], move[1], vertex_coords, last_settlement)
        else:
            prior = 1.0
        child = MCTS_Node(
            game_state = node.game_state,
            move = move, # For roads, the move is a tuple (edge, edge_key)
            parent = node,
            move_type = node.move_type
        )
        child.P = prior
        node.children[key] = child


def simulate_rollout(node, vertex_coords):
    '''
    When a leaf node is reached, use the neural network to estimate the value.
    For now, we use a one step evaluation.
    TODO: Implement a deeper implemention that simulates further moves.
    '''
    if node.move_type == "settlement":
        value, _ = neural_network.predict_settlement(node.game_state, node.move, vertex_coords)
    elif node.move_type == "road":
        last_settlement = node.game_state.get("last_settlement")
        value, _ = neural_network.predict_road(node.game_state, node.move[0], node.move[1], vertex_coords, last_settlement)
    else:
        value = 0.0
    return value


def backpropagate(node, value):
    '''
    Backpropagate the value estimated up the tree.
    '''
    while node is not None:
        node.N += 1
        node.W += value
        node.Q = node.W / node.N
        node = node.parent


def monte_carlo_tree_search(
    root,
    available_moves,
    vertex_coords,
    num_simulations,
    c_puct,
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
        expand_node(root, available_moves, vertex_coords)
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
            expand_node(node, available_moves, vertex_coords)
        # Complete evaluation by using the neural network to get the value estimate.
        value = simulate_rollout(node, vertex_coords)
        # Complete backpropagation by updating node statistics.
        backpropagate(node, value)
    # Choose the move from the root with the highest visit count / that was most visited.
    best_key, best_child = max(root.children.items(), key = lambda item: item[1].N)
    return best_child.move


def predict_best_settlement(game_state, available_vertices, vertex_coords, num_simulations = settings.num_simulations, c_puct = settings.c_puct):
    '''
    Run a full MCTS for settlement moves and return the best vertex.
    game_state: dictionary containing details of the current state
    available_vertices: list of vertex labels (e.g., "V01", "V02", ...) available for settlement
    vertex_coords: dictionary mapping vertex label to (x, y) coordinates / positions
    '''
    root = MCTS_Node(game_state = game_state, move_type = "settlement")
    best_vertex = monte_carlo_tree_search(
        root,
        available_vertices,
        vertex_coords,
        num_simulations,
        c_puct,
        add_dirichlet_noise = True
    )
    return best_vertex


def predict_best_road(game_state, available_edges, vertex_coords, num_simulations = settings.num_simulations, c_puct = settings.c_puct):
    '''
    Run a full MCTS for road moves and return the best edge and its key.
    available_edges: list of tuples (edge, edge_key) available for road placement
    game_state: dictionary that must include `last_settlement` for road evaluation
    '''
    last_settlement = game_state.get("last_settlement")
    if last_settlement is None:
        return None, None
    root = MCTS_Node(game_state, move_type = "road")
    best_move = monte_carlo_tree_search(
        root,
        available_edges,
        vertex_coords,
        num_simulations,
        c_puct,
        add_dirichlet_noise = True
    )
    best_edge, best_edge_key = best_move
    return best_edge, best_edge_key