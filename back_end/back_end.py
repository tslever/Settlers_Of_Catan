from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
import random


BASE_NAME_OF_DATABASE = "settlers.db"

BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]


def get_list_of_labels_of_all_vertices():
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

    vertex_set = {}
    for (_, x, y) in hexes:
        verts = [
            (x + 0.5 * hexWidth, y),
            (x + hexWidth, y + 0.25 * hexHeight),
            (x + hexWidth, y + 0.75 * hexHeight),
            (x + 0.5 * hexWidth, y + hexHeight),
            (x, y + 0.75 * hexHeight),
            (x, y + 0.25 * hexHeight)
        ]
        for v in verts:
            key_str = vertex_key(v)
            if key_str not in vertex_set:
                vertex_set[key_str] = v
    
    vertices_list = list(vertex_set.values())

    list_of_labels_of_all_vertices = [f"V{(i+1):02d}" for i in range(len(vertices_list))]
    return list_of_labels_of_all_vertices


def get_connection_to_database():
    connection = sqlite3.connect(BASE_NAME_OF_DATABASE)
    connection.row_factory = sqlite3.Row
    return connection


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
    connection.commit()
    connection.close()


def vertex_key(v):
    return f"{v[0]:.2f}-{v[1]:.2f}"


app = Flask(__name__)


@app.route("/", methods = ["GET"])
def listen_at_root():
    return jsonify(
        {
            "message": "You have requested information from endpoint '/'."
        }
    )


@app.route("/settlement", methods = ["POST"])
def add_settlement():
    connection = get_connection_to_database()
    cursor = connection.cursor()

    cursor.execute("SELECT vertex FROM settlements")
    used_vertices = {row["vertex"] for row in cursor.fetchall()}

    cursor.execute("SELECT COUNT(*) as count FROM settlements")
    count = cursor.fetchone()["count"]
    current_player = (count % 3) + 1

    available = [vertex for vertex in get_list_of_labels_of_all_vertices() if vertex not in used_vertices]
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