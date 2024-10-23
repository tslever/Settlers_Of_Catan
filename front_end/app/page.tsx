export default function Home() {
  return (
    <div className="flex flex-col items-center">
      {/* Row 1 */}
      <div className="flex">
        <HexTile />
        <HexTile />
        <HexTile />
      </div>
      {/* Row 2 */}
      <div className="flex">
        <HexTile />
        <HexTile />
        <HexTile />
        <HexTile />
      </div>
      {/* Row 3 */}
      <div className="flex">
        <HexTile />
        <HexTile />
        <HexTile />
        <HexTile />
        <HexTile />
      </div>
      {/* Row 4 */}
      <div className="flex">
        <HexTile />
        <HexTile />
        <HexTile />
        <HexTile />
      </div>
      {/* Row 5 */}
      <div className="flex">
        <HexTile />
        <HexTile />
        <HexTile />
      </div>
    </div>
  );
}

function HexTile() {
  return (
    <div className="hex-tile w-24 h-24">
      {/* You can add content inside the hexagon here */}
    </div>
  );
}