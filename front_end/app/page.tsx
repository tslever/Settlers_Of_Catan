import React from 'react';

export default function Home() {
  const board = [
    ["H01", "H02", "H03"], // Row 1
    ["H04", "H05", "H06", "H07"], // Row 2
    ["H08", "H09", "H10", "H11", "H12"], // Row 3
    ["H13", "H14", "H15", "H16"], // Row 4
    ["H17", "H18", "H19"], // Row 5
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
              <HexTile key={id} id={id} />
            ))}
          </div>
        ))}
      </div>
    </div>
  );
}

function HexTile({ id }: { id: string }) {
  return <div className="hex-tile">{id}</div>;
}