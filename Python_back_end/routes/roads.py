from ..db.database import Road
from ..db.database import get_db_session
from flask import Blueprint, jsonify


blueprint_for_route_roads = Blueprint("roads", __name__)


@blueprint_for_route_roads.route("/roads", methods = ["GET"])
def get_roads():
    with get_db_session() as session:
        roads = session.query(Road).all()
        list_of_dictionaries_of_road_information = [
            {"id": road.id, "player": road.player, "edge": road.edge} for road in roads
        ]
        return jsonify({"roads": list_of_dictionaries_of_road_information})