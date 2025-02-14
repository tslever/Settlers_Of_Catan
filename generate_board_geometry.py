#!/usr/bin/env python3
import os
from back_end.utilities.board import HEIGHT_OF_HEX
from back_end.utilities.board import MARGIN_OF_ERROR
from back_end.utilities.board import WIDTH_OF_BOARD_IN_VMIN
from back_end.utilities.board import WIDTH_OF_HEX
from back_end.utilities.board import Board
import json
import logging


ARRAY_OF_HEX_IDS_REPRESENTING_BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]
BOARD = Board()
DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW = HEIGHT_OF_HEX / 4
DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW = HEIGHT_OF_HEX - DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW
NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST = len(ARRAY_OF_HEX_IDS_REPRESENTING_BOARD) - 1
HEIGHT_OF_BOARD = HEIGHT_OF_HEX + DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW * NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST
VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES = (100 - HEIGHT_OF_BOARD) / 2


logging.basicConfig(level = logging.INFO)
logger = logging.getLogger(__name__)


def get_list_of_tuples_of_hex_information():
    '''
    Provide a list of tuples of hex information.

    Returns:
        list_of_tuples_of_hex_information: list[tuple] --
            list of tuples of hex information including hex ID,
            horizontal position of left edge of hex, and vertical position of top point of hex
    '''
    list_of_tuples_of_hex_information = []
    for row_index, row in enumerate(ARRAY_OF_HEX_IDS_REPRESENTING_BOARD):
        vertical_position = VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES + row_index * DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW
        number_of_hexes = len(row)
        horizontal_position_of_first_hex = (WIDTH_OF_BOARD_IN_VMIN - number_of_hexes * WIDTH_OF_HEX) / 2
        for index_of_hex, id_of_hex in enumerate(row):
            horizontal_position = horizontal_position_of_first_hex + index_of_hex * WIDTH_OF_HEX
            tuple_of_hex_information = (id_of_hex, horizontal_position, vertical_position)
            list_of_tuples_of_hex_information.append(tuple_of_hex_information)
    return list_of_tuples_of_hex_information


def edge_already_exists(tuple_of_edge_coordinates, list_of_tuples_of_edge_coordinates, margin_of_error):
    '''
    Determine whether the given tuple of edge coordinates
    is already present in a given list of tuples of edges coordinates.

    Arguments:
        edge: tuple -- a tuple of edge coordinates
        list_of_tuples_of_edge_coordinates: list[tuple] -- a list of tuples of edge coordinates
        margin of error: float -- tolerance for equality of two coordinates

        Returns:
            bool: True if the given tuple of edge coordinates already exists in the given list; False otherwise
    '''
    (x11, y11, x12, y12) = tuple_of_edge_coordinates
    for x21, y21, x22, y22 in list_of_tuples_of_edge_coordinates:
        if (
            (abs(x11 - x21) < margin_of_error and abs(y11 - y21) < margin_of_error and
             abs(x12 - x22) < margin_of_error and abs(y12 - y22) < margin_of_error) or
            (abs(x11 - x22) < margin_of_error and abs(y11 - y22) < margin_of_error and
             abs(x12 - x21) < margin_of_error and abs(y12 - y21) < margin_of_error)
        ):
            return True
    return False


def get_all_edges(list_of_dictionaries_of_hex_information: list[dict]):
    '''
    Provide a list of tuples of edge coordinates for all unique edges
    for the hexes represented by the given list of dictionaries of hex information.

    Arguments:
        list_of_dictionaries_of_hex_information: list[dict] --
            list of dictionaries of hex information. Each dictionary includes keys "id", "x", and "y".
    
    Returns:
        list_of_tuples_of_edge_coordinates: list[tuple[float, float]] --
            a list of tuples of edge coordinates. Each tuple is of the form (x1, y1, x2, y2).
    '''
    list_of_tuples_of_edge_coordinates = []
    for dictionary_of_hex_information in list_of_dictionaries_of_hex_information:
        list_of_pairs_of_coordinates_of_vertices_of_hex = BOARD.get_hex_vertices(dictionary_of_hex_information)
        number_of_vertices_of_hex = len(list_of_pairs_of_coordinates_of_vertices_of_hex)
        for i in range(number_of_vertices_of_hex):
            v1 = list_of_pairs_of_coordinates_of_vertices_of_hex[i]
            v2 = list_of_pairs_of_coordinates_of_vertices_of_hex[(i + 1) % number_of_vertices_of_hex]
            x1 = v1[0]
            y1 = v1[1]
            x2 = v2[0]
            y2 = v2[1]
            tuple_of_edge_coordinates = (x1, y1, x2, y2)
            if not edge_already_exists(tuple_of_edge_coordinates, list_of_tuples_of_edge_coordinates, MARGIN_OF_ERROR):
                list_of_tuples_of_edge_coordinates.append(tuple_of_edge_coordinates)
    return list_of_tuples_of_edge_coordinates


