import json
import math
import os
from typing import List, Dict, Optional, Tuple

# -- Constants (extracted from the original module) --
MARGIN_OF_ERROR = 0.01
WIDTH_OF_BOARD_IN_VMIN = 100  # vmin
NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6
RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX = math.tan(math.pi / 6)
RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX

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

WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD
HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX

# -- The Board class --
class Board:
    def __init__(self, geometry_file: Optional[str] = None):
        if geometry_file is None:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            geometry_file = os.path.join(base_dir, "..", "..", "board_geometry.json")
        with open(geometry_file, "r") as f:
            data = json.load(f)
        self.hexes: List[Dict] = data["hexes"]
        self.vertices: List[Dict] = data["vertices"]
        self.edges: List[Dict] = data["edges"]

    @staticmethod
    def get_edge_key(v1: Tuple[float, float], v2: Tuple[float, float]) -> str:
        (x1, y1) = v1
        (x2, y2) = v2
        if x1 < x2 or (x1 == x2 and y1 <= y2):
            return f"{x1:.2f}-{y1:.2f}_{x2:.2f}-{y2:.2f}"
        else:
            return f"{x2:.2f}-{y2:.2f}_{x1:.2f}-{y1:.2f}"

    def get_hex_vertices(self, hex_tile: Dict) -> List[Tuple[float, float]]:
        x = hex_tile["x"]
        y = hex_tile["y"]
        return [
            (x + 0.5 * WIDTH_OF_HEX, y),
            (x + WIDTH_OF_HEX, y + 0.25 * HEIGHT_OF_HEX),
            (x + WIDTH_OF_HEX, y + 0.75 * HEIGHT_OF_HEX),
            (x + 0.5 * WIDTH_OF_HEX, y + HEIGHT_OF_HEX),
            (x, y + 0.75 * HEIGHT_OF_HEX),
            (x, y + 0.25 * HEIGHT_OF_HEX)
        ]

    def get_vertex_by_label(self, label: str) -> Optional[Dict]:
        for vertex in self.vertices:
            if vertex["label"] == label:
                return vertex
        return None

    def get_vertex_features(self, vertex_label: str) -> Optional[List[float]]:
        """
        Compute a feature vector for the given vertex label.
        Features (all normalized):
          1. Normalized pip sum: total_pips / (hex_count * 5)
          2. Normalized x: x / WIDTH_OF_BOARD_IN_VMIN
          3. Normalized y: y / 100.0
          4. Normalized adjacent hex count: hex_count / 3.0
          5. Bias term: 1.0
        """
        vertex = self.get_vertex_by_label(vertex_label)
        if vertex is None:
            return None
        x, y = vertex["x"], vertex["y"]
        total_pips = 0
        hex_count = 0
        for hex_tile in self.hexes:
            vertices = self.get_hex_vertices(hex_tile)
            for vx, vy in vertices:
                if math.isclose(vx, x, abs_tol=MARGIN_OF_ERROR) and math.isclose(vy, y, abs_tol=MARGIN_OF_ERROR):
                    hex_id = hex_tile["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        total_pips += TOKEN_DOT_MAPPING.get(token, 0)
                        hex_count += 1
                    break
        normalized_pip = total_pips / (hex_count * 5) if hex_count > 0 else 0.0
        normalized_x = x / WIDTH_OF_BOARD_IN_VMIN
        normalized_y = y / 100.0
        normalized_hex_count = hex_count / 3.0
        return [normalized_pip, normalized_x, normalized_y, normalized_hex_count, 1.0]

    def get_available_settlement_moves(self, used_vertices: List[str]) -> List[str]:
        """Return a list of vertex labels available for settlement placement."""
        used_coords = []
        for label in used_vertices:
            vertex = self.get_vertex_by_label(label)
            if vertex:
                used_coords.append((vertex["x"], vertex["y"]))
        available = []
        threshold = 2.0  # Euclidean distance threshold
        for vertex in self.vertices:
            label = vertex["label"]
            if label in used_vertices:
                continue
            x, y = vertex["x"], vertex["y"]
            too_close = False
            for ex, ey in used_coords:
                if math.sqrt((x - ex) ** 2 + (y - ey) ** 2) < threshold:
                    too_close = True
                    break
            if not too_close:
                available.append(label)
        return available

    def get_available_road_moves(self, last_settlement: str, used_edge_keys: List[str]) -> List[Tuple[Dict, str]]:
        """Return a list of (edge, edge_key) tuples adjacent to the given settlement."""
        vertex = self.get_vertex_by_label(last_settlement)
        if vertex is None:
            return []
        settlement_coord = (vertex["x"], vertex["y"])
        adjacent_edges = []
        for edge in self.edges:
            v1 = (edge["x1"], edge["y1"])
            v2 = (edge["x2"], edge["y2"])
            if (
                (math.isclose(v1[0], settlement_coord[0], abs_tol=MARGIN_OF_ERROR) and math.isclose(v1[1], settlement_coord[1], abs_tol=MARGIN_OF_ERROR)) or
                (math.isclose(v2[0], settlement_coord[0], abs_tol=MARGIN_OF_ERROR) and math.isclose(v2[1], settlement_coord[1], abs_tol=MARGIN_OF_ERROR))
            ):
                edge_key = self.get_edge_key(v1, v2)
                if edge_key not in used_edge_keys:
                    adjacent_edges.append((edge, edge_key))
        return adjacent_edges