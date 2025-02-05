'use client'

import React, { useEffect, useState } from 'react';

const colorOf = {
  Brick: 'rgb(170, 74, 68)',
  Desert: 'rgb(245, 213, 161)',
  Grain: 'rgb(250, 219, 94)',
  Ore: 'rgb(128, 128, 128)',
  Wood: 'rgb(34, 139, 34)',
  Wool: 'rgb(121, 208, 33)',
};

const idToColor = {
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

const tokenMapping: { [key in HexID]: number | null } = {
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
const hexes: HexPosition[] = [];

board.forEach((row, rowIndex) => {
  const n = row.length;
  let baseX = (100 - n * hexWidth) / 2;
  const y = boardYOffset + rowIndex * verticalSpacing;
  row.forEach((id, colIndex) => {
    const x = baseX + colIndex * hexWidth;
    hexes.push({ id, x, y });
  });
});

type VertexCoord = { x: number; y: number };

function isClose(v1: VertexCoord, v2: VertexCoord, epsilon: number): boolean {
  return Math.abs(v1.x - v2.x) < epsilon && Math.abs(v1.y - v2.y) < epsilon;
}

const epsilon = 0.01;
const uniqueVertices: VertexCoord[] = [];

hexes.forEach(({ x, y }) => {
  const potentialVertices: VertexCoord[] = [
    {x: x + 0.5 * hexWidth, y: y },
    {x: x + hexWidth, y: y + 0.25 * hexHeight},
    {x: x + hexWidth, y: y + 0.75 * hexHeight},
    {x: x + 0.5 * hexWidth, y: y + hexHeight},
    {x: x, y: y + 0.75 * hexHeight},
    {x: x, y: y + 0.25 * hexHeight}
  ];
  potentialVertices.forEach(v => {
    if (!uniqueVertices.some(existing => isClose(existing, v, epsilon))) {
      uniqueVertices.push(v);
    }
  });
});
const vertices = uniqueVertices;

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

function getEdgeKey(v1: VertexCoord, v2: VertexCoord): string {
  if (v1.x < v2.x || (v1.x === v2.x && v1.y <= v2.y)) {
    return `${v1.x.toFixed(2)}-${v1.y.toFixed(2)}_${v2.x.toFixed(2)}-${v2.y.toFixed(2)}`;
  } else {
    return `${v2.x.toFixed(2)}-${v2.y.toFixed(2)}_${v1.x.toFixed(2)}-${v1.y.toFixed(2)}`;
  }
}

const edges: Edge[] = [];

hexes.forEach(({ x, y }) => {
  const verts: VertexCoord[] = [
    { x: x + 0.5 * hexWidth, y: y },
    { x: x + hexWidth, y: y + 0.25 * hexHeight },
    { x: x + hexWidth, y: y + 0.75 * hexHeight },
    { x: x + 0.5 * hexWidth, y: y + hexHeight },
    { x: x, y: y + 0.75 * hexHeight },
    { x: x, y: y + 0.25 * hexHeight }
  ];
  for (let i = 0; i < verts.length; i++) {
    const v1 = verts[i];
    const v2 = verts[(i + 1) % verts.length];
    const newEdge = { x1: v1.x, y1: v1.y, x2: v2.x, y2: v2.y };
    if (!edges.some(existingEdge => isEdgeClose(existingEdge, newEdge, epsilon))) {
      edges.push(newEdge);
    }
  }
});

const portMapping: { [vertexLabel: string]: string } = {
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

type Settlement = {
  id: number;
  player: number;
  vertex: string;
};

type Road = {
  id: number;
  player: number;
  edge: string;
}

export default function Home() {
  const [serverMessage, setServerMessage] = useState<string>('');
  const [settlements, setSettlements] = useState<Settlement[]>([]);
  const [roads, setRoads] = useState<Road[]>([]);

  useEffect(() => {
    async function loadSettlements() {
      try {
        const response = await fetch("http://localhost:5000/settlements");
        if (!response.ok) {
          throw new Error(`Server error: ${response.status}`)
        }
        const data = await response.json();
        setSettlements(data.settlements);
      } catch (error: any) {
        console.error("Error fetching settlements:", error.message);
      }
    }
    loadSettlements();
  }, []);

  useEffect(() => {
    async function loadRoads() {
      try {
        const response = await fetch("http://localhost:5000/roads");
        if (!response.ok) {
          throw new Error(`Server error: ${response.status}`);
        }
        const data = await response.json();
        setRoads(data.roads);
      } catch (error: any) {
        console.error("Error fetching roads:", error.message);
      }
    }
    loadRoads();
  }, []);

  async function handleNext() {
    try {
      const response = await fetch("http://localhost:5000/next", {
        method: "POST",
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({})
      });
      if (!response.ok) {
        throw new Error(`Server error: ${response.status}`);
      }
      const data = await response.json();
      if (data.settlement) {
        setSettlements(prev => [...prev, data.settlement]);
      } else if (data.road) {
        setRoads(prev => [...prev, data.road]);
      }
      setServerMessage(data.message);
    } catch (error: any) {
      setServerMessage(`Error: ${error.message}`);
    }
  }

  return (
    <div>
      <div className="outer-container">
        <Ocean />
        <div className="board-container">
          <div className="board">
            {hexes.map(({ id, x, y }) => (
              <HexTile
                key={id}
                id={id}
                color={idToColor[id]}
                token={tokenMapping[id]}
                style={{ left: `${x}vmin`, top: `${y}vmin` }}
              />
            ))}
            <svg className="edge-layer" viewBox="0 0 100 100" preserveAspectRatio="none">
              {edges.map((edge, index) => {
                const midX = (edge.x1 + edge.x2) / 2;
                const midY = (edge.y1 + edge.y2) / 2;
                const label = `E${(index + 1).toString().padStart(2, '0')}`;
                return (
                  <g key={index}>
                    <line
                      x1={edge.x1}
                      y1={edge.y1}
                      x2={edge.x2}
                      y2={edge.y2}
                      stroke="blue"
                      strokeWidth="0.5"
                    />
                    <text
                      x={midX}
                      y={midY}
                      className="edge-label"
                      textAnchor="middle"
                      alignmentBaseline="middle"
                    >
                      {label}
                    </text>
                  </g>
                );
              })}
            </svg>
            <svg className = "road-layer" viewBox = "0 0 100 100" preserveAspectRatio = "none">
              {roads.map((road, index) => {
                const parts = road.edge.split('_');
                const [x1, y1] = parts[0].split('-').map(Number);
                const [x2, y2] = parts[1].split('-').map(Number);
                const colorMapping: { [key: number]: string } = { 1: 'red', 2: 'orange', 3: 'green' };
                const strokeColor = colorMapping[road.player] || 'gray';
                return (
                  <line
                    key = {index}
                    x1 = {x1}
                    y1 = {y1}
                    x2 = {x2}
                    y2 = {y2}
                    stroke = {strokeColor}
                    strokeWidth = "1"
                  />
                );
              })}
            </svg>
            {vertices.map((v, i) => {
              const vLabel = `V${(i + 1).toString().padStart(2, '0')}`;
              return (
                <React.Fragment key={i}>
                  <Vertex x={v.x} y={v.y} label={vLabel} />
                  {portMapping[vLabel] && (
                    <Port x={v.x} y={v.y} type={portMapping[vLabel]} />
                  )}
                </React.Fragment>
              );
            })}
            {settlements.map(s => {
              const vertexIndex = parseInt(s.vertex.substring(1)) - 1;
              const v = vertices[vertexIndex];
              if (!v) return null;
              return (
                <SettlementMarker key={s.id} x={v.x} y={v.y} player={s.player} />
              );
            })}
          </div>
        </div>
      </div>
      <div style={{ textAlign: 'center', marginTop: '1rem' }}>
        <button onClick={handleNext}>Next</button>
        {serverMessage && <p>{serverMessage}</p>}
      </div>
    </div>
  );
}

function Ocean() {
  const oceanWidth = 90; // vmin
  const oceanHeight = oceanWidth * 1.1547;
  return (
    <div
      className="ocean"
      style={{
        left: '50%',
        top: '50%',
        width: `${oceanWidth}vmin`,
        height: `${oceanHeight}vmin`,
        transform: 'translate(-50%, -50%) rotate(30deg)'
      }}
    />
  );
}

function HexTile({
  id,
  token,
  color,
  style
}: {
  id: HexID;
  token: number | null;
  color: string;
  style?: React.CSSProperties;
}) {
  const getTokenFontSize = (token: number): string => {
    const baseFontSize = 4;
    const mapping: { [key: number]: number } = {
      2: 3/11,
      3: 5/11,
      4: 7/11,
      5: 9/11,
      6: 1,
      8: 1,
      9: 9/11,
      10: 7/11,
      11: 5/11,
      12: 3/11
    };
    return `${baseFontSize * (mapping[token] || 1)}vmin`;
  };

  const tokenDotMapping: { [key: number]: number } = {
    2: 1,
    3: 2,
    4: 3,
    5: 4,
    6: 5,
    8: 5,
    9: 4,
    10: 3,
    11: 2,
    12: 1
  };

  return (
    <div className="hex-tile" style={{ ...style, backgroundColor: color }}>
      {token !== null && (
        <div
          className="hex-token"
          style={{
            fontSize: getTokenFontSize(token),
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center'
          }}>
            <div
              className="token-number"
            >
              {token}
            </div>
            <div
              className="token-dots"
            >
              {Array.from({ length: tokenDotMapping[token] || 0 }, (_, i) => (
                <span key = {i} className = "dot" />
              ))}
            </div>
          </div>
        )}
    </div>
  );
}

function Vertex({ x, y, label }: { x: number; y: number; label: string }) {
  return (
    <div className="vertex" style={{ left: `${x}vmin`, top: `${y}vmin` }}>
      <span className="vertex-label">{label}</span>
    </div>
  );
}

function Port({ x, y, type }: { x: number; y: number; type: string }) {
  return (
    <div className="port" style={{ left: `${x}vmin`, top: `${y}vmin` }}>
      <span className="port-label">{type}</span>
    </div>
  );
}

function SettlementMarker({ x, y, player }: { x: number; y: number; player: number }) {
  const colorMapping: { [key: number]: string } = {
    1: 'red',
    2: 'orange',
    3: 'green'
  };
  return (
    <div
      className="settlement"
      style={{
        left: `${x}vmin`,
        top: `${y}vmin`,
        backgroundColor: colorMapping[player] || 'gray'
      }}
    />
  );
}