def vertex_already_exists(tuple_of_vertex_coordinates, list_of_tuples_of_vertex_coordinates, margin_of_error):
    '''
    Check whether a tuple of vertex coordinates is already in a list of tuples of vertex coordinates.

    Arguments:
        tuple_of_vertex_coordinates: tuple[float, float] -- a tuple of vertex coordinates of the form (x1, y1)
        list_of_tuples_of_vertex_coordinates: list[tuple[float, float]] -- a list of tuples of vertex coordinates
        margin_of_error: float -- tolerance for equality of two coordinates

    Returns:
        bool: True if the given tuple of vertex coordinates exists in the list; False otherwise
    '''
    (x1, y1) = tuple_of_vertex_coordinates
    for x2, y2 in list_of_tuples_of_vertex_coordinates:
        if abs(x1 - x2) < margin_of_error and abs(y1 - y2) < margin_of_error:
            return True
    return False


def get_unique_vertices(list_of_dictionaries_of_hex_information: list[dict]):
    '''
    Provide a list of tuples of coordinates of unique vertices
    for the hexes represented by the given list of dictionaries of hex information.

    Arguments:
        list_of_dictionaries_of_hex_information: list[dict[str, str | float]] -- list of dictionaries of hex information
    
    Returns:
        list_of_tuples_of_coordinates_of_unique_vertices: list[tuple[float, float]] --
            list of tuples of coordinates of unique vertices. Each tuple is of the form (x, y).
    '''
    list_of_tuples_of_coordinates_of_unique_vertices = []
    for dictionary_of_hex_information in list_of_dictionaries_of_hex_information:
        for tuple_of_coordinates_of_vertex in BOARD.get_hex_vertices(dictionary_of_hex_information):
            if not vertex_already_exists(
                tuple_of_coordinates_of_vertex,
                list_of_tuples_of_coordinates_of_unique_vertices,
                MARGIN_OF_ERROR
            ):
                list_of_tuples_of_coordinates_of_unique_vertices.append(tuple_of_coordinates_of_vertex)
    return list_of_tuples_of_coordinates_of_unique_vertices


def get_vertices_with_labels(list_of_dictionaries_of_hex_information: list[dict[str, str | float]]):
    '''
    Provide a list of dictionaries of information re unique vertices.

    Arguments:
        list_of_dictionaries_of_hex_information: list[dict[str, str | float]] -- list of dictionaries of hex information
    
    Returns:
        list_of_dictionaries_of_information_re_unique_vertices: list[dict[str, str | float]] --
            list where each element is a dictionary with keys "label", "x", and "y".
    '''
    list_of_tuples_of_coordinates_of_unique_vertices = get_unique_vertices(list_of_dictionaries_of_hex_information)
    list_of_dictionaries_of_information_re_unique_vertices = []
    for index_of_tuple_of_coordinates_of_unique_vertices, (x, y) in enumerate(list_of_tuples_of_coordinates_of_unique_vertices):
        label_of_vertex = f"V{index_of_tuple_of_coordinates_of_unique_vertices+1:02d}" # pad to 2 digits
        dictionary_of_vertex_information = {
            "label": label_of_vertex,
            "x": x,
            "y": y
        }
        list_of_dictionaries_of_information_re_unique_vertices.append(dictionary_of_vertex_information)
    return list_of_dictionaries_of_information_re_unique_vertices


def generate_board_geometry():
    '''
    Generate the full board geometry, including hex information, vertex information, and edge coordinates.

    Returns:
        dictionary_of_board_geometry: dict[str, list[dict[str, str | float] | dict[str, float]]] --
            dictionary of board geometry with keys "hexes", "vertices", and "edges"
    '''
    list_of_tuples_of_hex_information = get_list_of_tuples_of_hex_information()
    list_of_dictionaries_of_hex_information = [{"id": hex_id, "x": x, "y": y} for hex_id, x, y in list_of_tuples_of_hex_information]
    list_of_dictionaries_of_information_re_unique_vertices = get_vertices_with_labels(list_of_dictionaries_of_hex_information)
    list_of_tuples_of_edge_coordinates = get_all_edges(list_of_dictionaries_of_hex_information)
    list_of_dictionaries_of_edge_coordinates = [{"x1": x1, "y1": y1, "x2": x2, "y2": y2} for x1, y1, x2, y2 in list_of_tuples_of_edge_coordinates]
    dictionary_of_board_geometry = {
        "hexes": list_of_dictionaries_of_hex_information,
        "vertices": list_of_dictionaries_of_information_re_unique_vertices,
        "edges": list_of_dictionaries_of_edge_coordinates
    }
    return dictionary_of_board_geometry


def main():
    '''
    Generate a dictionary of board geometry and write the dictionary to a JSON file.
    '''
    geometry = generate_board_geometry()
    number_of_hexes = len(geometry["hexes"])
    number_of_vertices = len(geometry["vertices"])
    number_of_edges = len(geometry["edges"])
    logger.info(f"Geometry generated with {number_of_hexes} hexes, {number_of_vertices} vertices, {number_of_edges} edges")
    path_to_directory = os.path.dirname(os.path.abspath(__file__))
    path_to_json_file = os.path.join(path_to_directory, "board_geometry.json")
    with open(path_to_json_file, "w") as f:
        json.dump(geometry, f, indent = 4)
    logger.info(f"A dictionary of board geometry was written to {path_to_json_file}.")


if __name__ == "__main__":
    main()