from back_end.ai.mcts.node import MCTS_Node


def expand_node(
    node,
    list_of_labels_of_available_vertices_or_tuples_of_information_re_available_edges, # `list_of_labels_of_available_vertices_or_tuples_of_information_re_available_edges` represents a list of available moves.
    dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
    neural_network
):
    # `label_of_available_vertex_or_tuple_of_information_re_available_edge` represents an available move.
    for label_of_available_vertex_or_tuple_of_information_re_available_edge in list_of_labels_of_available_vertices_or_tuples_of_information_re_available_edges:
        # Determine the key of the move based on the type of the move.
        # If the move is placing a settlement, the key is a label of an available vertex.
        # If the move is placing a road, the key is an edge key.
        key = label_of_available_vertex_or_tuple_of_information_re_available_edge if node.move_type != "road" else label_of_available_vertex_or_tuple_of_information_re_available_edge[1]
        if key in node.children:
            continue
        # Evaluate the move via the neural network.
        if node.move_type == "city":
            _, prior_probability = neural_network.evaluate_city(label_of_available_vertex_or_tuple_of_information_re_available_edge)
        elif node.move_type == "road":
            label_of_vertex_of_last_settlement = node.game_state.get("last_settlement")
            dictionary_of_coordinates_of_available_edge = label_of_available_vertex_or_tuple_of_information_re_available_edge[0]
            _, prior_probability = neural_network.evaluate_road(
                dictionary_of_coordinates_of_available_edge,
                dictionary_of_labels_of_vertices_and_tuples_of_coordinates,
                label_of_vertex_of_last_settlement
            )
        elif node.move_type == "settlement":
            _, prior_probability = neural_network.evaluate_settlement(label_of_available_vertex_or_tuple_of_information_re_available_edge)
        else:
            prior_probability = 1.0
        # Create and store a child node.
        child = MCTS_Node(
            game_state = node.game_state,
            move = label_of_available_vertex_or_tuple_of_information_re_available_edge,
            parent = node,
            move_type = node.move_type
        )
        child.P = prior_probability
        node.children[key] = child