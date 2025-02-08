from flask_cors import CORS
from flask import Flask
from config import INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED
from config import PORT_ON_WHICH_BACK_END_LISTENS
from config import URL_OF_FRONT_END
from routes import blueprint_for_route_next
from routes import blueprint_for_route_roads
from routes import blueprint_for_route_root
from routes import blueprint_for_route_settlements
from flask import jsonify
from db.database import initialize_database
import logging


def create_app():
    app = Flask(__name__)

    CORS(
        app,
        resources = {
            r"/*": {
                "origins": URL_OF_FRONT_END
            }
        }
    )

    app.register_blueprint(blueprint_for_route_next)
    app.register_blueprint(blueprint_for_route_roads)
    app.register_blueprint(blueprint_for_route_root)
    app.register_blueprint(blueprint_for_route_settlements)

    initialize_database()

    logging.basicConfig(level = logging.INFO)

    @app.errorhandler(400)
    def handle_bad_request(e):
        app.logger.error(f"400 Bad Request: {e}")
        return (
            jsonify(
                {
                    "error": "Bad Request",
                    "message": getattr(
                        e,
                        'description',
                        str(e)
                    )
                }
            ),
            400
        )
    
    @app.errorhandler(404)
    def handle_not_found(e):
        app.logger.error(f"404 Not Found: {e}")
        return (
            jsonify(
                {
                    "error": "Not Found",
                    "message": getattr(
                        e,
                        'description',
                        str(e)
                    )
                }
            ),
            404
        )
    
    @app.errorhandler(500)
    def handle_internal_server_error(e):
        app.logger.error(f"500 Internal Server Error: {e}")
        return (
            jsonify(
                {
                    "error": "Internal Server Error",
                    "message": "An unexpected error occurred."
                }
            ),
            500
        )
    
    @app.errorhandler(Exception)
    def handle_exception(e):
        code = 500
        if hasattr(e, 'code') and isinstance(e.code, int):
            code = e.code
        app.logger.error(f"Unhandled Exception: {e}")
        return (
            jsonify(
                {
                    "error": "Server Error",
                    "message": str(e)
                }
            ),
            code
        )
    
    return app


if __name__ == '__main__':
    app = create_app()
    app.run(
        port = PORT_ON_WHICH_BACK_END_LISTENS,
        debug = INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED
    )