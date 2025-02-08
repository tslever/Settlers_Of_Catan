import json
import os


DIRECTORY_OF_MODULE_BOARD = os.path.dirname(os.path.abspath(__file__))
DIRECTORY_OF_MODULE_GENERATE_BOARD_GEOMETRY = os.path.join(DIRECTORY_OF_MODULE_BOARD, "..", "..")
GEOMETRY_FILE = os.path.join(DIRECTORY_OF_MODULE_GENERATE_BOARD_GEOMETRY, "board_geometry.json")


def load_board_geometry():
    with open(GEOMETRY_FILE, "r") as f:
        return json.load(f)
    
board_geometry = load_board_geometry()

hexes = board_geometry["hexes"]
vertices_with_labels = board_geometry["vertices"]
all_edges_of_all_hexes = board_geometry["edges"]


def get_edge_key(v1, v2):
    (x1, y1) = v1
    (x2, y2) = v2
    if (
        x1 < x2 or
        (x1 == x2 and y1 <= y2)
    ):
        return f"{x1:.2f}-{y1:.2f}_{x2:.2f}-{y2:.2f}"
    else:
        return f"{x2:.2f}-{y2:.2f}_{x1:.2f}-{y1:.2f}"