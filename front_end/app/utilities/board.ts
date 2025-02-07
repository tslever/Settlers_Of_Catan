const colorOf = {
    Brick: 'rgb(170, 74, 68)',
    Desert: 'rgb(245, 213, 161)',
    Grain: 'rgb(250, 219, 94)',
    Ore: 'rgb(128, 128, 128)',
    Wood: 'rgb(34, 139, 34)',
    Wool: 'rgb(121, 208, 33)',
};

export const idToColor = {
    H01: colorOf.Ore,
    H02: colorOf.Wool,
    H03: colorOf.Wood,
    H04: colorOf.Grain,
    H05: colorOf.Brick,
    H06: colorOf.Wool,
    H07: colorOf.Brick,
    H08: colorOf.Grain,
    H09: colorOf.Wood,
    H10: colorOf.Desert,
    H11: colorOf.Wood,
    H12: colorOf.Ore,
    H13: colorOf.Wood,
    H14: colorOf.Ore,
    H15: colorOf.Grain,
    H16: colorOf.Wool,
    H17: colorOf.Brick,
    H18: colorOf.Grain,
    H19: colorOf.Wool,
} as const;

export type HexID = keyof typeof idToColor;

export const tokenMapping: { [key in HexID]: number | null } = {
    H01: 10,
    H02: 2,
    H03: 9,
    H04: 12,
    H05: 6,
    H06: 4,
    H07: 10,
    H08: 9,
    H09: 11,
    H10: null,
    H11: 3,
    H12: 8,
    H13: 8,
    H14: 3,
    H15: 4,
    H16: 5,
    H17: 5,
    H18: 6,
    H19: 11,
} as const;

const board: HexID[][] = [
    ['H01', 'H02', 'H03'],
    ['H04', 'H05', 'H06', 'H07'],
    ['H08', 'H09', 'H10', 'H11', 'H12'],
    ['H13', 'H14', 'H15', 'H16'],
    ['H17', 'H18', 'H19']
];

const hexWidth = 100 / 6;
const hexHeight = hexWidth * 1.1547;
const hexOverlap = hexHeight * 0.25;
const verticalSpacing = hexHeight - hexOverlap;
const boardHeight = (board.length - 1) * verticalSpacing + hexHeight;
const boardYOffset = (100 - boardHeight) / 2;

type HexPosition = { id: HexID; x: number; y: number };

export const hexes: HexPosition[] = board.flatMap((row, rowIndex) => {
    const n = row.length;
    const baseX = (100 - n * hexWidth) / 2;
    const y = boardYOffset + rowIndex * verticalSpacing;
    return row.map((id, colIndex) => ({
        id,
        x: baseX + colIndex * hexWidth,
        y
    }));
});

type VertexCoord = { x: number; y: number };

function isClose(v1: VertexCoord, v2: VertexCoord, epsilon: number): boolean {
    return Math.abs(v1.x - v2.x) < epsilon && Math.abs(v1.y - v2.y) < epsilon;
}

const epsilon = 0.01;

export const vertices: VertexCoord[] = (() => {
    const uniqueVertices: VertexCoord[] = [];
    hexes.forEach(({ x, y }) => {
        const potentialVertices: VertexCoord[] = [
            { x: x + 0.5 * hexWidth, y },
            { x: x + hexWidth, y: y + 0.25 * hexHeight },
            { x: x + hexWidth, y: y + 0.75 * hexHeight },
            { x: x + 0.5 * hexWidth, y: y + hexHeight },
            { x, y: y + 0.75 * hexHeight },
            { x, y: y + 0.25 * hexHeight }
        ];
        potentialVertices.forEach(v => {
            if (!uniqueVertices.some(existing => isClose(existing, v, epsilon))) {
                uniqueVertices.push(v);
            }
        })
    })
    return uniqueVertices;
})();

type Edge = { x1: number; y1: number; x2: number; y2: number };

function isEdgeClose(e1: Edge, e2: Edge, epsilon: number): boolean {
    const sameOrder =
        isClose({ x: e1.x1, y: e1.y1 }, { x: e2.x1, y: e2.y1 }, epsilon) &&
        isClose({ x: e1.x2, y: e1.y2 }, { x: e2.x2, y: e2.y2 }, epsilon);
    const swappedOrder =
        isClose({ x: e1.x1, y: e1.y1 }, { x: e2.x2, y: e2.y2 }, epsilon) &&
        isClose({ x: e1.x2, y: e1.y2 }, { x: e2.x1, y: e2.y1 }, epsilon);
    return sameOrder || swappedOrder;
}

export const edges: Edge[] = (() => {
    const edgesArray: Edge[] = [];
    hexes.forEach(({ x, y }) => {
        const verts: VertexCoord[] = [
            { x: x + 0.5 * hexWidth, y },
            { x: x + hexWidth, y: y + 0.25 * hexHeight },
            { x: x + hexWidth, y: y + 0.75 * hexHeight },
            { x: x + 0.5 * hexWidth, y: y + hexHeight },
            { x, y: y + 0.75 * hexHeight },
            { x, y: y + 0.25 * hexHeight }
        ];
        for (let i = 0; i < verts.length; i++) {
            const v1 = verts[i];
            const v2 = verts[(i + 1) % verts.length];
            const newEdge: Edge = { x1: v1.x, y1: v1.y, x2: v2.x, y2: v2.y };
            if (!edgesArray.some(existing => isEdgeClose(existing, newEdge, epsilon))) {
                edgesArray.push(newEdge);
            }
        }
    });
    return edgesArray;
})();