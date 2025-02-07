from db.database import initialize_database
from flask import Flask
from flask_cors import CORS
from routes import blueprint_for_route_next, blueprint_for_route_roads, blueprint_for_route_root, blueprint_for_route_settlements


if __name__ == '__main__':
    app = Flask(__name__)
    CORS(
        app,
        resources = {
            r"/*": {
                "origins": "http://localhost:3000"
            }
        }
    )
    app.register_blueprint(blueprint_for_route_next)
    app.register_blueprint(blueprint_for_route_roads)
    app.register_blueprint(blueprint_for_route_root)
    app.register_blueprint(blueprint_for_route_settlements)
    initialize_database()
    app.run(
        port = 5000,
        debug = True
    )