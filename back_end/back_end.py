from flask import Flask, jsonify, request
from flask_cors import CORS
import math
import random
import sqlite3


BASE_NAME_OF_DATABASE = "game.db"

BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]


def calculate_distance(v1, v2):
    return math.sqrt((v1[0] - v2[0])**2 + (v1[1] - v2[1])**2)


def edge_already_exists(edge, edges):
        epsilon = 0.01
        (x1, y1, x2, y2) = edge
        for ex in edges:
            (ex1, ey1, ex2, ey2) = ex
            if ((abs(x1-ex1) < epsilon and abs(y1-ey1) < epsilon and abs(x2-ex2) < epsilon and abs(y2-ey2) < epsilon) or
                (abs(x1-ex2) < epsilon and abs(y1-ey2) < epsilon and abs(x2-ex1) < epsilon and abs(y2-ey1) < epsilon)):
                return True
        return False


def get_connection_to_database():
    connection = sqlite3.connect(BASE_NAME_OF_DATABASE)
    connection.row_factory = sqlite3.Row
    return connection


def get_edge_key(v1, v2):
    if v1[0] < v2[0] or (v1[0] == v2[0] and v1[1] <= v2[1]):
            return f"{v1[0]:.2f}-{v1[1]:.2f}_{v2[0]:.2f}-{v2[1]:.2f}"
    else:
            return f"{v2[0]:.2f}-{v2[1]:.2f}_{v1[0]:.2f}-{v1[1]:.2f}"


def get_edges():
    hexWidth = 100 / 6
    hexHeight = hexWidth * 1.1547
    hexOverlap = hexHeight * 0.25
    verticalSpacing = hexHeight - hexOverlap
    boardHeight = (len(BOARD) - 1) * verticalSpacing + hexHeight
    boardYOffset = (100 - boardHeight) / 2
    hexes = []
    for rowIndex, row in enumerate(BOARD):
        n = len(row)
        baseX = (100 - n * hexWidth) / 2
        y = boardYOffset + rowIndex * verticalSpacing
        for colIndex, hex_id in enumerate(row):
            x = baseX + colIndex * hexWidth
            hexes.append((hex_id, x, y))
    edges = []
    for (_, x, y) in hexes:
        verts = [
            (x + 0.5 * hexWidth, y),
            (x + hexWidth, y + 0.25 * hexHeight),
            (x + hexWidth, y + 0.75 * hexHeight),
            (x + 0.5 * hexWidth, y + hexHeight),
            (x, y + 0.75 * hexHeight),
            (x, y + 0.25 * hexHeight)
        ]
        for i in range(len(verts)):
            v1 = verts[i]
            v2 = verts[(i+1) % len(verts)]
            new_edge = (v1[0], v1[1], v2[0], v2[1])
            if not edge_already_exists(new_edge, edges):
                edges.append(new_edge)
    return edges


def get_vertices_with_labels():
    hexWidth = 100 / 6
    hexHeight = hexWidth * 1.1547
    hexOverlap = hexHeight * 0.25
    verticalSpacing = hexHeight - hexOverlap
    boardHeight = (len(BOARD) - 1) * verticalSpacing + hexHeight
    boardYOffset = (100 - boardHeight) / 2
    hexes = []
    for rowIndex, row in enumerate(BOARD):
        n = len(row)
        baseX = (100 - n * hexWidth) / 2
        y = boardYOffset + rowIndex * verticalSpacing
        for colIndex, hex_id in enumerate(row):
            x = baseX + colIndex * hexWidth
            hexes.append((hex_id, x, y))
    unique_vertices = []
    for (_, x, y) in hexes:
        potential_vertices = [
            (x + 0.5 * hexWidth, y),
            (x + hexWidth, y + 0.25 * hexHeight),
            (x + hexWidth, y + 0.75 * hexHeight),
            (x + 0.5 * hexWidth, y + hexHeight),
            (x, y + 0.75 * hexHeight),
            (x, y + 0.25 * hexHeight)
        ]
        for v in potential_vertices:
            if not vertex_already_exists(unique_vertices, v):
                unique_vertices.append(v)
    vertices_with_labels = [(f"V{(i+1):02d}", v[0], v[1]) for i, v in enumerate(unique_vertices)]
    return vertices_with_labels


