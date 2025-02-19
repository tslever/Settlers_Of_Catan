from flask import Blueprint
from ..utilities.board import Board
from ..utilities.board import MARGIN_OF_ERROR
from ..utilities.phase import Phase
from ..utilities.board import RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX
from ..db.database import City
from ..db.database import Road
from ..db.database import Settlement
from ..db.database import State
from ..utilities.board import TOKEN_DOT_MAPPING
from ..utilities.board import TOKEN_MAPPING
from ..utilities.board import WIDTH_OF_HEX
from flask import abort
from ..db.database import get_db_session
import json
from flask import jsonify
import logging
import math
from ..ai.strategy import predict_best_city
from ..ai.strategy import predict_best_settlement
from ..ai.strategy import predict_best_road


ID_OF_STATE = 1
THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT = RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX * WIDTH_OF_HEX + MARGIN_OF_ERROR


blueprint_for_route_next = Blueprint("next", __name__)
board = Board()
logger = logging.getLogger(__name__)


def create_settlement(session, current_player, phase: Phase):
    settlements = session.query(Settlement).all()
    cities = session.query(City).all()
    used_vertices = {settlement.vertex for settlement in settlements}.union({city.vertex for city in cities})
    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    existing_coords = [vertex_coords[label] for label in used_vertices if label in vertex_coords]
    available = []
    for v in board.vertices:
        label = v["label"]
        x1 = v["x"]
        y1 = v["y"]
        if label in used_vertices:
            continue
        too_close = any(
            math.sqrt((x1 - x2)**2 + (y1 - y2)**2) < THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT
            for x2, y2 in existing_coords
        )
        if not too_close:
            available.append(label)
    if not available:
        logger.error(f"No vertices available for settlement placement (current_player={current_player}, phase={phase.value}).")
        return None, None, None, "No vertices are available."
    
    '''
    TODO: Include details such as
    the names of all players,
    the positions of each player's settlements,
    the positions of each player's roads,
    and other details.
    '''
    game_state = {
        
    }
    chosen_vertex = predict_best_settlement(game_state, available, vertex_coords)
    if not chosen_vertex:
        logger.error("AI failed to choose a vertex for settlement placement.")
        return None, None, None, "AI decision error."
    settlement = Settlement(player = current_player, vertex = chosen_vertex)
    session.add(settlement)
    session.commit()
    settlement_id = settlement.id
    if phase == Phase.TO_PLACE_FIRST_SETTLEMENT:
        next_phase = Phase.TO_PLACE_FIRST_ROAD
    else:
        logger.error(f"Invalid phase for settlement placement: {phase.value}")
        return None, None, None, "Invalid phase for settlement placement."
    return chosen_vertex, settlement_id, next_phase, None


def create_city(session, current_player, phase: Phase):
    settlements = session.query(Settlement).all()
    cities = session.query(City).all()
    used_vertices = {s.vertex for s in settlements}.union({city.vertex for city in cities})
    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    existing_coords = [vertex_coords[label] for label in used_vertices if label in vertex_coords]
    available = []
    for v in board.vertices:
        label = v["label"]
        x1 = v["x"]
        y1 = v["y"]
        if label in used_vertices:
            continue
        too_close = any(
            math.sqrt((x1 - x2)**2 + (y1 - y2)**2) < THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT
            for x2, y2 in existing_coords
        )
        if not too_close:
            available.append(label)
    if not available:
        logger.error(f"No vertices available for city placement (current_player={current_player}, phase={phase.value}).")
        return None, None, None, "No vertices are available."
    
    '''
    TODO: Include details such as
    the names of all players,
    the positions of each player's settlements,
    the positions of each player's roads,
    and other details.
    '''
    game_state = {
        
    }
    chosen_vertex = predict_best_city(game_state, available, vertex_coords)
    if not chosen_vertex:
        logger.error("AI failed to choose a vertex for city placement.")
        return None, None, None, "AI decision error."
    city = City(player = current_player, vertex = chosen_vertex)
    session.add(city)
    session.commit()
    city_id = city.id
    if phase == Phase.TO_PLACE_FIRST_CITY:
        next_phase = Phase.TO_PLACE_SECOND_ROAD
    else:
        logger.error(f"Invalid phase for city placement: {phase.value}")
        return None, None, None, "Invalid phase for city placement."
    return chosen_vertex, city_id, next_phase, None
    

