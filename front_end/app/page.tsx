import React from 'react';

export default function Home() {

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
  };

  const board = [
    ['H01', 'H02', 'H03'], // Row 1
    ['H04', 'H05', 'H06', 'H07'], // Row 2
    ['H08', 'H09', 'H10', 'H11', 'H12'], // Row 3
    ['H13', 'H14', 'H15', 'H16'], // Row 4
    ['H17', 'H18', 'H19'], // Row 5
  ];

  return (
    <div className="board-container">
      <div className="board">
        {board.map((row, rowIndex) => (
          <div
            key={rowIndex}
            className={"row"}
          >
            {row.map((id) => (
              <HexTile key={id} id={id} color={idToColor[id]} />
            ))}
          </div>
        ))}
      </div>
    </div>
  );
}

function HexTile({ id, color }: { id: string; color: string }) {
  return (
    <div className="hex-tile" style={{ backgroundColor: color }}>
      {id}
    </div>
  );
}