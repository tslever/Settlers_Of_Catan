from flask import Blueprint
from ..db.database import City
from ..db.database import get_db_session
from flask import jsonify


blueprint_for_route_cities = Blueprint("cities", __name__)


@blueprint_for_route_cities.route("/cities", methods = ["GET"])
def get_cities():
    with get_db_session() as session:
        cities = session.query(City).all()
        list_of_dictionaries_of_city_information = [
            {"id": city.id, "player": city.player, "vertex": city.vertex} for city in cities
        ]
        return jsonify({"cities": list_of_dictionaries_of_city_information})