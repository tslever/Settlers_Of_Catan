import React from 'react';

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

type HexPosition = { id: HexID; x: number; y: number };
const hexes: HexPosition[] = [];

board.forEach((row, rowIndex) => {
  const n = row.length;
  let baseX = (100 - n * hexWidth) / 2;
  const y = rowIndex * verticalSpacing;
  row.forEach((id, colIndex) => {
    const x = baseX + colIndex * hexWidth;
    hexes.push({ id, x, y });
  });
});

type VertexCoord = { x: number; y: number };
const vertexSet = new Map<string, VertexCoord>();

hexes.forEach(({ x, y }) => {
  const vertices: VertexCoord[] = [
    { x: x + 0.5 * hexWidth, y: y },
    { x: x + hexWidth,     y: y + 0.25 * hexHeight },
    { x: x + hexWidth,     y: y + 0.75 * hexHeight },
    { x: x + 0.5 * hexWidth, y: y + hexHeight },
    { x: x,                y: y + 0.75 * hexHeight },
    { x: x,                y: y + 0.25 * hexHeight }
  ];
  vertices.forEach(v => {
    const key = `${v.x.toFixed(2)}-${v.y.toFixed(2)}`;
    vertexSet.set(key, v);
  });
});
const vertices = Array.from(vertexSet.values());
console.log(vertices.length)

export default function Home() {
  return (
    <div className="board-container">
      <div className="board">
        {hexes.map(({ id, x, y }) => (
          <HexTile
            key={id}
            id={id}
            color={idToColor[id]}
            style={{ left: `${x}vmin`, top: `${y}vmin` }}
          />
        ))}
        {vertices.map((v, i) => (
          <Vertex
            key={i}
            x={v.x}
            y={v.y}
            label={`V${(i + 1).toString().padStart(2, '0')}`}
          />
        ))}
      </div>
    </div>
  );
}

function HexTile({
  id,
  color,
  style,
}: {
  id: HexID;
  color: string;
  style?: React.CSSProperties;
}) {
  return (
    <div className="hex-tile" style={{ ...style, backgroundColor: color }}>
      {id}
    </div>
  );
}

function Vertex({ x, y, label }: { x: number; y: number; label: string }) {
  return (
    <div
      className="vertex"
      style={{ left: `${x}vmin`, top: `${y}vmin` }}
    >
      <span className="vertex-label">{label}</span>
    </div>
  );
}