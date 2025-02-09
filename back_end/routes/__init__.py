from routes.next import blueprint_for_route_next
from routes.roads import blueprint_for_route_roads
from routes.settlements import blueprint_for_route_settlements
from routes.root import blueprint_for_route_root


__all__ = [
    "blueprint_for_route_next",
    "blueprint_for_route_roads",
    "blueprint_for_route_root",
    "blueprint_for_route_settlements"
]