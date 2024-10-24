import React from 'react';

export default function Home() {
  const board = [
    ["H1", "H2", "H3"], // Row 1
    ["H4", "H5", "H6", "H7"], // Row 2
    ["H8", "H9", "H10", "H11", "H12"], // Row 3
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