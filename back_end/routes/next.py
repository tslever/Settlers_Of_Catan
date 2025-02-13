from flask import Blueprint
from ..utilities.board import Board
from ..utilities.board import MARGIN_OF_ERROR
from ..utilities.phase import Phase
from ..utilities.board import RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX
from ..utilities.board import TOKEN_DOT_MAPPING
from ..utilities.board import TOKEN_MAPPING
from ..utilities.board import WIDTH_OF_HEX
from flask import abort
from ..db.database import get_db_connection
import json
from flask import jsonify
import logging
import math
from ..ai.strategy import predict_best_settlement
from ..ai.strategy import predict_best_road


ID_OF_STATE = 1
THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT = RATIO_OF_LENGTH_OF_SIDE_OF_HEX_AND_WIDTH_OF_HEX * WIDTH_OF_HEX + MARGIN_OF_ERROR


blueprint_for_route_next = Blueprint("next", __name__)
board = Board()
logger = logging.getLogger(__name__)


def create_settlement(cursor, current_player, phase: Phase):
    cursor.execute("SELECT vertex FROM settlements")
    used_vertices = {r["vertex"] for r in cursor.fetchall()}
    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    existing_coords = [vertex_coords[label] for label in used_vertices if label in vertex_coords]
    available = []
    for v in board.vertices:
        label = v["label"]
        x = v["x"]
        y = v["y"]
        if label in used_vertices:
            continue
        too_close = False
        for ex in existing_coords:
            if math.sqrt((x - ex[0])**2 + (y - ex[1])**2) < THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT:
                too_close = True
                break
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



    cursor.execute(
        "INSERT INTO settlements (player, vertex) VALUES (?, ?)",
        (current_player, chosen_vertex)
    )
    settlement_id = cursor.lastrowid
    next_phase = ""
    if phase == Phase.TO_PLACE_FIRST_SETTLEMENT:
        next_phase = Phase.TO_PLACE_FIRST_ROAD
    elif phase == Phase.TO_PLACE_SECOND_SETTLEMENT:
        next_phase = Phase.TO_PLACE_SECOND_ROAD
    else:
        logger.error(f"Invalid phase for settlement placement: {phase.value}")
        return None, None, None, "Invalid phase for settlement placement."
    return chosen_vertex, settlement_id, next_phase, None
    

