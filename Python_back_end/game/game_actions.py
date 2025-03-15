from Python_back_end.game.board import Board
from Python_back_end.db.database import City
from Python_back_end.game.phase import Phase
from Python_back_end.game.board import RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX
from Python_back_end.db.database import Road
from Python_back_end.db.database import Settlement
from Python_back_end.game.board import TOKEN_DOT_MAPPING
from Python_back_end.game.board import TOKEN_MAPPING
from Python_back_end.game.board import WIDTH_OF_HEX
from flask import abort
import logging
import math
from Python_back_end.ai.neural_network import neural_network
from Python_back_end.ai.strategy import predict_best_settlement
from Python_back_end.ai.strategy import predict_best_city
from Python_back_end.ai.strategy import predict_best_road
from Python_back_end.settings import settings


THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT = RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX * WIDTH_OF_HEX + settings.margin_of_error
logger = logging.getLogger(__name__)
board = Board()


def get_list_of_labels_of_occupied_vertices(session) -> list[str]:
    settlements = session.query(Settlement).all()
    cities = session.query(City).all()
    set_of_labels_of_vertices_with_settlements = {settlement.vertex for settlement in settlements}
    set_of_labels_of_vertices_with_cities = {city.vertex for city in cities}
    set_of_labels_of_occupied_vertices = set_of_labels_of_vertices_with_settlements.union(set_of_labels_of_vertices_with_cities)
    list_of_labels_of_occupied_vertices = list(set_of_labels_of_occupied_vertices)
    return list_of_labels_of_occupied_vertices


def compute_strengths(session):
    vertex_coord = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    strengths = {1: 0, 2: 0, 3: 0}
    settlements = session.query(Settlement).all()
    cities = session.query(City).all()
    all_buildings = settlements + cities
    for building in all_buildings:
        player = building.player
        vertex_label = building.vertex
        if vertex_label not in vertex_coord:
            continue
        building_coord = vertex_coord[vertex_label]
        for hex in board.hexes:
            for hv in board.get_hex_vertices(hex):
                if abs(hv[0] - building_coord[0]) < settings.margin_of_error and abs(hv[1] - building_coord[1]) < settings.margin_of_error:
                    hex_id = hex["id"]
                    number_of_token = TOKEN_MAPPING.get(hex_id)
                    if number_of_token is not None:
                        number_of_pips_corresponding_to_number_of_token = TOKEN_DOT_MAPPING.get(number_of_token, 0)
                        strengths[player] += number_of_pips_corresponding_to_number_of_token
                    break
    return strengths


def create_settlement(session, current_player, phase: Phase):
    list_of_labels_of_occupied_vertices = get_list_of_labels_of_occupied_vertices(session)
    list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
    if not list_of_labels_of_available_vertices:
        logger.exception(f"No vertices are available for settlement placement by Player {current_player} during phase {phase.value}.")
        return None, None, None, f"No vertices are available for settlement placement by Player {current_player} during phase {phase.value}."

    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    game_state = {
        
    }
    chosen_vertex = predict_best_settlement(game_state, list_of_labels_of_available_vertices, vertex_coords, neural_network)
    if not chosen_vertex:
        logger.error("AI failed to choose a vertex for settlement placement.")
        return None, None, None, "AI failed to choose a vertex for settlement placement."
    settlement = Settlement(player = current_player, vertex = chosen_vertex)
    session.add(settlement)
    session.commit()
    settlement_id = settlement.id
    next_phase = Phase.TO_PLACE_FIRST_ROAD if phase == Phase.TO_PLACE_FIRST_SETTLEMENT else None
    return chosen_vertex, settlement_id, next_phase, None


def create_city(session, current_player, phase: Phase):
    list_of_labels_of_occupied_vertices = get_list_of_labels_of_occupied_vertices(session)
    list_of_labels_of_available_vertices = board.get_available_building_moves(list_of_labels_of_occupied_vertices)
    if not list_of_labels_of_available_vertices:
        logger.exception(f"No vertices are available for city placement by Player {current_player} during phase {phase.value}.")
        return None, None, None, f"No vertices are available for city placement by Player {current_player} during phase {phase.value}."

    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    game_state = {
        
    }
    chosen_vertex = predict_best_city(game_state, list_of_labels_of_available_vertices, vertex_coords, neural_network)
    if not chosen_vertex:
        logger.exception("AI failed to choose a vertex for city placement.")
        return None, None, None, "AI failed to choose a vertex for city placement."
    city = City(player = current_player, vertex = chosen_vertex)
    session.add(city)
    session.commit()
    city_id = city.id
    next_phase = Phase.TO_PLACE_SECOND_ROAD if phase == Phase.TO_PLACE_FIRST_CITY else None
    return chosen_vertex, city_id, next_phase, None


