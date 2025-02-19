#!/usr/bin/env python3
from typing import Dict
from typing import List
from typing import Tuple
from back_end.utilities.board import HEIGHT_OF_HEX
from back_end.utilities.board import MARGIN_OF_ERROR
from back_end.utilities.board import WIDTH_OF_BOARD_IN_VMIN
from back_end.utilities.board import WIDTH_OF_HEX
from back_end.utilities.board import Board
import json
import logging
from back_end.logger import set_up_logging
from back_end.settings import settings


set_up_logging()
logger = logging.getLogger(__name__)


DEFAULT_BOARD_LAYOUT: List[List[str]] = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]


class BoardGeometryGenerator:

    def __init__(
        self,
        board_layout: List[List[str]],
        board_width: float,
        hex_width: float,
        hex_height: float,
        margin_of_error: float
    ):
        self.board_layout = board_layout,
        self.board_width = board_width,
        self.hex_width = hex_width,
        self.hex_height = hex_height,
        self.margin_of_error = margin_of_error

        self.distance_between_bottom_of_hex_in_first_row_and_top_of_hex_in_second_row = self.hex_height / 4
        self.distance_between_top_of_hex_in_one_row_and_top_of_hex_in_next_row = self.hex_height - self.distance_between_bottom_of_hex_in_first_row_and_top_of_hex_in_second_row
        self.number_of_rows_of_hexes_after_first = len(self.board_layout) - 1
        self.height_of_board = self.hex_height + self.distance_between_top_of_hex_in_one_row_and_top_of_hex_in_next_row * self.number_of_rows_of_hexes_after_first
        self.vertical_position_of_first_row_of_hexes = (100 - self.height_of_board) / 2


    def generate_hexes(self) -> List[Dict[str, str | float]]:
        '''
        Generate a list of dictionaries of hex information.
        Each dictionary contains a hex's ID, horizontal position of left edge, and vertical position of top vertex.
        '''
        list_of_dictionaries_of_hex_information: List[Dict[str, str | float]] = []
        for index_of_row, row in enumerate(self.board_layout):
            vertical_position = self.vertical_position_of_first_row_of_hexes + index_of_row * self.distance_between_top_of_hex_in_one_row_and_top_of_hex_in_next_row
            number_of_hexes = len(row)
            horizontal_position = (self.board_width - number_of_hexes * self.hex_width) / 2
            for index_of_hex, id_of_hex in enumerate(row):
                x = horizontal_position + index_of_hex * self.hex_width
                y = vertical_position
                dictionary_of_hex_information = {"id": id_of_hex, "x": x, "y": y}
                list_of_dictionaries_of_hex_information.append(dictionary_of_hex_information)
        return list_of_dictionaries_of_hex_information


    def _coordinates_are_close(self, a: float, b: float) -> bool:
        return abs(a - b) < self.margin_of_error


    def _tuples_of_coordinates_are_close(
        self,
        tuple_of_coordinates_1: Tuple[float, float],
        tuple_of_coordinates_2: Tuple[float, float]
    ) -> bool:
        x1, y1 = tuple_of_coordinates_1
        x2, y2 = tuple_of_coordinates_2
        indicator_of_whether_tuples_of_coordinates_are_close = (
            self._coordinates_are_close(x1, x2) and self._coordinates_are_close(y1, y2)
        )
        return indicator_of_whether_tuples_of_coordinates_are_close


    def _edge_already_exists(
        self,
        tuple_of_edge_coordinates: Tuple[float, float, float, float],
        list_of_tuples_of_edge_coordinates: List[Tuple[float, float, float, float]]
    ) -> bool:
        '''
        Determine whether the given tuple of edge coordinates
        is already present in a given list of tuples of edges coordinates.

        Arguments:
            tuple_of_edge_coordinates: tuple -- a tuple of edge coordinates of the form (x1, y1, x2, y2)
            list_of_tuples_of_edge_coordinates: list[tuple] -- a list of tuples of edge coordinates
            margin of error: float -- tolerance for equality of two coordinates

            Returns:
                True if the given tuple of edge coordinates matches a tuple of edge coordinates in the given list;
                False otherwise
        '''
        x11, y11, x12, y12 = tuple_of_edge_coordinates
        for x21, y21, x22, y22 in list_of_tuples_of_edge_coordinates:
            if (
                (self._tuples_of_coordinates_are_close((x11, y11), (x21, y21)) and
                self._tuples_of_coordinates_are_close((x12, y12), (x22, y22))) or
                (self._tuples_of_coordinates_are_close((x11, y11), (x22, y22)) and
                self._tuples_of_coordinates_are_close((x12, y12), (x21, y21)))
            ):
                return True
        return False


    def generate_edges(
        self,
        board: Board,
        list_of_dictionaries_of_hex_information: List[Dict[str, str | float]]
    ) -> List[Dict[str, float]]:
        '''
        Provide a list of tuples of edge coordinates for all unique edges
        for the hexes represented by the given list of dictionaries of hex information.

        Arguments:
            list_of_dictionaries_of_hex_information: List[Dict[str, str | float]] --
                list of dictionaries of hex information. Each dictionary includes keys "id", "x", and "y".
        
        Returns:
            list_of_tuples_of_edge_coordinates: List[Tuple[float, float]] --
                a list of tuples of edge coordinates. Each tuple is of the form (x1, y1, x2, y2).
        '''
        list_of_tuples_of_edge_coordinates: List[Tuple[float, float, float, float]] = []
        for dictionary_of_hex_information in list_of_dictionaries_of_hex_information:
            list_of_pairs_of_coordinates_of_vertices_of_hex: List[Tuple[float, float]] = board.get_hex_vertices(dictionary_of_hex_information)
            number_of_vertices_of_hex: int = len(list_of_pairs_of_coordinates_of_vertices_of_hex)
            for i in range(0, number_of_vertices_of_hex):
                v1: Tuple[float, float] = list_of_pairs_of_coordinates_of_vertices_of_hex[i]
                v2: Tuple[float, float] = list_of_pairs_of_coordinates_of_vertices_of_hex[(i + 1) % number_of_vertices_of_hex]
                x1, y1 = v1
                x2, y2 = v2
                tuple_of_edge_coordinates: Tuple[float, float, float, float] = (x1, y1, x2, y2)
                if not self._edge_already_exists(tuple_of_edge_coordinates, list_of_tuples_of_edge_coordinates):
                    list_of_tuples_of_edge_coordinates.append(tuple_of_edge_coordinates)
        list_of_dictionaries_of_edge_coordinates = [
            {"x1": e[0], "y1": e[1], "x2": e[2], "y2": e[3]} for e in list_of_tuples_of_edge_coordinates
        ]
        return list_of_dictionaries_of_edge_coordinates


    def _vertex_already_exists(
        self,
        tuple_of_vertex_coordinates: Tuple[float, float],
        list_of_tuples_of_vertex_coordinates: List[Tuple[float, float]]
    ) -> bool:
        '''
        Check whether a tuple of vertex coordinates is matches a tuple of vertex coordinates in the given list.

        Arguments:
            tuple_of_vertex_coordinates: Tuple[float, float] -- a tuple of vertex coordinates of the form (x1, y1)
            list_of_tuples_of_vertex_coordinates: List[Tuple[float, float]] -- a list of tuples of vertex coordinates
            margin_of_error: float -- tolerance for equality of two coordinates

        Returns:
            True if the given tuple of vertex coordinates matches a tuple in the list; False otherwise
        '''
        for tuple_of_vertex_coordinates in list_of_tuples_of_vertex_coordinates:
            if self._tuples_of_coordinates_are_close(tuple_of_vertex_coordinates, tuple_of_vertex_coordinates):
                return True
        return False


    def generate_vertices(
        self,
        board: Board,
        list_of_dictionaries_of_hex_information: List[Dict[str, str | float]]
    ) -> List[Dict[str, float]]:
        '''
        Provide a list of tuples of coordinates of unique vertices
        for the hexes represented by the given list of dictionaries of hex information.

        Arguments:
            list_of_dictionaries_of_hex_information: List[Dict[str, str | float]] -- list of dictionaries of hex information
        
        Returns:
            list_of_tuples_of_coordinates_of_unique_vertices: List[Tuple[float, float]] --
                list of tuples of coordinates of unique vertices. Each tuple is of the form (x, y).
        '''
        list_of_tuples_of_coordinates_of_unique_vertices: List[Tuple[float, float]] = []
        for dictionary_of_hex_information in list_of_dictionaries_of_hex_information:
            for tuple_of_coordinates_of_vertex in board.get_hex_vertices(dictionary_of_hex_information):
                if not self._vertex_already_exists(tuple_of_coordinates_of_vertex, list_of_tuples_of_coordinates_of_unique_vertices):
                    list_of_tuples_of_coordinates_of_unique_vertices.append(tuple_of_coordinates_of_vertex)
        list_of_dictionaries_of_vertex_information = []
        for index_of_tuple_of_coordinates_of_unique_vertices, (x, y) in enumerate(list_of_tuples_of_coordinates_of_unique_vertices):
            label = f"V{index_of_tuple_of_coordinates_of_unique_vertices + 1:02d}"
            dictionary_of_vertex_information = {"label": label, "x": x, "y": y}
            list_of_dictionaries_of_vertex_information.append(dictionary_of_vertex_information)
        return list_of_dictionaries_of_vertex_information


    def generate_board_geometry(self, board: Board) -> Dict[str, List[Dict[str, str | float] | Dict[str, float]]]:
        '''
        Generate the full board geometry, including hex information, vertex information, and edge coordinates.

        Returns:
            dictionary_of_board_geometry: Dict[str, List[Dict[str, str | float] | Dict[str, float]]] --
                dictionary of board geometry with keys "hexes", "vertices", and "edges"
        '''
        list_of_dictionaries_of_hex_information: List[Dict[str, str | float]] = self.generate_hexes()
        list_of_dictionaries_of_information_re_unique_vertices: List[Dict[str, str | float]] = self.generate_vertices(board, list_of_dictionaries_of_hex_information)
        list_of_dictionaries_of_edge_coordinates: List[Dict[str, float]] = self.generate_edges(board, list_of_dictionaries_of_hex_information)
        dictionary_of_board_geometry: Dict[str, List[Dict[str, str | float] | Dict[str, float]]] = {
            "hexes": list_of_dictionaries_of_hex_information,
            "vertices": list_of_dictionaries_of_information_re_unique_vertices,
            "edges": list_of_dictionaries_of_edge_coordinates
        }
        return dictionary_of_board_geometry


def main():
    '''
    Generate a dictionary of board geometry and write the dictionary to a JSON file.
    '''
    generator = BoardGeometryGenerator(
        board_layout = DEFAULT_BOARD_LAYOUT,
        board_width = WIDTH_OF_BOARD_IN_VMIN,
        hex_width = WIDTH_OF_HEX,
        hex_height = HEIGHT_OF_HEX,
        margin_of_error = MARGIN_OF_ERROR
    )
    board = Board()
    dictionary_of_board_geometry: Dict[str, List[Dict[str, str | float] | Dict[str, float]]] = generator.generate_board_geometry(board)
    number_of_hexes: int = len(dictionary_of_board_geometry["hexes"])
    number_of_vertices: int = len(dictionary_of_board_geometry["vertices"])
    number_of_edges: int = len(dictionary_of_board_geometry["edges"])
    logger.info(f"Geometry generated with {number_of_hexes} hexes, {number_of_vertices} vertices, {number_of_edges} edges")

    with open(settings.board_geometry_path, "w") as f:
        json.dump(dictionary_of_board_geometry, f, indent = 4)
    logger.info(f"A dictionary of board geometry was written to {settings.board_geometry_path}.")


if __name__ == "__main__":
    main()