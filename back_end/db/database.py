'''
Instructions to set up database
Download MySQL Community Installer for Windows from https://dev.mysql.com/downloads/installer/ .
Install and configure MySQL.
Check that MySQL is running by running in Git Bash on Windows command `tasklist | grep mysqld`.

Start MySQL Workbench 8.0 CE, connect to Local instance MySQL80, and open SQL scripts `create_game_database.sql` and `delete_game_database.sql`.
Press the lightning button to create the game database.

Alternatively, in Git Bash, run `winpty mysql -u root -p`.
Enter the password for the root user.
Run the commands in `create_game_database.sql` to create the database.
Run command exit.

Run `pip install sqlalchemy pymysql cryptography`.
'''


from sqlalchemy import Column
from back_end.config import DB_HOST
from back_end.config import DB_NAME
from back_end.config import DB_PASSWORD
from back_end.config import DB_USERNAME
from sqlalchemy import Integer
from sqlalchemy import String
from contextlib import contextmanager
from sqlalchemy import create_engine
from sqlalchemy.orm import declarative_base
import logging
import os
from sqlalchemy.orm import sessionmaker


logger = logging.getLogger(__name__)


DATABASE_URL = os.getenv(
    "DATABASE_URL",
    f"mysql+pymysql://{DB_USERNAME}:{DB_PASSWORD}@{DB_HOST}/{DB_NAME}"
)
engine_with_connection_pool = create_engine(
    DATABASE_URL,
    pool_size = 10,
    max_overflow = 20,
    pool_pre_ping = True
)
SessionFactory = sessionmaker(autocommit = False, autoflush = False, bind = engine_with_connection_pool)
BaseClassForOrmModels = declarative_base()


class Settlement(BaseClassForOrmModels):
    __tablename__ = "settlements"
    id = Column(Integer, primary_key = True, autoincrement = True)
    player = Column(Integer, nullable = False)
    vertex = Column(String(length = 50), nullable = False)


class Road(BaseClassForOrmModels):
    __tablename__ = "roads"
    id = Column(Integer, primary_key = True, autoincrement = True)
    player = Column(Integer, nullable = False)
    edge = Column(String(length = 50), nullable = False)


class State(BaseClassForOrmModels):
    __tablename__ = "state"
    id = Column(Integer, primary_key = True)
    current_player = Column(Integer, nullable = False)
    phase = Column(String(length = 100), nullable = False)
    last_settlement = Column(String(length = 50), nullable = True)


@contextmanager
def get_db_session():
    session = SessionFactory()
    try:
        yield session
        session.commit()
    except Exception as e:
        session.rollback()
        logger.exception("Database session error")
        raise e
    finally:
        session.close()


def initialize_database():
    '''
    Create all tables if they do not exist and ensure that the initial state record exists.
    '''
    BaseClassForOrmModels.metadata.create_all(bind = engine_with_connection_pool)
    with get_db_session() as session:
        state = session.query(State).filter_by(id = 1).first()
        if not state:
            initial_state = State(
                id = 1,
                current_player = 1,
                phase = "phase to place first settlement",
                last_settlement = None
            )
            session.add(initial_state)
            session.commit()
            logger.info("State table was initialized with the initial state.")
        else:
            logger.info("State table was already initialized.")