import React from 'react';

export type ResourcesByKind = Record<
  | 'brick' | 'grain' | 'lumber' | 'ore'
  | 'wool'  | 'cloth' | 'coin'   | 'paper',
  number
>;

export type Totals = Record<string, ResourcesByKind>;

interface Props {
  totals: Totals;
  gained: Totals;
}

export const ZERO_BAG: ResourcesByKind = {
  brick: 0, grain: 0, lumber: 0, ore: 0,
  wool: 0, cloth: 0, coin: 0, paper: 0
};

const resourceColours: Record<keyof ResourcesByKind,string> = {
  brick: '#aa4a44', grain: '#fadb5e', lumber: '#228b22', ore: 'gray',
  wool: '#79d021', cloth:'#e682b4', coin:'gold', paper:'wheat'
};

const ORDER: (keyof ResourcesByKind)[] = [
  'brick', 'grain', 'lumber', 'ore', 'wool', 'cloth', 'coin', 'paper'
];

const BOX: React.CSSProperties = { boxSizing: "border-box" };

const PADDING = 1; // vmin

const STAT_H = 3;      // vmin – height of stat boxes
const STAT_PAD = 0.75; // vmin - inner padding around each stats box
const FS      = 2.3;    // baseline font‑size in vmin

const baseCell:React.CSSProperties = {
  display     :'flex',
  alignItems  :'center',
  justifyContent:'center',
  fontSize    : `calc(${FS}vmin)`,
  ...BOX
};

const rectStyle = (r: keyof ResourcesByKind, active: boolean) : React.CSSProperties => ({
  ...baseCell,
  width        : '100%',
  aspectRatio  : '2 / 3', // width : height  = 1 : 1.5
  background   : active ? resourceColours[r] : '#bdbdbd',
  borderRadius : '0.4vmin',
  color        : '#fff',
  fontWeight   : 600,
  padding      : `${PADDING * 0.25}vmin`
});

const statBox: React.CSSProperties = {
  display           : 'grid',
  gridTemplateColumns: '1fr 1fr',
  columnGap         : `${PADDING}vmin`,
  minHeight         : `${STAT_H}vmin`,
  border            : '0.15vmin solid #ddd',
  ...baseCell,
  padding           : `${STAT_PAD}vmin`
};

function StatCell({ total, gain, active }: { total: number; gain: number; active: boolean }) {
  return (
    <div style={statBox}>
      <span style={{ fontWeight: 700, opacity: active ? 1 : 0.4 }}>{total}</span>
      <span style={{
        textAlign: 'right',
        color: 'inherit',
        opacity: active ? 1 : 0.4
      }}>
        {gain >= 0 ? `+${gain}` : gain}
      </span>
    </div>
  );
}

function PlayerColumn({ label, totals, gained, active } : { label: string; totals: ResourcesByKind; gained: ResourcesByKind; active: boolean; }) {
  return (
    <div
      style={{
        display         : 'grid',
        gridTemplateColumns: 'repeat(2, minmax(0, 1fr))',
        rowGap          : `${PADDING}vmin`,
        columnGap       : `${PADDING}vmin`,
        padding         : `${PADDING}vmin`,
        border          : '0.2vmin solid #ccc',
        background      : active ? 'transparent' : '#efefef',
        opacity         : active ? 1 : 0.35,
        filter          : active ? 'none' : 'grayscale(100%)',
        color           : 'inherit',
        overflow        : 'hidden',
        ...BOX
      }}
    >
      <div style={{
        gridColumn: '1 / span 2',
        textAlign : 'center',
        fontWeight: 700,
        fontSize  : `calc(${FS * 1.2}vmin)`
      }}>
        {label}
      </div>

      {ORDER.reduce<React.ReactNode[]>((nodes, resA, idx) => {
        if (idx % 2) { return nodes; }
        const resB = ORDER[idx + 1];
        nodes.push(
          <div key={`${label}-${resA}-r`} style={rectStyle(resA, active)} title={resA}/> ,
          <div key={`${label}-${resB}-r`} style={rectStyle(resB, active)} title={resB}/>
        );
        nodes.push(
          <StatCell key={`${label}-${resA}-s`} total={totals[resA]} gain={gained[resA]} active={active}/> ,
          <StatCell key={`${label}-${resB}-s`} total={totals[resB]} gain={gained[resB]} active={active}/>
        );
        return nodes;
      }, [])}
    </div>
  );
}

const ResourcesDisplay: React.FC<Props> = ({ totals, gained }) => {
  const players = React.useMemo(() => Object.keys(totals).sort(), [totals]);
  return (
    <div
      style={{
        flex: 1,
        display: 'grid',
        columnGap : `${PADDING * 2}vmin`,
        rowGap    : `${PADDING * 2}vmin`,
        gridTemplateColumns: `repeat(${players.length}, 1fr)`,
        color  : 'inherit',
        alignContent: 'start',
        padding: `${PADDING}vmin`
      }}
    >
      {players.map(label => (
        <PlayerColumn
          key    ={label}
          label  ={label}
          totals ={totals[label] ?? ZERO_BAG}
          gained ={gained[label] ?? ZERO_BAG}
          active ={Boolean(totals[label])}
        />
      ))}
    </div>
  );
};

export default ResourcesDisplay;