def create_road(cursor, current_player, phase: Phase, last_settlement):
    if not last_settlement:
        logger.error(f"No settlement recorded for road placement (current_player={current_player}, phase={phase.value})")
        return None, None, None, None, "No settlement recorded for road placement."
    vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    if last_settlement not in vertex_coords:
        logger.error(f"Invalid last settlement vertex: {last_settlement}")
        return None, None, None, None, "Invalid last settlement vertex."
    settlement_coord = vertex_coords[last_settlement]
    adjacent_edges = []
    for edge in board.edges:
        x1 = edge["x1"]
        y1 = edge["y1"]
        x2 = edge["x2"]
        y2 = edge["y2"]
        if (
            (math.isclose(x1, settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(y1, settlement_coord[1], abs_tol = MARGIN_OF_ERROR)) or
            (math.isclose(x2, settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(y2, settlement_coord[1], abs_tol = MARGIN_OF_ERROR))
        ):
            adjacent_edges.append(edge)
    if not adjacent_edges:
        logger.error(f"No adjacent edges found for settlement at {settlement_coord}")
        return None, None, None, None, "No adjacent edges found for settlement."
    cursor.execute("SELECT edge FROM roads")
    used_edges = {row["edge"] for row in cursor.fetchall()}
    available_edges = []
    for edge in adjacent_edges:
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        edge_key = board.get_edge_key(v1, v2)
        if edge_key not in used_edges:
            available_edges.append((edge, edge_key))
    if not available_edges:
        logger.error(f"No available roads adjacent to settlement {last_settlement}.")
        return None, None, None, None, "No available roads adjacent to settlement."
    


    '''
    TODO: Include details such as
    the names of all players,
    the positions of each player's settlements,
    the positions of each player's roads,
    and other details.
    '''
    game_state = {
        "last_settlement": last_settlement
    }
    best_edge, best_edge_key = predict_best_road(game_state, available_edges, vertex_coords)
    if best_edge is None:
        logger.error("AI failed to choose an edge for road placement.")
        return None, None, None, None, "No valid edge found for road placement."



    cursor.execute("INSERT INTO roads (player, edge) VALUES (?, ?)", (current_player, best_edge_key))
    road_id = cursor.lastrowid
    next_player = 0
    if phase == Phase.TO_PLACE_FIRST_ROAD:
        next_phase = Phase.TO_PLACE_FIRST_SETTLEMENT
        if current_player == 1:
            next_player = 2
        elif current_player == 2:
            next_player = 3
        elif current_player == 3:
            next_player = 3
            next_phase = Phase.TO_PLACE_SECOND_SETTLEMENT
        else:
            logger.error(f"Current player does not exist: {current_player}")
            abort(500, description = "Current player does not exist.")
    elif phase == Phase.TO_PLACE_SECOND_ROAD:
        next_phase = Phase.TO_PLACE_SECOND_SETTLEMENT
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


def compute_strengths():
    vertex_coord = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
    strengths = {1: 0, 2: 0, 3: 0}
    with get_db_connection() as connection:
        cursor = connection.execute("SELECT player, vertex FROM settlements")
        settlements = cursor.fetchall()
    for settlement in settlements:
        player = settlement["player"]
        vertex_label = settlement["vertex"]
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
    with get_db_connection() as connection:
        cursor = connection.cursor()
        cursor.execute("SELECT current_player, phase, last_settlement FROM state WHERE id = ?", (ID_OF_STATE,))
        row = cursor.fetchone()

        if row is None:
            current_player = 1
            phase = Phase.TO_PLACE_FIRST_SETTLEMENT
            last_settlement = None
            cursor.execute(
                "INSERT INTO state (id, current_player, phase, last_settlement) VALUES (?, 1, ?, NULL)",
                (ID_OF_STATE, Phase.TO_PLACE_FIRST_SETTLEMENT)
            )
            logger.info(f"Initialized state: player=1, phase={Phase.TO_PLACE_FIRST_SETTLEMENT.value}")
        else:
            current_player = row["current_player"]
            try:
                phase = Phase(row["phase"])
            except ValueError:
                logger.error(f"Invalid phase in state: {row["phase"]}")
                abort(500, description = "Invalid phase in state.")
            last_settlement = row["last_settlement"]
            logger.info(f"Current state: player={current_player}, phase={phase.value}, last_settlement={last_settlement}")
        if phase in (Phase.TO_PLACE_FIRST_SETTLEMENT, Phase.TO_PLACE_SECOND_SETTLEMENT):
            chosen_vertex, settlement_id, next_phase, error = create_settlement(cursor, current_player, phase)
            if error:
                logger.error(f"Settlement creation error: {error}")
                abort(400, description = error)
            cursor.execute(
                "UPDATE state SET phase = ?, last_settlement = ? WHERE id = ?",
                (next_phase.value, chosen_vertex, ID_OF_STATE)
            )
            logger.info(f"Settlement created for Player {current_player} at vertex {chosen_vertex} (settlement_id={settlement_id}). State updated to phase={next_phase.value}.")
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
        elif phase in (Phase.TO_PLACE_FIRST_ROAD, Phase.TO_PLACE_SECOND_ROAD):
            chosen_edge_key, road_id, next_phase, next_player, error = create_road(cursor, current_player, phase, last_settlement)
            if error:
                logger.error(f"Road creation error: {error}")
                abort(400, description = error)
            cursor.execute(
                "UPDATE state SET current_player = ?, phase = ?, last_settlement = NULL WHERE id = ?",
                (next_player, next_phase.value, ID_OF_STATE)
            )
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
                strengths = compute_strengths()
                response["strengths"] = strengths
                response["message"] += "\nGame setup complete. Strengths computed:\n" + json.dumps(strengths, indent = 4, sort_keys = True)
            return jsonify(response)
        else:
            logger.error(f"Invalid phase encountered: {phase.value}")
            abort(400, "Invalid phase in state.")