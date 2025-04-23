export type EdgeKey = string;

export type StructureType = "city" | "settlement" | "wall";

export type Player = 1 | 2 | 3;

export type ResetResponse = { message: string };

export type VertexLabel = string;


export interface Dice {
    yellowProductionDie: number;
    redProductionDie: number;
    whiteEventDie: 'yellow' | 'green' | 'blue' | 'black';
}


export interface Edge {
    x1: number;
    y1: number;
    x2: number;
    y2: number;
}

export interface HexInformation {
    id: ID_Of_Hex;
    x: number;
    y: number;
}

export interface ResourcesByKind {
    brick: number;
    grain: number;
    lumber: number;
    ore: number;
    wool: number;
    cloth: number;
    coin: number;
    paper: number;
}


export interface RecommendMoveResponse {
    move: string;
    moveType: string;
}


export type ID_Of_Hex = keyof typeof idToColor;

export type AutomateMoveResponse = {
    message: string;
    moveType: string;
    dice?: Dice;
    totalResources: Totals;
    gainedResources?: Totals;
    possibleNextMoves: {
        player: Player;
        nextPlayerWillRollDice: boolean;
        vertices: Record<string, StructureType[]>;
        edges: string[];
    }
}

export interface Road {
    id: number;
    player: Player;
    edge: EdgeKey;
}

export interface WallInformation {
    id: number;
    player: Player;
    vertex: VertexLabel;
}

export interface Settlement {
    id: number;
    player: Player;
    vertex: VertexLabel;
}

export type Totals = Record<string, ResourcesByKind>;

export interface VertexInformation {
    x: number;
    y: number;
    label?: VertexLabel;
}


export const colorOf = {
    Brick: "rgb(170, 74, 68)",
    Desert: "rgb(245, 213, 161)",
    Grain: "rgb(250, 219, 94)",
    Ore: "rgb(128, 128, 128)",
    Wood: "rgb(34, 139, 34)",
    Wool: "rgb(121, 208, 33)",
} as const;
  
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

export const portMapping: Record<string, string> = {
    "V01": "3:1",
    "V06": "3:1",
    "V07": "Grain",
    "V08": "Grain",
    "V13": "Ore",
    "V23": "Ore",
    "V36": "3:1",
    "V37": "3:1",
    "V46": "Wool",
    "V47": "Wool",
    "V51": "3:1",
    "V52": "3:1",
    "V49": "3:1",
    "V50": "3:1",
    "V41": "Brick",
    "V27": "Brick",
    "V17": "Wood",
    "V18": "Wood",
};

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