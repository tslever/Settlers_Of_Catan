import React from 'react'

/* ─────── types ─────────────────────────────────────────────────────────── */

export type ResourcesByKind = Record<
  'brick' |
  'grain' |
  'lumber' |
  'ore' |
  'wool' |
  'cloth' |
  'coin' |
  'paper',
  number
>
export type Totals = Record<string, ResourcesByKind>

interface Props {
  totals : Totals
  gained : Totals
}

/* ─────── constants ─────────────────────────────────────────────────────── */

export const ZERO_BAG: ResourcesByKind = {
  brick: 0,
  grain: 0,
  lumber: 0,
  ore: 0,
  wool: 0,
  cloth: 0,
  coin: 0,
  paper: 0
}

const COLORS: Record<keyof ResourcesByKind, string> = {
  brick: "rgb(170, 74, 68)",
  grain: "rgb(250, 219, 94)",
  lumber: "rgb(34, 139, 34)",
  ore: "rgb(128, 128, 128)",
  wool: "rgb(121, 208, 33)",
  cloth: "rgb(250, 219, 94)",
  coin: "rgb(128, 128, 128)",
  paper: "rgb(34, 139, 34)"
};

// order of the eight real resources
const ORDER:(keyof ResourcesByKind)[] = [
  'brick',
  'grain',
  'lumber',
  'ore',
  'wool',
  'cloth',
  'coin',
  'paper'
];

// number of additional empty slots to pad to the right
const PLACEHOLDER_COUNT = 5;

/* –––––– tuned sizes so **13** cards fit in 100 vmin ––––––––––––––––––––– */

const PAD = 0.65; // gap between cards / rows
const FS = 1.9; // base font-size (vmin)
const STAT_H = 2.2; // stat-box height  (vmin)
const STAT_PAD = 0.45;

const MAX_PER_ROW  = 13; // 8 resources + 5 placeholders
/* card width = (100% – gaps) ÷ 13, with a practical min / max guard */
const CARD_W = `clamp(4vmin, calc((100% - ${PAD*(MAX_PER_ROW-1)}vmin) / ${MAX_PER_ROW}), 8vmin)`

/* ─────── tiny building blocks ─────────────────────────────────────────── */

const baseCell: React.CSSProperties = {
  boxSizing: 'border-box',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  fontSize: `calc(${FS}vmin)`
};

const makeRectCss = (bg: string, dim: boolean = false): React.CSSProperties => ({
  ...baseCell,
  width: '100%',
  aspectRatio: '2/3',
  background: bg,
  borderRadius: '0.25vmin',
  color: '#fff',
  fontWeight: 600,
  opacity: dim ? 0.35 : 1
});

const statBoxCss = (dim: boolean): React.CSSProperties => ({
  ...baseCell,
  display: 'grid',
  gridTemplateColumns: '1fr 1fr',
  columnGap: `${PAD}vmin`,
  minHeight: `${STAT_H}vmin`,
  border: '0.15vmin solid #ddd',
  padding: `${STAT_PAD}vmin`
})

const gainBg = (g: number): string => {
  return g === 0 ? "#ffffff" : g > 0 ? "#c8e6c9" : "#ffcdd2";
}

function StatCell({total, gain, dim}: {total: number; gain: number; dim: boolean}) {
  const mutedText = dim ? "#7a7a7a" : undefined;
  return (
    <div style = {statBoxCss(dim)}>
      <span style = {{fontWeight: 700, color: mutedText}}>{total}</span>
      <span
        style = {{
          textAlign: 'right',
          backgroundColor: gainBg(gain),
          padding: "0 0.3vmin",
          borderRadius: "0.15vmin",
          color: gain === 0 ? mutedText : "#000"
        }}
      >
        {gain >= 0 ? `+${gain}` : gain}
      </span>
    </div>
  )
}

/* ─────── resource card (rectangle + stats) ─────────────────────────────── */

function ResourceCard({res, total, gain}: {res: keyof ResourcesByKind, total: number, gain: number}) {
  const dim = total === 0
  const rectColor = dim ? '#bdbdbd' : COLORS[res]
  return(
    <div
      style = {{
        display: 'flex',
        flexDirection:'column',
        rowGap: `${PAD * 0.3}vmin`,
        flex: `0 0 ${CARD_W}`,
        width: CARD_W
      }}
    >
      <div style = {makeRectCss(rectColor, dim)} title = {res}/>
      <StatCell total = {total} gain = {gain} dim = {dim}/>
    </div>
  );
}

/* ─────── placeholder card (grey rectangle, empty stats) ───────────────── */

function PlaceholderCard() {
  const dim = true;
  return (
    <div
      style = {{
        display: 'flex',
        flexDirection: 'column',
        rowGap: `${PAD * 0.3}vmin`,
        flex: `0 0 ${CARD_W}`,
        width: CARD_W
      }}
    >
      <div style = {makeRectCss('#bdbdbd', dim)}/>
      <div style = {statBoxCss(dim)}></div>
    </div>
  )
}

/* ─────── per-player panel ─────────────────────────────────────────────── */

function PlayerPanel({label, totals, gained}: {label: string, totals: ResourcesByKind, gained: ResourcesByKind}) {
  return (
    <div
      style = {{
        border: '0.2vmin solid #ccc',
        padding: `${PAD}vmin`,
        display: 'flex',
        flexDirection: 'column',
        rowGap:`${PAD}vmin`
      }}
    >
      {/* Player label – never greyed out */}
      <div
        style = {{
          textAlign: 'center',
          fontWeight: 700,
          fontSize: `calc(${FS*1.05}vmin)`
        }}
      >
        {label}
      </div>

      <div
        style = {{
          display: 'flex',
          flexWrap: 'nowrap',
          columnGap: `${PAD}vmin`,
          overflow: 'hidden'
        }}
      >
        {ORDER.map(res => (
          <ResourceCard
            key = {`${label}-${res}`}
            res = {res}
            total = {totals[res]}
            gain = {gained[res]}
          />
        ))}
        {Array.from({ length: PLACEHOLDER_COUNT }, (_, i) => (
          <PlaceholderCard key = {`ph-${label}-${i}`}/>
        ))}
      </div>
    </div>
  );
}

/* ─────── main export ───────────────────────────────────────────────────── */

const ResourcesDisplay: React.FC<Props> = ({ totals,gained }) => {

  const players = React.useMemo(
    () => Object.keys(totals).sort(
      (a, b) => parseInt(a.replace(/\D/g, '')) - parseInt(b.replace(/\D/g, ''))
    ),
    [totals]
  );

  return(
    <div
      style = {{
        display: 'flex',
        flexDirection: 'column',
        rowGap: `${PAD}vmin`,
        padding: `${PAD}vmin`
      }}
    >
      {players.map(label => (
        <PlayerPanel
          key = {label}
          label = {label}
          totals = {totals[label] ?? ZERO_BAG}
          gained = {gained[label] ?? ZERO_BAG}
        />
      ))}
    </div>
  );
};

export default ResourcesDisplay;