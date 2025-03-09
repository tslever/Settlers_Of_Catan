from typing import List
from typing import Dict
from typing import Optional
from typing import Tuple
import json
import math
import numpy as np
from back_end.settings import settings


RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX = math.tan(math.pi / 6)
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

RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX
WIDTH_OF_HEX = settings.width_of_board_in_vmin / settings.number_of_hexes_that_span_board
HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX
LENGTH_OF_SIDE_OF_HEX = WIDTH_OF_HEX * RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX


def euclidean_distance_between_vertices_with_coordinates(x1: float, y1: float, x2: float, y2: float) -> float:
    return math.sqrt((x1 - x2)**2 + (y1 - y2)**2)


class Board:

    def __init__(self, geometry_file: Optional[str] = None):
        if geometry_file is None:
            geometry_file = settings.board_geometry_path
        with open(geometry_file, "r") as f:
            data = json.load(f)
        self.hexes: List[Dict] = data["hexes"]
        self.vertices: List[Dict] = data["vertices"]
        self.edges: List[Dict] = data["edges"]

        self._hex_vertex_cache: Dict[str, List[Tuple[float, float]]] = {}

        self._vertex_feature_map = {}
        for vertex in self.vertices:
            self._vertex_feature_map[vertex["label"]] = self.compute_vertex_feature(vertex)


    @staticmethod
    def get_edge_key(v1: Tuple[float, float], v2: Tuple[float, float]) -> str:
        (x1, y1) = v1
        (x2, y2) = v2
        # Sorts the two endpoints so that edge direction is irrelevant.
        if x1 < x2 or (x1 == x2 and y1 <= y2):
            return f"{x1:.2f}-{y1:.2f}_{x2:.2f}-{y2:.2f}"
        else:
            return f"{x2:.2f}-{y2:.2f}_{x1:.2f}-{y1:.2f}"


    def compute_vertex_feature(self, dictionary_of_vertex_information):
        x, y = dictionary_of_vertex_information["x"], dictionary_of_vertex_information["y"]
        total_number_of_pips = 0
        number_of_hexes = 0
        array_of_coordinates = np.array([x, y])
        for dictionary_of_hex_information in self.hexes:
            array_of_coordinates_of_vertices_of_hex = np.array(self.get_hex_vertices(dictionary_of_hex_information))
            if np.any(
                np.all(
                    np.abs(array_of_coordinates_of_vertices_of_hex - array_of_coordinates) < settings.margin_of_error,
                    axis = 1
                )
            ):
                id_of_hex = dictionary_of_hex_information["id"]
                number_of_token = TOKEN_MAPPING.get(id_of_hex)
                if number_of_token is not None:
                    number_of_pips = TOKEN_DOT_MAPPING.get(number_of_token, 0)
                    total_number_of_pips += number_of_pips
                    number_of_hexes += 1
        normalized_total_number_of_pips = total_number_of_pips / (number_of_hexes * 5) if number_of_hexes > 0 else 0.0
        normalized_x_coordinate = x / settings.width_of_board_in_vmin
        normalized_y_coordinate = y / 100.0
        normalized_number_of_hexes = number_of_hexes / 3.0
        return [normalized_total_number_of_pips, normalized_x_coordinate, normalized_y_coordinate, normalized_number_of_hexes, 1.0]


    def get_hex_vertices(self, hex_tile: Dict) -> List[Tuple[float, float]]:
        '''
        Given a dictionary of hex information, return a list of pairs of coordinates of the vertices of the hex.
        This function caches the result so that repeated calls with the same hex (determined by its ID)
        do not recompute the vertex coordinates.
        '''
        hex_id = hex_tile.get("id")
        if hex_id in self._hex_vertex_cache:
            return self._hex_vertex_cache[hex_id]

        x = hex_tile["x"]
        y = hex_tile["y"]
        vertices = [
            (x + 0.5 * WIDTH_OF_HEX, y),
            (x + WIDTH_OF_HEX, y + 0.25 * HEIGHT_OF_HEX),
            (x + WIDTH_OF_HEX, y + 0.75 * HEIGHT_OF_HEX),
            (x + 0.5 * WIDTH_OF_HEX, y + HEIGHT_OF_HEX),
            (x, y + 0.75 * HEIGHT_OF_HEX),
            (x, y + 0.25 * HEIGHT_OF_HEX)
        ]
        self._hex_vertex_cache[hex_id] = vertices
        return vertices


    def get_vertex_by_label(self, label: str) -> Optional[Dict]:
        for vertex in self.vertices:
            if vertex["label"] == label:
                return vertex
        return None


    def get_vertex_features(self, vertex_label: str) -> Optional[List[float]]:
        '''
        Return the precomputed feature vector for the given vertex label.
        '''
        return self._vertex_feature_map.get(vertex_label)


    def get_available_building_moves(self, list_of_labels_of_occupied_vertices: List[str]) -> List[str]:
        '''
        Return the list of labels of vertices that are not too close to any vertex in the given list of labels of occupied vertices.
        Two vertices are considered too close if the Euclidean distance between them is less than the length of the side of a hex plus a small margin.
        '''
        list_of_tuples_of_coordinates_of_occupied_vertices = []
        for label in list_of_labels_of_occupied_vertices:
            dictionary_of_vertex_information = self.get_vertex_by_label(label)
            if dictionary_of_vertex_information:
                x = dictionary_of_vertex_information["x"]
                y = dictionary_of_vertex_information["y"]
                tuple_of_coordinates_of_occupied_vertex = (x, y)
                list_of_tuples_of_coordinates_of_occupied_vertices.append(tuple_of_coordinates_of_occupied_vertex)
        list_of_labels_of_available_vertices = []
        for dictionary_of_vertex_information in self.vertices:
            label = dictionary_of_vertex_information["label"]
            if label in list_of_labels_of_occupied_vertices:
                continue
            x1 = dictionary_of_vertex_information["x"]
            y1 = dictionary_of_vertex_information["y"]
            if any(
                euclidean_distance_between_vertices_with_coordinates(x1, y1, x2, y2) < (LENGTH_OF_SIDE_OF_HEX + settings.margin_of_error)
                for x2, y2 in list_of_tuples_of_coordinates_of_occupied_vertices
            ):
                continue
            list_of_labels_of_available_vertices.append(label)
        return list_of_labels_of_available_vertices


    def get_available_road_moves(self, last_settlement_or_city: str, used_edge_keys: List[str]) -> List[Tuple[Dict, str]]:
        '''
        Return a list of (edge, edge_key) tuples adjacent to the given settlement or city.
        '''
        vertex = self.get_vertex_by_label(last_settlement_or_city)
        if vertex is None:
            return []
        settlement_or_city_coord = (vertex["x"], vertex["y"])
        adjacent_edges = []
        for edge in self.edges:
            v1 = (edge["x1"], edge["y1"])
            v2 = (edge["x2"], edge["y2"])
            if (
                (math.isclose(v1[0], settlement_or_city_coord[0], abs_tol = settings.margin_of_error) and math.isclose(v1[1], settlement_or_city_coord[1], abs_tol = settings.margin_of_error)) or
                (math.isclose(v2[0], settlement_or_city_coord[0], abs_tol = settings.margin_of_error) and math.isclose(v2[1], settlement_or_city_coord[1], abs_tol = settings.margin_of_error))
            ):
                edge_key = self.get_edge_key(v1, v2)
                if edge_key not in used_edge_keys:
                    adjacent_edges.append((edge, edge_key))
        return adjacent_edges