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


def get_connection_to_database():
    connection = sqlite3.connect(BASE_NAME_OF_DATABASE)
    connection.row_factory = sqlite3.Row
    return connection


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
        CREATE TABLE IF NOT EXISTS state (
            id INTEGER PRIMARY KEY,
            current_player INTEGER NOT NULL
        )
        '''
    )
    cur = connection.execute("SELECT COUNT(*) as count FROM state")
    row = cur.fetchone()
    if row["count"] == 0:
        connection.execute("INSERT INTO state (id, current_player) VALUES (1, 1)")
    
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


@app.route("/settlement", methods = ["POST"])
def add_settlement():
    initialize_database()
    connection = get_connection_to_database()
    cursor = connection.cursor()

    cursor.execute("SELECT vertex FROM settlements")
    used_vertex_labels = {row["vertex"] for row in cursor.fetchall()}

    vertices = get_vertices_with_labels()
    vertex_coords = {label: (x, y) for label, x, y in vertices}
    existing_coords = [vertex_coords[label] for label in used_vertex_labels if label in vertex_coords]

    hexWidth = 100 / 6
    adjacency_threshold = 0.57735 * hexWidth + 0.1

    available = []
    for label, x, y in vertices:
        if label in used_vertex_labels:
            continue
        too_close = False
        for ex in existing_coords:
            if calculate_distance((x, y), ex) < adjacency_threshold:
                too_close = True
                break
        if not too_close:
            available.append(label)
    if not available:
        connection.close()
        return jsonify({"message": "No vertices are available."}), 400
    
    cursor.execute("SELECT current_player FROM state WHERE id = 1")
    row = cursor.fetchone()
    current_player = row["current_player"] if row else 1

    chosen_vertex = random.choice(available)

    cursor.execute(
        '''
        INSERT INTO settlements (player, vertex)
        VALUES (?, ?)
        ''',
        (current_player, chosen_vertex)
    )

    next_player = (current_player % 3) + 1
    cursor.execute("UPDATE state SET current_player = ? WHERE id = 1", (next_player,))

    connection.commit()
    settlement_id = cursor.lastrowid
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


@app.route("/settlements", methods = ["GET"])
def get_settlements():
    initialize_database()
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