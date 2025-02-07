BOARD = [
    ["H01", "H02", "H03"],
    ["H04", "H05", "H06", "H07"],
    ["H08", "H09", "H10", "H11", "H12"],
    ["H13", "H14", "H15", "H16"],
    ["H17", "H18", "H19"]
]

def edge_already_exists(edge, edges, epsilon):
    (x1, y1, x2, y2) = edge
    for ex in edges:
        (ex1, ey1, ex2, ey2) = ex
        if (
            (
                abs(x1 - ex1) < epsilon and
                abs(y1 - ey1) < epsilon and
                abs(x2 - ex2) < epsilon and
                abs(y2 - ey2) < epsilon
            ) or
            (
                abs(x1 - ex2) < epsilon and
                abs(y1 - ey2) < epsilon and
                abs(x2 - ex1) < epsilon and
                abs(y2 - ey1) < epsilon
            )
        ):
            return True
    return False

def vertex_already_exists(vertex_list, v, epsilon):
    for vx, vy in vertex_list:
        if (
            abs(vx - v[0]) < epsilon and
            abs(vy - v[1]) < epsilon
        ):
            return True
    return False


def get_edge_key(v1, v2):
    if (
        v1[0] < v2[0] or
        (v1[0] == v2[0] and v1[1] <= v2[1])
    ):
        return f"{v1[0]:.2f}-{v1[1]:.2f}_{v2[0]:.2f}-{v2[1]:.2f}"
    else:
        return f"{v2[0]:.2f}-{v2[1]:.2f}_{v1[0]:.2f}-{v1[1]:.2f}"
    

def get_edges():
    hexWidth = 100 / 6
    hexHeight = hexWidth * 1.1547
    hexOverlap = hexHeight * 0.25
    verticalSpacing = hexHeight - hexOverlap
    boardHeight = (len(BOARD) - 1) * verticalSpacing + hexHeight
    boardYOffset = (100 - boardHeight) / 2
    hexes = []
    for rowIndex, row in enumerate(BOARD):
        n = len(row)
        baseX = (100 - n * hexWidth) / 2
        y = boardYOffset + rowIndex * verticalSpacing
        for colIndex, hex_id in enumerate(row):
            x = baseX + colIndex * hexWidth
            hexes.append((hex_id, x, y))
    edges = []
    for (_, x, y) in hexes:
        verts = [
            (x + 0.5 * hexWidth, y),
            (x + hexWidth, y + 0.25 * hexHeight),
            (x + hexWidth, y + 0.75 * hexHeight),
            (x + 0.5 * hexWidth, y + hexHeight),
            (x, y + 0.75 * hexHeight),
            (x, y + 0.25 * hexHeight)
        ]
        for i in range(len(verts)):
            v1 = verts[i]
            v2 = verts[(i+1) % len(verts)]
            new_edge = (v1[0], v1[1], v2[0], v2[1])
            epsilon = 0.01
            if not edge_already_exists(new_edge, edges, epsilon):
                edges.append(new_edge)
    return edges


def get_vertices_with_labels():
    hexWidth = 100 / 6
    hexHeight = hexWidth * 1.1547
    hexOverlap = hexHeight * 0.25
    verticalSpacing = hexHeight - hexOverlap
    boardHeight = (len(BOARD) - 1) * verticalSpacing + hexHeight
    boardYOffset = (100 - boardHeight) / 2
    hexes = []
    for rowIndex, row in enumerate(BOARD):
        n = len(row)
        baseX = (100 - n * hexWidth) / 2
        y = boardYOffset + rowIndex * verticalSpacing
        for colIndex, hex_id in enumerate(row):
            x = baseX + colIndex * hexWidth
            hexes.append((hex_id, x, y))
    unique_vertices = []
    for (_, x, y) in hexes:
        potential_vertices = [
            (x + 0.5 * hexWidth, y),
            (x + hexWidth, y + 0.25 * hexHeight),
            (x + hexWidth, y + 0.75 * hexHeight),
            (x + 0.5 * hexWidth, y + hexHeight),
            (x, y + 0.75 * hexHeight),
            (x, y + 0.25 * hexHeight)
        ]
        for v in potential_vertices:
            epsilon = 0.01
            if not vertex_already_exists(unique_vertices, v, epsilon):
                unique_vertices.append(v)
    vertices_with_labels = [(f"V{(i+1):02d}", v[0], v[1]) for i, v in enumerate(unique_vertices)]
    return vertices_with_labels