def initialize_database():
    connection = get_connection_to_database()
    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS settlements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player INTEGER NOT NULL,
            vertex TEXT NOT NULL
        )
        '''
    )

    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS roads (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player INTEGER NOT NULL,
            edge TEXT NOT NULL
        )
        '''
    )

    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS state (
            id INTEGER PRIMARY KEY,
            current_player INTEGER NOT NULL,
            phase TEXT NOT NULL,
            last_settlement TEXT
        )
        '''
    )
    cur = connection.execute("SELECT COUNT(*) as count FROM state")
    row = cur.fetchone()
    if row["count"] == 0:
        connection.execute("INSERT INTO state (id, current_player, phase, last_settlement) VALUES (1, 1, 'phase to place first settlement', NULL)")
    
    connection.commit()
    connection.close()


def vertex_already_exists(vertex_list, v, epsilon = 0.01):
    for vx, vy in vertex_list:
        if abs(vx - v[0]) < epsilon and abs(vy - v[1]) < epsilon:
            return True
    return False


def vertex_key(v):
    return f"{v[0]:.2f}-{v[1]:.2f}"


app = Flask(__name__)


@app.route("/next", methods=["POST"])
def next_move():
    connection = get_connection_to_database()
    cursor = connection.cursor()

    # Read the current state (player, phase, and last settlement).
    cursor.execute("SELECT current_player, phase, last_settlement FROM state WHERE id = 1")
    row = cursor.fetchone()
    if row is None:
         current_player = 1
         phase = "phase to place first settlement"
         last_settlement = None
         cursor.execute("INSERT INTO state (id, current_player, phase, last_settlement) VALUES (1, 1, 'phase to place first settlement', NULL)")
         connection.commit()
    else:
         current_player = row["current_player"]
         phase = row["phase"]
         last_settlement = row["last_settlement"]

    if phase == "phase to place first settlement" or phase == "phase to place second settlement":
        cursor.execute("SELECT vertex FROM settlements")
        used_vertices = {r["vertex"] for r in cursor.fetchall()}

        vertices = get_vertices_with_labels()
        vertex_coords = {label: (x, y) for label, x, y in vertices}
        existing_coords = [vertex_coords[label] for label in used_vertices if label in vertex_coords]

        hexWidth = 100 / 6
        adjacency_threshold = 0.57735 * hexWidth + 0.1

        available = []
        for label, x, y in vertices:
            if label in used_vertices:
                continue
            too_close = False
            for ex in existing_coords:
                if math.sqrt((x - ex[0])**2 + (y - ex[1])**2) < adjacency_threshold:
                    too_close = True
                    break
            if not too_close:
                available.append(label)
        if not available:
            connection.close()
            return jsonify({"message": "No vertices are available."}), 400
        chosen_vertex = random.choice(available)
        cursor.execute(
            '''
            INSERT INTO settlements (player, vertex)
            VALUES (?, ?)
            ''',
            (current_player, chosen_vertex)
        )
        settlement_id = cursor.lastrowid
        if phase == 'phase to place first settlement':
            cursor.execute("UPDATE state SET phase = 'phase to place first road', last_settlement = ? WHERE id = 1", (chosen_vertex,))
        elif phase == 'phase to place second settlement':
            cursor.execute("UPDATE state SET phase = 'phase to place second road', last_settlement = ? WHERE id = 1", (chosen_vertex,))
        connection.commit()
        connection.close()
        return jsonify(
            {
                "message": f"Settlement created for Player {current_player}",
                "settlement": {
                    "id": settlement_id,
                    "player": current_player,
                    "vertex": chosen_vertex
                }
            }
        )
    elif phase == "phase to place first road" or phase == "phase to place second road":
        if not last_settlement:
            connection.close()
            return jsonify({"message": "Error: no settlement recorded for road placement."}), 400
        vertices = get_vertices_with_labels()
        vertex_coords = {label: (x, y) for label, x, y in vertices}
        if last_settlement not in vertex_coords:
            connection.close()
            return jsonify({"message": "Invalid last settlement vertex."}), 400
        settlement_coord = vertex_coords[last_settlement]
        all_edges = get_edges()
        epsilon = 0.01
        adjacent_edges = []
        for edge in all_edges:
            (x1, y1, x2, y2) = edge
            if (abs(x1 - settlement_coord[0]) < epsilon and abs(y1 - settlement_coord[1]) < epsilon) or \
               (abs(x2 - settlement_coord[0]) < epsilon and abs(y2 - settlement_coord[1]) < epsilon):
                adjacent_edges.append(edge)
        if not adjacent_edges:
            connection.close()
            return jsonify({"message": "No adjacent edges found for settlement."}), 400

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
            connection.close()
            return jsonify({"message": "No available roads adjacent to settlement."}), 400
        _, chosen_edge_key = random.choice(available_edges)
        cursor.execute("INSERT INTO roads (player, edge) VALUES (?, ?)", (current_player, chosen_edge_key))
        road_id = cursor.lastrowid
        next_player = 0
        if phase == "phase to place first road":
            phase_for_database = "phase to place first settlement"
            if current_player == 1:
                next_player = 2
            elif current_player == 2:
                next_player = 3
            elif current_player == 3:
                next_player = 3
                phase_for_database = "phase to place second settlement"
            cursor.execute("UPDATE state SET current_player = ?, phase = ?, last_settlement = NULL WHERE id = 1", (next_player, phase_for_database))
            connection.commit()
        elif phase == "phase to place second road":
            phase_for_database = "phase to place second settlement"
            if current_player == 3:
                next_player = 2
            elif current_player == 2:
                next_player = 1
            elif current_player == 1:
                next_player = 1
                phase_for_database = "turn"
            cursor.execute("UPDATE state SET current_player = ?, phase = ?, last_settlement = NULL WHERE id = 1", (next_player, phase_for_database))
            connection.commit()
        connection.close()
        return jsonify({
            "message": f"Road created for Player {current_player}",
            "road": {"id": road_id, "player": current_player, "edge": chosen_edge_key}
        })
    else:
        connection.close()
        return jsonify({"message": "Invalid phase in state."}), 400


@app.route("/roads", methods = ["GET"])
def get_roads():
    connection = get_connection_to_database()
    cursor = connection.cursor()
    cursor.execute("SELECT * FROM roads")
    roads = [dict(row) for row in cursor.fetchall()]
    connection.close()
    return jsonify({"roads": roads})


@app.route("/settlements", methods = ["GET"])
def get_settlements():
    connection = get_connection_to_database()
    cursor = connection.cursor()
    cursor.execute("SELECT * FROM settlements")
    settlements = [dict(row) for row in cursor.fetchall()]
    connection.close()
    return jsonify({"settlements": settlements})


@app.route("/", methods = ["GET"])
def listen_at_root():
    return jsonify(
        {
            "message": "You have requested information from endpoint '/'."
        }
    )


if __name__ == '__main__':
    initialize_database()
    CORS(
        app,
        resources = {
            r"/*": {
                "origins": "http://localhost:3000"
            }
        }
    )
    app.run(
        port = 5000,
        debug = True
    )