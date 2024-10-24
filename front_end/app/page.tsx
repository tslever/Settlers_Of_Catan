import React from 'react';

export default function Home() {
  return (
    <div className="board-container">
      <div className="board">
        {/* Row 1 */}
        <div className="row">
          <HexTile />
          <HexTile />
          <HexTile />
        </div>
        {/* Row 2 */}
        <div className="row offset">
          <HexTile />
          <HexTile />
          <HexTile />
          <HexTile />
        </div>
        {/* Row 3 */}
        <div className="row">
          <HexTile />
          <HexTile />
          <HexTile />
          <HexTile />
          <HexTile />
        </div>
        {/* Row 4 */}
        <div className="row offset">
          <HexTile />
          <HexTile />
          <HexTile />
          <HexTile />
        </div>
        {/* Row 5 */}
        <div className="row">
          <HexTile />
          <HexTile />
          <HexTile />
        </div>
      </div>
    </div>
  );
}

function HexTile() {
  return <div className="hex-tile"></div>;
}