import math

BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]

WIDTH_OF_BOARD_IN_VMIN = 100 # vmin
NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6
WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD
RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = math.tan(math.pi / 6)
RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON
HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX
DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW = HEIGHT_OF_HEX / 4
DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW = HEIGHT_OF_HEX - DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW
NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST = len(BOARD) - 1
HEIGHT_OF_BOARD = HEIGHT_OF_HEX + DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW * NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST
VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES = (100 - HEIGHT_OF_BOARD) / 2 # vmin
MARGIN_OF_ERROR = 0.01


def edge_already_exists(e1, edges, margin_of_error):
    (x1_of_e1, y1_of_e1, x2_of_e1, y2_of_e1) = e1
    for e2 in edges:
        (x1_of_e2, y1_of_e2, x2_of_e2, y2_of_e2) = e2
        if (
            (
                abs(x1_of_e1 - x1_of_e2) < margin_of_error and
                abs(y1_of_e1 - y1_of_e2) < margin_of_error and
                abs(x2_of_e1 - x2_of_e2) < margin_of_error and
                abs(y2_of_e1 - y2_of_e2) < margin_of_error
            ) or
            (
                abs(x1_of_e1 - x2_of_e2) < margin_of_error and
                abs(y1_of_e1 - y2_of_e2) < margin_of_error and
                abs(x2_of_e1 - x1_of_e2) < margin_of_error and
                abs(y2_of_e1 - y1_of_e2) < margin_of_error
            )
        ):
            return True
    return False


def vertex_already_exists(v1, vertices, margin_of_error):
    (x_of_v1, y_of_v1) = v1
    for v2 in vertices:
        (x_of_v2, y_of_v2) = v2
        if (
            abs(x_of_v2 - x_of_v1) < margin_of_error and
            abs(y_of_v2 - y_of_v1) < margin_of_error
        ):
            return True
    return False


def get_edge_key(v1, v2):
    (x_of_v1, y_of_v1) = v1
    (x_of_v2, y_of_v2) = v2
    if (
        x_of_v1 < x_of_v2 or
        (x_of_v1 == x_of_v2 and y_of_v1 <= y_of_v2)
    ):
        return f"{x_of_v1:.2f}-{y_of_v1:.2f}_{x_of_v2:.2f}-{y_of_v2:.2f}"
    else:
        return f"{x_of_v2:.2f}-{y_of_v2:.2f}_{x_of_v1:.2f}-{y_of_v1:.2f}"


def get_list_of_hexes():
    hexes = []
    for index_of_row, row in enumerate(BOARD):
        vertical_position_of_hex = VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES + index_of_row * DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW
        number_of_hexes_in_row = len(row)
        horizontal_position_of_first_hex_in_row = (WIDTH_OF_BOARD_IN_VMIN - number_of_hexes_in_row * WIDTH_OF_HEX) / 2 # vmin
        for index_of_hex, id_of_hex in enumerate(row):
            horizontal_position_of_hex = horizontal_position_of_first_hex_in_row + index_of_hex * WIDTH_OF_HEX
            hexes.append((id_of_hex, horizontal_position_of_hex, vertical_position_of_hex))
    return hexes


def get_list_of_vertices(hex):
    (_, horizontal_position_of_hex, vertical_position_of_hex) = hex
    vertices = [
        (horizontal_position_of_hex + 0.5 * WIDTH_OF_HEX, vertical_position_of_hex),
        (horizontal_position_of_hex + WIDTH_OF_HEX, vertical_position_of_hex + 0.25 * HEIGHT_OF_HEX),
        (horizontal_position_of_hex + WIDTH_OF_HEX, vertical_position_of_hex + 0.75 * HEIGHT_OF_HEX),
        (horizontal_position_of_hex + 0.5 * WIDTH_OF_HEX, vertical_position_of_hex + HEIGHT_OF_HEX),
        (horizontal_position_of_hex, vertical_position_of_hex + 0.75 * HEIGHT_OF_HEX),
        (horizontal_position_of_hex, vertical_position_of_hex + 0.25 * HEIGHT_OF_HEX)
    ]
    return vertices


def get_all_edges_of_all_hexes():
    hexes = get_list_of_hexes()
    edges = []
    for hex in hexes:
        vertices = get_list_of_vertices(hex)
        for i in range(0, len(vertices)):
            v1 = vertices[i]
            v2 = vertices[(i + 1) % len(vertices)]
            (x_of_v1, y_of_v1) = v1
            (x_of_v2, y_of_v2) = v2
            edge = (x_of_v1, y_of_v1, x_of_v2, y_of_v2)
            if not edge_already_exists(edge, edges, MARGIN_OF_ERROR):
                edges.append(edge)
    return edges


def get_vertices_with_labels():
    hexes = get_list_of_hexes()
    unique_vertices = []
    for hex in hexes:
        potential_vertices = get_list_of_vertices(hex)
        for v in potential_vertices:
            if not vertex_already_exists(v, unique_vertices, MARGIN_OF_ERROR):
                unique_vertices.append(v)
    vertices_with_labels = [(f"V{(i+1):02d}", v[0], v[1]) for i, v in enumerate(unique_vertices)]
    return vertices_with_labels