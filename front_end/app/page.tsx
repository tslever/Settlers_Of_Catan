import React from 'react';

export default function Home() {
  const board = [
    [1, 2, 3],          // Row 1
    [4, 5, 6, 7],       // Row 2 (offset)
    [8, 9, 10, 11, 12], // Row 3
    [13, 14, 15, 16],   // Row 4 (offset)
    [17, 18, 19],       // Row 5
  ];

  return (
    <div className="board-container">
      <div className="board">
        {board.map((row, rowIndex) => (
          <div
            key={rowIndex}
            className={`row ${rowIndex % 2 === 1 ? 'offset' : ''}`}
          >
            {row.map((tileNumber) => (
              <HexTile key={tileNumber} number={tileNumber} />
            ))}
          </div>
        ))}
      </div>
    </div>
  );
}

function HexTile({ number }: { number: number }) {
  return <div className="hex-tile">{number}</div>;
}