def create_road(session, current_player, phase: Phase, last_settlement_or_city):
    if (phase == Phase.TO_PLACE_FIRST_ROAD) and (not last_settlement_or_city):
        logger.error(f"No settlement recorded for road placement (current_player = {current_player}, phase = {phase.value})")
        return None, None, None, None, "No settlement recorded for road placement."
    if (phase == Phase.TO_PLACE_SECOND_ROAD) and (not last_settlement_or_city):
        logger.error(f"No city recorded for road placement (current_player = {current_player}, phase = {phase.value})")
        return None, None, None, None, "No settlement recorded for city placement."
    
    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    if last_settlement_or_city not in vertex_coords:
        logger.error(f"Invalid last settlement or city vertex: {last_settlement_or_city}")
        return None, None, None, None, "Invalid last settlement or city vertex."
    settlement_or_city_coord = vertex_coords[last_settlement_or_city]
    adjacent_edges = []
    for edge in board.edges:
        x1 = edge["x1"]
        y1 = edge["y1"]
        x2 = edge["x2"]
        y2 = edge["y2"]
        if (
            (math.isclose(x1, settlement_or_city_coord[0], abs_tol = settings.margin_of_error) and math.isclose(y1, settlement_or_city_coord[1], abs_tol = settings.margin_of_error)) or
            (math.isclose(x2, settlement_or_city_coord[0], abs_tol = settings.margin_of_error) and math.isclose(y2, settlement_or_city_coord[1], abs_tol = settings.margin_of_error))
        ):
            adjacent_edges.append(edge)
    if not adjacent_edges:
        logger.error(f"No adjacent edges found for settlement or city at {settlement_or_city_coord}")
        return None, None, None, None, "No adjacent edges found for settlement or city."

    roads = session.query(Road).all()
    used_edges = {road.edge for road in roads}
    available_edges = []
    for edge in adjacent_edges:
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        edge_key = board.get_edge_key(v1, v2)
        if edge_key not in used_edges:
            available_edges.append((edge, edge_key))
    if not available_edges:
        logger.error(f"No available roads adjacent to settlement or city {last_settlement_or_city}.")
        return None, None, None, None, "No available roads adjacent to settlement or city."
    
    '''
    TODO: Consider whether or not more items of the game state should be used.
    '''
    game_state = {
        "last_building": last_settlement_or_city,
        "settlements": [s.vertex for s in session.query(Settlement).all()],
        "cities": [c.vertex for c in session.query(City).all()],
        "roads": [r.edge for r in session.query(Road).all()],
        "players": [1, 2, 3]
    }
    best_edge, best_edge_key = predict_best_road(game_state, available_edges, vertex_coords, neural_network)
    if best_edge is None:
        logger.error("AI failed to choose an edge for road placement.")
        return None, None, None, None, "No valid edge found for road placement."

    road = Road(player = current_player, edge = best_edge_key)
    session.add(road)
    session.commit()
    road_id = road.id

    if phase == Phase.TO_PLACE_FIRST_ROAD:
        next_phase = Phase.TO_PLACE_FIRST_SETTLEMENT
        if current_player == 1:
            next_player = 2
        elif current_player == 2:
            next_player = 3
        elif current_player == 3:
            next_player = 3
            next_phase = Phase.TO_PLACE_FIRST_CITY
        else:
            logger.error(f"Current player does not exist: {current_player}")
            abort(500, description = "Current player does not exist.")
    elif phase == Phase.TO_PLACE_SECOND_ROAD:
        next_phase = Phase.TO_PLACE_FIRST_CITY
        if current_player == 3:
            next_player = 2
        elif current_player == 2:
            next_player = 1
        elif current_player == 1:
            next_player = 1
            next_phase = Phase.TURN
        else:
            logger.error(f"Current player does not exist: {current_player}")
            abort(500, description = "Current player does not exist.")
    else:
        logger.error(f"Invalid phase for road placement: {phase.value}")
        return None, None, None, None, "Invalid phase for road placement."
    return best_edge_key, road_id, next_phase, next_player, None