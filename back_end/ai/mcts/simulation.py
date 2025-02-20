def simulate_rollout(node, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, neural_network):
    '''
    When a leaf node is reached, use the neural network to estimate the value.
    Currently, this function performs a one step evaluation.
    TODO: What is a one step evaluation?
    TODO: Implement a deeper implemention that simulates further moves.
    '''
    if node.move_type == "settlement":
        value, _ = neural_network.evaluate_settlement(node.move)
    elif node.move_type == "city":
        value, _ = neural_network.evaluate_city(node.move)
    elif node.move_type == "road":
        label_of_vertex_of_last_settlement = node.game_state.get("last_settlement")
        dictionary_of_coordinates_of_available_edge = node.move[0]
        value, _ = neural_network.evaluate_road(
            dictionary_of_coordinates_of_available_edge,
            dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
            label_of_vertex_of_last_settlement
        )
    else:
        value = 0.0
    return value