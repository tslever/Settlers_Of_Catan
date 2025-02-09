import json
import math
import os


DIRECTORY_OF_MODULE_BOARD = os.path.dirname(os.path.abspath(__file__))
DIRECTORY_OF_MODULE_GENERATE_BOARD_GEOMETRY = os.path.join(DIRECTORY_OF_MODULE_BOARD, "..", "..")
GEOMETRY_FILE = os.path.join(DIRECTORY_OF_MODULE_GENERATE_BOARD_GEOMETRY, "board_geometry.json")
MARGIN_OF_ERROR = 0.01
NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6
RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = math.tan(math.pi / 6)
RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON
TOKEN_DOT_MAPPING = {
    2: 1,
    3: 2,
    4: 3,
    5: 4,
    6: 5,
    8: 5,
    9: 4,
    10: 3,
    11: 2,
    12: 1
}
TOKEN_MAPPING = {
    "H01": 10,
    "H02": 2,
    "H03": 9,
    "H04": 12,
    "H05": 6,
    "H06": 4,
    "H07": 10,
    "H08": 9,
    "H09": 11,
    "H10": None,
    "H11": 3,
    "H12": 8,
    "H13": 8,
    "H14": 3,
    "H15": 4,
    "H16": 5,
    "H17": 5,
    "H18": 6,
    "H19": 11
}
WIDTH_OF_BOARD_IN_VMIN = 100  # vmin
WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD
HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX


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


def get_hex_vertices(hex):
    x = hex["x"]
    y = hex["y"]
    return [
        (x + 0.5 * WIDTH_OF_HEX, y),
        (x + WIDTH_OF_HEX, y + 0.25 * HEIGHT_OF_HEX),
        (x + WIDTH_OF_HEX, y + 0.75 * HEIGHT_OF_HEX),
        (x + 0.5 * WIDTH_OF_HEX, y + HEIGHT_OF_HEX),
        (x, y + 0.75 * HEIGHT_OF_HEX),
        (x, y + 0.25 * HEIGHT_OF_HEX)
    ]