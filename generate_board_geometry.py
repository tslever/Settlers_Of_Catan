#!/usr/bin/env python3
import json
import logging
import math
import os

# ----------------------------
# Board Layout and Constants
# ----------------------------

# Define the board as rows of hex tile IDs.
BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]

WIDTH_OF_BOARD_IN_VMIN = 100  # vmin
NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6
WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD
RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEX = math.tan(math.pi / 6)
RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEX
HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX
DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW = HEIGHT_OF_HEX / 4
DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW = HEIGHT_OF_HEX - DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW
NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST = len(BOARD) - 1
HEIGHT_OF_BOARD = HEIGHT_OF_HEX + DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW * NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST
VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES = (100 - HEIGHT_OF_BOARD) / 2
MARGIN_OF_ERROR = 0.01


logging.basicConfig(level = logging.INFO)
logger = logging.getLogger(__name__)


def get_list_of_hexes():
    hexes = []
    for row_index, row in enumerate(BOARD):
        vertical_position = VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES + row_index * DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW
        number_of_hexes = len(row)
        horizontal_position_first_hex = (WIDTH_OF_BOARD_IN_VMIN - number_of_hexes * WIDTH_OF_HEX) / 2
        for hex_index, hex_id in enumerate(row):
            horizontal_position = horizontal_position_first_hex + hex_index * WIDTH_OF_HEX
            hexes.append((hex_id, horizontal_position, vertical_position))
    return hexes

def get_list_of_vertices(hex_tuple):
    _, hex_x, hex_y = hex_tuple
    vertices = [
        (hex_x + 0.5 * WIDTH_OF_HEX, hex_y),
        (hex_x + WIDTH_OF_HEX, hex_y + 0.25 * HEIGHT_OF_HEX),
        (hex_x + WIDTH_OF_HEX, hex_y + 0.75 * HEIGHT_OF_HEX),
        (hex_x + 0.5 * WIDTH_OF_HEX, hex_y + HEIGHT_OF_HEX),
        (hex_x, hex_y + 0.75 * HEIGHT_OF_HEX),
        (hex_x, hex_y + 0.25 * HEIGHT_OF_HEX)
    ]
    return vertices

def edge_already_exists(edge, edge_list, margin):
    (x1, y1, x2, y2) = edge
    for (ex1, ey1, ex2, ey2) in edge_list:
        if ((abs(x1 - ex1) < margin and abs(y1 - ey1) < margin and
             abs(x2 - ex2) < margin and abs(y2 - ey2) < margin) or
            (abs(x1 - ex2) < margin and abs(y1 - ey2) < margin and
             abs(x2 - ex1) < margin and abs(y2 - ey1) < margin)):
            return True
    return False

def get_all_edges(hexes):
    edges = []
    for hex_tile in hexes:
        vertices = get_list_of_vertices(hex_tile)
        num_vertices = len(vertices)
        for i in range(num_vertices):
            v1 = vertices[i]
            v2 = vertices[(i + 1) % num_vertices]
            edge = (v1[0], v1[1], v2[0], v2[1])
            if not edge_already_exists(edge, edges, MARGIN_OF_ERROR):
                edges.append(edge)
    return edges

def vertex_already_exists(vertex, vertices, margin):
    (x, y) = vertex
    for (vx, vy) in vertices:
        if abs(x - vx) < margin and abs(y - vy) < margin:
            return True
    return False

def get_unique_vertices(hexes):
    unique_vertices = []
    for hex_tile in hexes:
        for vertex in get_list_of_vertices(hex_tile):
            if not vertex_already_exists(vertex, unique_vertices, MARGIN_OF_ERROR):
                unique_vertices.append(vertex)
    return unique_vertices

def get_vertices_with_labels(hexes):
    unique_vertices = get_unique_vertices(hexes)
    vertices_with_labels = []
    for i, (x, y) in enumerate(unique_vertices):
        label = f"V{i+1:02d}"
        vertices_with_labels.append({
            "label": label,
            "x": x,
            "y": y
        })
    return vertices_with_labels

def generate_board_geometry():
    hexes_raw = get_list_of_hexes()
    hexes_data = [{"id": hex_id, "x": x, "y": y} for (hex_id, x, y) in hexes_raw]
    vertices_data = get_vertices_with_labels(hexes_raw)
    edges_raw = get_all_edges(hexes_raw)
    edges_data = [{"x1": x1, "y1": y1, "x2": x2, "y2": y2} for (x1, y1, x2, y2) in edges_raw]
    geometry = {
        "hexes": hexes_data,
        "vertices": vertices_data,
        "edges": edges_data
    }
    
    return geometry


def main():
    geometry = generate_board_geometry()
    number_of_hexes = len(geometry["hexes"])
    number_of_vertices = len(geometry["vertices"])
    number_of_edges = len(geometry["edges"])
    logger.info(f"Geometry generated with {number_of_hexes} hexes, {number_of_vertices} vertices, {number_of_edges} edges")
    output_dir = os.path.dirname(os.path.abspath(__file__))
    output_file = os.path.join(output_dir, "board_geometry.json")
    with open(output_file, "w") as f:
        json.dump(geometry, f, indent = 4)
    logger.info(f"Board geometry generated and written to {output_file}")

if __name__ == "__main__":
    main()