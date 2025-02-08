from db.database import get_db_connection
from flask import Blueprint, jsonify


blueprint_for_route_roads = Blueprint("roads", __name__)


@blueprint_for_route_roads.route("/roads", methods = ["GET"])
def get_roads():
    with get_db_connection() as connection:
        cursor = connection.cursor()
        cursor.execute("SELECT * FROM roads")
        roads = [dict(row) for row in cursor.fetchall()]
        return jsonify({"roads": roads})