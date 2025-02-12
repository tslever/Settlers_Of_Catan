#!/usr/bin/env python3
"""
ai_strategy.py

This module implements move selection for settlements and roads using an
AlphaGo Zero style Monte Carlo Tree Search (MCTS) that builds a simple tree.
TODO: Implement deeper tree expansion.
In a full implementation the neural network would
be trained and used to evaluate nonterminal states. Here we use a
network that simply uses a heuristic
(summing the pips on adjacent hexes) with a small amount of added noise.
TODO: Develop code that trains and uses a neural network.
"""


import math
from ai.mcts_node import MCTS_Node
from ai.neural_network import neural_network


def select_child(node, c_puct):
    '''
    Select a child of node that maximizes the UCB value.
    '''
    best_score = -float('inf')
    best_child = None
    for child in node.children.values():
        score = child.Q + c_puct * child.P * math.sqrt(node.N) / (1 + child.N)
        if score > best_score:
            best_score = score
            best_child = child
    return best_child


def expand_node(node, available_moves, vertex_coords):
    '''
    Expand the node by adding one child for every available move.
    For settlements, `available_moves` is a list of vertex labels.
    For roads, `available_moves` is a list of tuples (edge, edge_key).
    The key used in node.children is the move itself for settlements,
    but for roads we use the `edge_key` (a string) so that it's hashable.
    The prior probability for each move is obtained from the neural network.
    '''
    for move in available_moves:
        if node.move_type == "road":
            key = move[1]
        else:
            key = move
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
            move = move, # Store the full move (for roads, the (edge, edge_key) tuple)
            parent = node,
            move_type = node.move_type
        )
        child.P = prior
        node.children[key] = child


def simulate_rollout(node, vertex_coords):
    '''
    When a leaf node is reached, simulate a rollout by evaluating the move
    using the neural network's value prediction.
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
    Backpropagate the rollout value up the tree.
    '''
    while node is not None:
        node.N += 1
        node.W += value
        node.Q = node.W / node.N
        node = node.parent


def monte_carlo_tree_search(root, available_moves, vertex_coords, num_simulations, c_puct):
    '''
    Run MCTS simulations starting at the root node.
    First, expand the root with all available moves. Then, for each simulation:
    1. Complete selection by choosing the child with highest UCB until a leaf is reached.
    2. Complete expansion by expanding the leaf if the leaf is not already expanded.
    3. Complete evaluation by rolling out the leaf node using the neural network.
    4. Complete backpropagation by propagating the evaluation value back up the tree.
    Finally, return the move from the root with the highest visit count. 
    '''
    if root.is_leaf():
        expand_node(root, available_moves, vertex_coords)
    for _ in range(0, num_simulations):
        node = root
        while not node.is_leaf():
            node = select_child(node, c_puct)
        value = simulate_rollout(node, vertex_coords)
        backpropagate(node, value)
    _, best_child = max(root.children.items(), key = lambda item: item[1].N)
    return best_child.move


def predict_best_settlement(game_state, available_vertices, vertex_coords, num_simulations = 100, c_puct = 1.0):
    '''
    Run a full MCTS for settlement moves.
    game_state: dictionary containing details of the current state
    available_vertices: list of vertex labels (e.g., "V01", "V02", ...) available for settlement
    vertex_coords: dictionary mapping vertex label to (x, y) positions
    '''
    root = MCTS_Node(game_state = game_state, move_type = "settlement")
    best_vertex = monte_carlo_tree_search(root, available_vertices, vertex_coords, num_simulations, c_puct)
    return best_vertex


def predict_best_road(game_state, available_edges, vertex_coords, num_simulations = 100, c_puct = 1.0):
    '''
    Run a full MCTS for road moves.
    available_edges: list of tuples (edge, edge_key) available for road placement
    game_state: dictionary that must include `last_settlement` for road evaluation
    '''
    last_settlement = game_state.get("last_settlement")
    if last_settlement is None:
        return None, None
    root = MCTS_Node(game_state, move_type = "road")
    best_move = monte_carlo_tree_search(root, available_edges, vertex_coords, num_simulations, c_puct)
    best_edge, best_edge_key = best_move
    return best_edge, best_edge_key