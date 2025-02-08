from flask import Blueprint
from enum import Enum
from flask import abort
from utilities.board import all_edges_of_all_hexes
from db.database import get_connection_to_database
from utilities.board import get_edge_key
from utilities.board import vertices_with_labels
from flask import jsonify
import math
import random


blueprint_for_route_next = Blueprint("next", __name__)


ID_OF_STATE = 1
RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = math.tan(math.pi / 6)
WIDTH_OF_HEX = 100 / 6 # vmin
MARGIN_OF_ERROR = 0.01
THRESHOLD_TO_DETERMINE_WHETHER_TWO_VERTICES_ARE_ADJACENT = RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON * WIDTH_OF_HEX + MARGIN_OF_ERROR


class Phase(Enum):
    TO_PLACE_FIRST_SETTLEMENT = "phase to place first settlement"
    TO_PLACE_FIRST_ROAD = "phase to place first road"
    TO_PLACE_SECOND_SETTLEMENT = "phase to place second settlement"
    TO_PLACE_SECOND_ROAD = "phase to place second road"
    TURN = "turn"


def create_settlement(cursor, current_player, phase: Phase):
    cursor.execute("SELECT vertex FROM settlements")
    used_vertices = {r["vertex"] for r in cursor.fetchall()}
    vertex_coords = {label: (x, y) for label, x, y in vertices_with_labels}
    existing_coords = [vertex_coords[label] for label in used_vertices if label in vertex_coords]
    available = []
    for label, x, y in vertices_with_labels:
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
        return None, None, None, "No vertices are available."
    chosen_vertex = random.choice(available)
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
        return None, None, None, "Invalid phase for settlement placement."
    return chosen_vertex, settlement_id, next_phase, None
    

def create_road(cursor, current_player, phase: Phase, last_settlement):
    if not last_settlement:
        return None, None, None, None, "No settlement recorded for road placement."
    vertex_coords = {label: (x, y) for label, x, y in vertices_with_labels}
    if last_settlement not in vertex_coords:
        return None, None, None, None, "Invalid last settlement vertex."
    settlement_coord = vertex_coords[last_settlement]
    adjacent_edges = []
    for edge in all_edges_of_all_hexes:
        (x1, y1, x2, y2) = edge
        if (
            (abs(x1 - settlement_coord[0]) < MARGIN_OF_ERROR and abs(y1 - settlement_coord[1]) < MARGIN_OF_ERROR) or
            (abs(x2 - settlement_coord[0]) < MARGIN_OF_ERROR and abs(y2 - settlement_coord[1]) < MARGIN_OF_ERROR)
        ):
            adjacent_edges.append(edge)
    if not adjacent_edges:
        return None, None, None, None, "No adjacent edges found for settlement."
    cursor.execute("SELECT edge FROM roads")
    used_edges = {row["edge"] for row in cursor.fetchall()}
    available_edges = []
    for edge in adjacent_edges:
        v1 = (edge[0], edge[1])
        v2 = (edge[2], edge[3])
        edge_key = get_edge_key(v1, v2)
        if edge_key not in used_edges:
            available_edges.append((edge, edge_key))
    if not available_edges:
        return None, None, None, None, "No available roads adjacent to settlement."
    _, chosen_edge_key = random.choice(available_edges)
    cursor.execute("INSERT INTO roads (player, edge) VALUES (?, ?)", (current_player, chosen_edge_key))
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
            abort(500, description = "Current player does not exist.")
    else:
        return None, None, None, None, "Invalid phase for road placement."
    return chosen_edge_key, road_id, next_phase, next_player, None


@blueprint_for_route_next.route("/next", methods = ["POST"])
def next_move():
    connection = get_connection_to_database()
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
        connection.commit()
    else:
        current_player = row["current_player"]
        try:
            phase = Phase(row["phase"])
        except ValueError:
            connection.close()
            abort(500, description = "Invalid phase in state.")
        last_settlement = row["last_settlement"]
    if phase in (Phase.TO_PLACE_FIRST_SETTLEMENT, Phase.TO_PLACE_SECOND_SETTLEMENT):
        chosen_vertex, settlement_id, next_phase, error = create_settlement(cursor, current_player, phase)
        if error:
            connection.close()
            abort(400, description = error)
        cursor.execute(
            "UPDATE state SET phase = ?, last_settlement = ? WHERE id = ?",
            (next_phase.value, chosen_vertex, ID_OF_STATE)
        )
        connection.commit()
        connection.close()
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
            connection.close()
            abort(400, description = error)
        cursor.execute(
            "UPDATE state SET current_player = ?, phase = ?, last_settlement = NULL WHERE id = ?",
            (next_player, next_phase.value, ID_OF_STATE)
        )
        connection.commit()
        connection.close()
        return jsonify(
            {
                "message": f"Road created for Player {current_player}",
                "moveType": "road",
                "road": {
                    "id": road_id,
                    "player": current_player,
                    "edge": chosen_edge_key
                }
            }
        )
    else:
        connection.close()
        abort(400, "Invalid phase in state.")