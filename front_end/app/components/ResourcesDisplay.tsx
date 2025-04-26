import React from 'react';

export type ResourcesByKind = Record<
  | 'brick' | 'grain' | 'lumber' | 'ore'
  | 'wool'  | 'cloth' | 'coin'   | 'paper',
  number
>;

export type Totals = Record<string, ResourcesByKind>;

interface Props {
  totals : Totals;
  gained : Totals;
}

export const ZERO_BAG: ResourcesByKind = {
  brick: 0, grain: 0, lumber: 0, ore: 0,
  wool: 0,  cloth: 0, coin: 0,  paper: 0
};

const resourceColours: Record<keyof ResourcesByKind, string> = {
  brick: '#aa4a44', grain : '#fadb5e', lumber: '#228b22', ore  : 'gray',
  wool : '#79d021', cloth : '#e682b4', coin  : 'gold',    paper: 'wheat'
};

const ORDER: (keyof ResourcesByKind)[] = [
  'brick','grain','lumber','ore','wool','cloth','coin','paper'
];

const BOX          : React.CSSProperties = { boxSizing: 'border-box' };
const PADDING      = 1;        // vmin
const STAT_HEIGHT  = 3;        // vmin
const STAT_PAD     = 0.75;     // vmin
const FS           = 2.3;      // baseline font-size (vmin)
const COLS         = 4;        // rectangles per row

/* ── primitive building blocks ────────────────────────────────────────────── */

const baseCell: React.CSSProperties = {
  display:'flex', alignItems:'center', justifyContent:'center',
  fontSize:`calc(${FS}vmin)`, ...BOX
};

const rectStyle = (r: keyof ResourcesByKind, active: boolean): React.CSSProperties => ({
  ...baseCell,
  width:'100%', aspectRatio:'2 / 3',
  background: active ? resourceColours[r] : '#bdbdbd',
  borderRadius:'0.4vmin', color:'#fff', fontWeight:600
});

const statBox: React.CSSProperties = {
  display:'grid', gridTemplateColumns:'1fr 1fr', columnGap:`${PADDING}vmin`,
  minHeight:`${STAT_HEIGHT}vmin`, border:'0.15vmin solid #ddd',
  padding:`${STAT_PAD}vmin`, ...baseCell
};

function StatCell({ total, gain, active }:
  { total:number; gain:number; active:boolean }) {
  return (
    <div style={statBox}>
      <span style={{ fontWeight:700, opacity:active?1:0.4 }}>{total}</span>
      <span style={{
        textAlign:'right', opacity:active?1:0.4
      }}>
        {gain >= 0 ? `+${gain}` : gain}
      </span>
    </div>
  );
}

/* ── resource “card”: rectangle over stat ─────────────────────────────────── */

function ResourceCard(
  { res, total, gain, active }:
  { res:keyof ResourcesByKind; total:number; gain:number; active:boolean }
) {
  return (
    <div style={{ display:'flex', flexDirection:'column', rowGap:`${PADDING*0.25}vmin` }}>
      <div style={rectStyle(res, active)} title={res}/>
      <StatCell total={total} gain={gain} active={active}/>
    </div>
  );
}

/* ── per-player column ────────────────────────────────────────────────────── */

function PlayerColumn(
  { label, totals, gained, active }:
  { label:string; totals:ResourcesByKind; gained:ResourcesByKind; active:boolean }
) {
  return (
    <div
      style={{
        display:'grid',
        gridTemplateColumns:`repeat(${COLS}, minmax(0, 1fr))`,
        columnGap:`${PADDING}vmin`, rowGap:`${PADDING}vmin`,
        padding:`${PADDING}vmin`,
        border:'0.2vmin solid #ccc',
        background: active ? 'transparent' : '#efefef',
        opacity: active ? 1 : 0.35,
        filter: active ? 'none' : 'grayscale(100%)',
        ...BOX
      }}
    >
      {/* player label spans full width */}
      <div style={{
        gridColumn:`1 / span ${COLS}`,
        textAlign:'center', fontWeight:700,
        fontSize:`calc(${FS*1.2}vmin)`
      }}>
        {label}
      </div>

      {ORDER.map(res => (
        <ResourceCard
          key={`${label}-${res}`}
          res={res}
          total={totals[res]}
          gain={gained[res]}
          active={active}
        />
      ))}
    </div>
  );
}

/* ── main export ──────────────────────────────────────────────────────────── */

const ResourcesDisplay: React.FC<Props> = ({ totals, gained }) => {
  const players = React.useMemo(() => Object.keys(totals).sort(), [totals]);

  return (
    <div
      style={{
        flex:1,
        display:'grid',
        gridTemplateColumns:`repeat(${players.length}, 1fr)`,
        columnGap:`${PADDING*2}vmin`, rowGap:`${PADDING*2}vmin`,
        alignContent:'start', padding:`${PADDING}vmin`
      }}
    >
      {players.map(label => (
        <PlayerColumn
          key={label}
          label={label}
          totals={totals[label] ?? ZERO_BAG}
          gained={gained[label] ?? ZERO_BAG}
          active={Boolean(totals[label])}
        />
      ))}
    </div>
  );
};

export default ResourcesDisplay;