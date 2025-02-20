from back_end.board import Board
from back_end.db.database import City
from back_end.db.database import Settlement
from back_end.board import TOKEN_DOT_MAPPING
from back_end.board import TOKEN_MAPPING
import math
from back_end.settings import settings


def calculate_euclidean_distance_between_points_defined_by_coordinates(x1: float, y1: float, x2: float, y2: float) -> float:
    return math.sqrt((x1 - x2)**2 + (y1 - y2)**2)


def get_list_of_labels_of_occupied_vertices(session, board: Board) -> list[str]:
    settlements = session.query(Settlement).all()
    cities = session.query(City).all()
    set_of_labels_of_vertices_with_settlements = {settlement.vertex for settlement in settlements}
    set_of_labels_of_vertices_with_cities = {city.vertex for city in cities}
    set_of_labels_of_occupied_vertices = set_of_labels_of_vertices_with_settlements.union(set_of_labels_of_vertices_with_cities)
    list_of_labels_of_occupied_vertices = list(set_of_labels_of_occupied_vertices)
    return list_of_labels_of_occupied_vertices


def compute_vertex_feature_vector(board: Board, label_of_vertex: str) -> list[float] | None:
    '''
    Return a feature vector with 5 elements for the vertex with the given label.
    First, attempt to use the board's precomputed features.
    If the boards precomputed features are missing,
    compute the feature vector using information regarding hexes adjacent to the vertex with the given label.
    '''
    feature_vector = board.get_vertex_features(label_of_vertex)
    if feature_vector is None:
        dictionary_of_vertex_information = board.get_vertex_by_label(label_of_vertex)
        if dictionary_of_vertex_information is None:
            return None
        x1 = dictionary_of_vertex_information["x"]
        y1 = dictionary_of_vertex_information["y"]
        total_number_of_pips = 0
        number_of_adjacent_hexes = 0
        for dictionary_of_hex_information in board.hexes:
            for x2, y2 in board.get_hex_vertices(dictionary_of_hex_information):
                if math.isclose(x2, x1, abs_tol = settings.margin_of_error) and math.isclose(y2, y1, abs_tol = settings.margin_of_error):
                    number_on_token = TOKEN_MAPPING.get(dictionary_of_hex_information["id"])
                    if number_on_token is not None:
                        number_of_pips_corresponding_to_number_on_token = TOKEN_DOT_MAPPING.get(number_on_token, 0)
                        total_number_of_pips += number_of_pips_corresponding_to_number_on_token
                        number_of_adjacent_hexes += 1
                    break
        normalized_total_number_of_pips = total_number_of_pips / (number_of_adjacent_hexes * 5) if number_of_adjacent_hexes > 0 else 0.0
        normalized_horizontal_position_of_vertex = x1 / settings.width_of_board_in_vmin
        normalized_vertical_position_of_vertex = y1 / 100.0
        normalized_number_of_adjacent_hexes = number_of_adjacent_hexes / 3.0
        feature_vector = [
            normalized_total_number_of_pips,
            normalized_horizontal_position_of_vertex,
            normalized_vertical_position_of_vertex,
            normalized_number_of_adjacent_hexes,
            1.0
        ]
    return feature_vector