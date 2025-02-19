import logging
import sys


def set_up_logging(level = logging.INFO):
    formatter = logging.Formatter(fmt = "%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)
    root = logging.getLogger()
    root.setLevel(level)
    if root.hasHandlers():
        root.handlers.clear()
    root.addHandler(handler)

set_up_logging()