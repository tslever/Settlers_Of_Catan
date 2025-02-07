from db.database import get_connection_to_database
from flask import Blueprint, jsonify


blueprint_for_route_roads = Blueprint("roads", __name__)


@blueprint_for_route_roads.route("/roads", methods = ["GET"])
def get_roads():
    connection = get_connection_to_database()
    cursor = connection.cursor()
    cursor.execute("SELECT * FROM roads")
    roads = [dict(row) for row in cursor.fetchall()]
    connection.close()
    return jsonify({"roads": roads})