def create_road(session, current_player, phase: Phase, last_settlement_or_city):
    if (Phase == Phase.TO_PLACE_FIRST_ROAD) and (not last_settlement_or_city):
        logger.error(f"No settlement recorded for road placement (current_player={current_player}, phase={phase.value})")
        return None, None, None, None, "No settlement recorded for road placement."
    if (Phase == Phase.TO_PLACE_SECOND_ROAD) and (not last_settlement_or_city):
        logger.error(f"No city recorded for road placement (current_player={current_player}, phase={phase.value})")
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
            (math.isclose(x1, settlement_or_city_coord[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(y1, settlement_or_city_coord[1], abs_tol = MARGIN_OF_ERROR)) or
            (math.isclose(x2, settlement_or_city_coord[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(y2, settlement_or_city_coord[1], abs_tol = MARGIN_OF_ERROR))
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
    TODO: Include details such as
    the names of all players,
    the positions of each player's settlements,
    the positions of each player's roads,
    and other details.
    '''
    game_state = {
        "last_settlement": last_settlement_or_city
    }
    best_edge, best_edge_key = predict_best_road(game_state, available_edges, vertex_coords)
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


def compute_strengths(session):
    vertex_coord = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    strengths = {1: 0, 2: 0, 3: 0}
    settlements = session.query(Settlement).all()
    for settlement in settlements:
        player = settlement.player
        vertex_label = settlement.vertex
        if vertex_label not in vertex_coord:
            continue
        settlement_coord = vertex_coord[vertex_label]
        for hex in board.hexes:
            for hv in board.get_hex_vertices(hex):
                if abs(hv[0] - settlement_coord[0]) < MARGIN_OF_ERROR and abs(hv[1] - settlement_coord[1]) < MARGIN_OF_ERROR:
                    hex_id = hex["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        dot_count = TOKEN_DOT_MAPPING.get(token, 0)
                        strengths[player] += dot_count
                    break
    return strengths


@blueprint_for_route_next.route("/next", methods = ["POST"])
def next_move():
    with get_db_session() as session:
        state = session.query(State).filter_by(id = ID_OF_STATE).first()
        if state is None:
            state = State(
                id = ID_OF_STATE,
                current_player = 1,
                phase = Phase.TO_PLACE_FIRST_SETTLEMENT.value,
                last_settlement = None,
                last_city = None
            )
            session.add(state)
            session.commit()
            logger.info(f"Initialized state: player = 1, phase = {Phase.TO_PLACE_FIRST_SETTLEMENT.value}")
        current_player = state.current_player
        try:
            phase = Phase(state.phase)
        except ValueError:
            logger.error(f"Invalid phase in state: {state.phase}")
            abort(500, description = "Invalid phase in state.")
        last_settlement = state.last_settlement
        last_city = state.last_city
        logger.info(f"Current state: player = {current_player}, phase = {phase.value}, last_settlement = {last_settlement}, last_city = {last_city}")

        if phase is Phase.TO_PLACE_FIRST_SETTLEMENT:
            chosen_vertex, settlement_id, next_phase, error = create_settlement(session, current_player, phase)
            if error:
                logger.error(f"Settlement creation error: {error}")
                abort(400, description = error)
            state.phase = next_phase.value
            state.last_settlement = chosen_vertex
            session.commit()
            logger.info(f"Settlement created for Player {current_player} at vertex {chosen_vertex} (settlement_id = {settlement_id}). State updated to phase = {next_phase.value}.")
            return jsonify(
                {
                    "message": f"Settlement created for Player {current_player}",
                    "moveType": "settlement",
                    "settlement": {
                        "id": settlement_id,
                        "player": current_player,
                        "vertex": chosen_vertex
                    }
                }
            )
        if phase is Phase.TO_PLACE_FIRST_CITY:
            chosen_vertex, city_id, next_phase, error = create_city(session, current_player, phase)
            if error:
                logger.error(f"City creation error: {error}")
                abort(400, description = error)
            state.phase = next_phase.value
            state.last_city = chosen_vertex
            session.commit()
            logger.info(f"City created for Player {current_player} at vertex {chosen_vertex} (city_id = {city_id}). State updated to phase = {next_phase.value}.")
            return jsonify(
                {
                    "message": f"City created for Player {current_player}",
                    "moveType": "city",
                    "city": {
                        "id": city_id,
                        "player": current_player,
                        "vertex": chosen_vertex
                    }
                }
            )
        elif phase in (Phase.TO_PLACE_FIRST_ROAD, Phase.TO_PLACE_SECOND_ROAD):
            if phase == Phase.TO_PLACE_FIRST_ROAD:
                last_vertex = state.last_settlement
            else:
                last_vertex = state.last_city
            chosen_edge_key, road_id, next_phase, next_player, error = create_road(session, current_player, phase, last_vertex)
            if error:
                logger.error(f"Road creation error: {error}")
                abort(400, description = error)
            state.current_player = next_player
            state.phase = next_phase.value
            if phase == Phase.TO_PLACE_FIRST_ROAD:
                state.last_settlement = None
            else:
                state.last_city == None
            session.commit()
            logger.info(f"Road created for Player {current_player} on edge {chosen_edge_key} (road_id={road_id}). State updated to player={next_player}, phase={next_phase.value}.")
            response = {
                "message": f"Road created for Player {current_player}",
                "moveType": "road",
                "road": {
                    "id": road_id,
                    "player": current_player,
                    "edge": chosen_edge_key
                }
            }
            if (next_phase == Phase.TURN):
                strengths = compute_strengths(session)
                response["strengths"] = strengths
                response["message"] += "\nGame setup complete. Strengths computed:\n" + json.dumps(strengths, indent = 4, sort_keys = True)
            return jsonify(response)
        else:
            logger.error(f"Invalid phase encountered: {phase.value}")
            abort(400, "Invalid phase in state.")