'''
app.py

Run from directory `Settlers_Of_Catan` with `python -m backend.app`.
'''


from flask_cors import CORS
from flask import Flask
from .config import INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED
from .config import PORT_ON_WHICH_BACK_END_LISTENS
from .config import URL_OF_FRONT_END
from .routes import blueprint_for_route_next
from .routes import blueprint_for_route_roads
from .routes import blueprint_for_route_root
from .routes import blueprint_for_route_settlements
from flask import jsonify
from .db.database import initialize_database
import logging
from .ai.continuous_training import start_continuous_training_in_background


logging.basicConfig(level = logging.INFO)
logger = logging.getLogger(__name__)


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
    logger.info("Registered blueprint for '/next'")
    app.register_blueprint(blueprint_for_route_roads)
    logger.info("Registered blueprint for '/roads'")
    app.register_blueprint(blueprint_for_route_root)
    logger.info("Registered blueprint for '/'")
    app.register_blueprint(blueprint_for_route_settlements)
    logger.info("Registered blueprint for '/settlements'")

    initialize_database()
    logger.info("Database initialized.")

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
    start_continuous_training_in_background()
    app = create_app()
    logger.info(f"Created back end on port {PORT_ON_WHICH_BACK_END_LISTENS}")
    app.run(
        port = PORT_ON_WHICH_BACK_END_LISTENS,
        debug = INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED,
        use_reloader = False
    )