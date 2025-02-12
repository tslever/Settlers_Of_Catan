from ..db.database import get_db_connection
from flask import Blueprint, jsonify


blueprint_for_route_settlements = Blueprint("settlements", __name__)


@blueprint_for_route_settlements.route("/settlements", methods = ["GET"])
def get_settlements():
    with get_db_connection() as connection:
        cursor = connection.cursor()
        cursor.execute("SELECT * FROM settlements")
        settlements = [dict(row) for row in cursor.fetchall()]
        return jsonify({"settlements": settlements})