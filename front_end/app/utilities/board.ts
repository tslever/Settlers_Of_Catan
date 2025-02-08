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

export type ID_Of_Hex = keyof typeof idToColor;

export const tokenMapping: { [key in ID_Of_Hex]: number | null } = {
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

const BOARD: ID_Of_Hex[][] = [
    ['H01', 'H02', 'H03'],
    ['H04', 'H05', 'H06', 'H07'],
    ['H08', 'H09', 'H10', 'H11', 'H12'],
    ['H13', 'H14', 'H15', 'H16'],
    ['H17', 'H18', 'H19']
];

const WIDTH_OF_BOARD_IN_VMIN = 100; // vmin
const NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6;
const WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD;
const RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = Math.tan(Math.PI / 6);
const RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON;
const HEIGHT_OF_HEX = WIDTH_OF_HEX * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX;
const DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW = HEIGHT_OF_HEX / 4;
const DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW = HEIGHT_OF_HEX - DISTANCE_BETWEEN_BOTTOM_OF_HEX_IN_FIRST_ROW_AND_TOP_OF_HEX_IN_SECOND_ROW;
const NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST = BOARD.length - 1;
const HEIGHT_OF_BOARD = HEIGHT_OF_HEX + DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW * NUMBER_OF_ROWS_OF_HEXES_AFTER_FIRST;
const VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES = (100 - HEIGHT_OF_BOARD) / 2; // vmin
const MARGIN_OF_ERROR = 0.01;

type Hex = { id: ID_Of_Hex; x: number; y: number };

export const hexes: Hex[] = BOARD.flatMap((row, index_of_row) => {
    const vertical_position_of_hex = VERTICAL_POSITION_OF_FIRST_ROW_OF_HEXES + index_of_row * DISTANCE_BETWEEN_TOP_OF_HEX_IN_ONE_ROW_AND_TOP_OF_HEX_IN_NEXT_ROW;
    const number_of_hexes_in_row = row.length;
    const horizontal_position_of_first_hex_in_row = (WIDTH_OF_BOARD_IN_VMIN - number_of_hexes_in_row * WIDTH_OF_HEX) / 2;
    return row.map((id_of_hex, index_of_hex) => ({
        id: id_of_hex,
        x: horizontal_position_of_first_hex_in_row + index_of_hex * WIDTH_OF_HEX,
        y: vertical_position_of_hex
    }));
});

type VertexCoord = { x: number; y: number };

function isClose(v1: VertexCoord, v2: VertexCoord, margin_of_error: number): boolean {
    return Math.abs(v1.x - v2.x) < margin_of_error && Math.abs(v1.y - v2.y) < margin_of_error;
}

export const vertices: VertexCoord[] = (() => {
    const uniqueVertices: VertexCoord[] = [];
    hexes.forEach(({ x, y }) => {
        const potentialVertices: VertexCoord[] = [
            { x: x + 0.5 * WIDTH_OF_HEX, y },
            { x: x + WIDTH_OF_HEX, y: y + 0.25 * HEIGHT_OF_HEX },
            { x: x + WIDTH_OF_HEX, y: y + 0.75 * HEIGHT_OF_HEX },
            { x: x + 0.5 * WIDTH_OF_HEX, y: y + HEIGHT_OF_HEX },
            { x, y: y + 0.75 * HEIGHT_OF_HEX },
            { x, y: y + 0.25 * HEIGHT_OF_HEX }
        ];
        potentialVertices.forEach(v => {
            if (!uniqueVertices.some(existing => isClose(existing, v, MARGIN_OF_ERROR))) {
                uniqueVertices.push(v);
            }
        })
    })
    return uniqueVertices;
})();

type Edge = { x1: number; y1: number; x2: number; y2: number };

function isEdgeClose(e1: Edge, e2: Edge, margin_of_error: number): boolean {
    const sameOrder =
        isClose({ x: e1.x1, y: e1.y1 }, { x: e2.x1, y: e2.y1 }, margin_of_error) &&
        isClose({ x: e1.x2, y: e1.y2 }, { x: e2.x2, y: e2.y2 }, margin_of_error);
    const swappedOrder =
        isClose({ x: e1.x1, y: e1.y1 }, { x: e2.x2, y: e2.y2 }, margin_of_error) &&
        isClose({ x: e1.x2, y: e1.y2 }, { x: e2.x1, y: e2.y1 }, margin_of_error);
    return sameOrder || swappedOrder;
}

export const edges: Edge[] = (() => {
    const edgesArray: Edge[] = [];
    hexes.forEach(({ x, y }) => {
        const vertices: VertexCoord[] = [
            { x: x + 0.5 * WIDTH_OF_HEX, y },
            { x: x + WIDTH_OF_HEX, y: y + 0.25 * HEIGHT_OF_HEX },
            { x: x + WIDTH_OF_HEX, y: y + 0.75 * HEIGHT_OF_HEX },
            { x: x + 0.5 * WIDTH_OF_HEX, y: y + HEIGHT_OF_HEX },
            { x, y: y + 0.75 * HEIGHT_OF_HEX },
            { x, y: y + 0.25 * HEIGHT_OF_HEX }
        ];
        for (let i = 0; i < vertices.length; i++) {
            const v1 = vertices[i];
            const v2 = vertices[(i + 1) % vertices.length];
            const newEdge: Edge = { x1: v1.x, y1: v1.y, x2: v2.x, y2: v2.y };
            if (!edgesArray.some(existing => isEdgeClose(existing, newEdge, MARGIN_OF_ERROR))) {
                edgesArray.push(newEdge);
            }
        }
    });
    return edgesArray;
})();