import type { Edge } from "./types";
import type { HexInformation } from "./types";
import type { VertexInformation } from "./types";
import boardGeometry from "../../board_geometry.json";


export interface BoardGeometry {
    hexes: HexInformation[];
    vertices: VertexInformation[];
    edges: Edge[];
}

export const geometry = boardGeometry as BoardGeometry;
export const hexes: HexInformation[] = geometry.hexes;
export const vertices: VertexInformation[] = geometry.vertices;
export const edges: Edge[] = geometry.edges;