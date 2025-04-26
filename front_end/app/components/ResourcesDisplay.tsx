import React from 'react';

export type ResourcesByKind = Record<
  | 'brick'
  | 'grain'
  | 'lumber'
  | 'ore'
  | 'wool'
  | 'cloth'
  | 'coin'
  | 'paper',
  number
>;
export type Totals = Record<string, ResourcesByKind>;

interface Props {
  // Aggregated resources for each player
  totals: Totals;
  // Resources gained or lost on the current turn for each player
  gained: Totals;
}

export const ZERO_BAG: ResourcesByKind = {
  brick: 0,
  grain: 0,
  lumber: 0,
  ore: 0,
  wool: 0,
  cloth: 0,
  coin: 0,
  paper: 0,
};

// Color coding for the resource rectangles
const resourceColours: Record<keyof ResourcesByKind, string> = {
  brick: 'rgb(170, 74, 68)',
  grain: 'rgb(250, 219, 94)',
  lumber: 'rgb(34, 139, 34)',
  ore: 'gray',
  wool: 'rgb(121, 208, 33)',
  cloth: 'rgb(230, 130, 180)',
  coin: 'gold',
  paper: 'wheat',
};

// Order we want the resources to appear in a player's column
const ORDER: (keyof ResourcesByKind)[] = [
  'brick',
  'grain',
  'lumber',
  'ore',
  'wool',
  'cloth',
  'coin',
  'paper',
];

// Layout constants
const RECT_COL_WIDTH_REM = 2.9; // grid column width (rem)
const RECT_HEIGHT_REM = RECT_COL_WIDTH_REM * 1.5;
const RECT_WIDTH = `${RECT_COL_WIDTH_REM}rem`;
const RECT_HEIGHT = `${RECT_HEIGHT_REM}rem`;

const baseCell: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  fontSize: '0.8rem',
};

/* Rectangle fills its grid area.
* Cell and rectangle are identical in size.
*/
const rectangleStyle = (
  resource: keyof ResourcesByKind,
): React.CSSProperties => ({
  ...baseCell,
  width: '100%', // fill cell horizontally (cell width is RECT_WIDTH)
  height: '100%', // fill cell vertically (cell height is RECT_HEIGHT)
  aspectRatio: `${RECT_COL_WIDTH_REM} / ${RECT_HEIGHT_REM}`,
  background: resourceColours[resource],
  borderRadius: '0.25rem',
  color: '#fff',
  fontWeight: 600,
});

const statWrapper: React.CSSProperties = {
  display: 'grid',
  gridTemplateColumns: '1fr 1fr',
  columnGap: '0.5rem', // separation between total and gain
  border: '1px solid #ddd',
  height: '2rem',
  ...baseCell,
};

function StatCell({ total, gain }: { total: number; gain: number }) {
  return (
    <div style={statWrapper}>
      <span style={{ fontWeight: 700 }}>{total}</span>
      <span
        style={{
          color: gain > 0 ? 'green' : gain < 0 ? 'red' : '#555',
          textAlign: 'right',
        }}
      >
        {gain >= 0 ? `+${gain}` : gain}
      </span>
    </div>
  );
}

/** Render one player column – a grid of 8 pairs (rectangle row + stats row).
 * Grid rows are sized so each rectangle cell equals rectangle size.
 */
function PlayerColumn({
  label,
  totals,
  gained,
}: {
  label: string;
  totals: ResourcesByKind;
  gained: ResourcesByKind;
}) {
  return (
    <div
      style={{
        display: 'grid',
        gridTemplateColumns: `repeat(2, ${RECT_WIDTH})`,
        // 4 resource pairs in 8 rows (rectangle + stats)
        gridTemplateRows: `auto repeat(4, ${RECT_HEIGHT} 2rem)`, // auto for header
        gap: '0.5rem',
        border: '1px solid #ccc',
        padding: '0.5rem',
      }}
    >
      {/* Header spanning both columns */}
      <div
        style={{
          gridColumn: '1 / span 2',
          textAlign: 'center',
          fontWeight: 700,
        }}
      >
        {label}
      </div>

      {ORDER.reduce<React.ReactNode[]>((acc, resA, idx) => {
        if (idx % 2 === 1) return acc;
        const resB = ORDER[idx + 1];

        // Rectangle row (A & B)
        acc.push(
          <div key={`${label}-${resA}-rect`} style={rectangleStyle(resA)} title={resA} />, // A
        );
        acc.push(
          <div key={`${label}-${resB}-rect`} style={rectangleStyle(resB)} title={resB} />, // B
        );

        // Stats row (A & B)
        acc.push(
          <StatCell key={`${label}-${resA}-stats`} total={totals[resA]} gain={gained[resA]} />,
        );
        acc.push(
          <StatCell key={`${label}-${resB}-stats`} total={totals[resB]} gain={gained[resB]} />,
        );

        return acc;
      }, [])}
    </div>
  );
}

// High‑level component arranging `<PlayerColumn />` components horizontally
const ResourcesDisplay: React.FC<Props> = ({ totals, gained }) => {
  const players = Object.keys(totals);
  return (
    <div
      style={{
        display: 'grid',
        gap: '1rem',
        gridTemplateColumns: `repeat(${players.length}, auto)`,
      }}
    >
      {players.map((label) => (
        <PlayerColumn
          key={label}
          label={label}
          totals={totals[label] ?? ZERO_BAG}
          gained={gained[label] ?? ZERO_BAG}
        />
      ))}
    </div>
  );
};

export default ResourcesDisplay;