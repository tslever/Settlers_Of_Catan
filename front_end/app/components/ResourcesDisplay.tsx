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
const MAX_PLAYERS = 6;

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
}

const ROW1: (keyof ResourcesByKind)[] = [
  'brick',
  'grain',
  'lumber',
  'ore',
  'wool'
]

const ROW2: (keyof ResourcesByKind)[] = [
  'cloth',
  'coin',
  'paper'
]

// number of greyed‑out placeholders in third row
const PLACEHOLDER_COUNT = 5

/* –––––– card layout constants ––––––––––––––––––––– */
const PAD = 0.65              // gap between cards / rows
const FS = 1.9                // base font‑size (vmin)
const STAT_H = 2.2            // stat‑box height  (vmin)
const STAT_PAD = 0.45

/* ─────── tiny building blocks ─────────────────────────────────────────── */
const baseCell: React.CSSProperties = {
  boxSizing: 'border-box',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  fontSize: `calc(${FS}vmin)`
}

const makeRectCss = (bg: string, dim: boolean = false): React.CSSProperties => ({
  ...baseCell,
  width: '100%',
  aspectRatio: '2/3',
  background: bg,
  borderRadius: '0.25vmin',
  color: '#fff',
  fontWeight: 600,
  opacity: dim ? 0.35 : 1
})

const statBoxCss = (dim: boolean): React.CSSProperties => ({
  ...baseCell,
  display: 'grid',
  gridTemplateColumns: '1fr 1fr',
  columnGap: `${PAD}vmin`,
  minHeight: `${STAT_H}vmin`,
  border: '0.15vmin solid #ddd',
  padding: `${STAT_PAD}vmin`
})

const gainBg = (g: number): string => g === 0 ? '#ffffff' : g > 0 ? '#c8e6c9' : '#ffcdd2'

function StatCell({ total, gain, dim }: { total: number; gain: number; dim: boolean }) {
  const muted = dim ? '#7a7a7a' : undefined
  return (
    <div style={statBoxCss(dim)}>
      <span style={{ fontWeight: 700, color: muted }}>{total}</span>
      <span style={{
        textAlign: 'right',
        backgroundColor: gainBg(gain),
        padding: '0 0.3vmin',
        borderRadius: '0.15vmin',
        color: gain === 0 ? muted : '#000'
      }}>
        {gain >= 0 ? `+${gain}` : gain}
      </span>
    </div>
  )
}

/* ─────── resource card (real resource) ────────────────────────────────── */
function ResourceCard({ res, total, gain }: { res: keyof ResourcesByKind; total: number; gain: number }) {
  const dim = total === 0
  return (
    <div style={{ display: 'flex', flexDirection: 'column', rowGap: `${PAD * 0.3}vmin`, width: "100%" }}>
      <div style={makeRectCss(dim ? '#bdbdbd' : COLORS[res], dim)} title={res} />
      <StatCell total={total} gain={gain} dim={dim} />
    </div>
  )
}

/* ─────── grey placeholder card ───────────────────────────────────────── */
function PlaceholderCard() {
  const dim = true
  return (
    <div style={{ display: 'flex', flexDirection: 'column', rowGap: `${PAD * 0.3}vmin`, width: "100%" }}>
      <div style={makeRectCss('#bdbdbd', dim)} />
      <div style={statBoxCss(dim)} />
    </div>
  )
}

/* ─────── helpers for wrapping rows ───────────────────────────────────── */
function Row({ children, cols }: { children: React.ReactNode, cols: number }) {
  return (
    <div style={{
      display: 'grid',
      gridTemplateColumns: `repeat(${cols}, 1fr)`,
      columnGap: `${PAD}vmin`,
      overflow: 'hidden'
    }}>
      {children}
    </div>
  );
}

/* ─────── per‑player panel ─────────────────────────────────────────────── */
function PlayerPanel({ label, totals, gained, dim }: { label: string; totals: ResourcesByKind; gained: ResourcesByKind; dim: boolean }) {
  return (
    <div style={{
      border: '0.2vmin solid #ccc',
      padding: `${PAD}vmin`,
      display: 'flex',
      flexDirection: 'column',
      rowGap: `${PAD}vmin`,
      opacity: dim ? 0.35 : 1,
      width: '100%',
      boxSizing: 'border-box'
    }}>
      {/* label – never greyed out */}
      <div style={{ textAlign: 'center', fontWeight: 700, fontSize: `calc(${FS * 1.1}vmin)`, color: dim ? '#7a7a7a' : undefined }}>
        {label}
      </div>
      {/* 5 real resources */}
      <Row cols = {5}>
        {ROW1.map(res => <ResourceCard key={`${label}-row1-${res}`} res={res} total={totals[res]} gain={gained[res]} />)}
      </Row>
      {/* 3 real resources */}
      <Row cols = {5}>
        {ROW2.map(res => <ResourceCard key={`${label}-row2-${res}`} res={res} total={totals[res]} gain={gained[res]} />)}
        {Array.from({ length: 5 - ROW2.length }).map((_, i) => (
          <div key = {`row2-spacer-${label}-${i}`} style = {{ visibility: 'hidden', width: '100%' }}/>
        ))}
      </Row>
      {/* 5 greyed placeholders */}
      <Row cols = {5}>
        {Array.from({ length: PLACEHOLDER_COUNT }, (_, i) => <PlaceholderCard key={`ph-${label}-${i}`} />)}
      </Row>
    </div>
  )
}

/* ─────── main export ─────────────────────────────────────────────────── */
const ResourcesDisplay: React.FC<Props> = ({ totals, gained }) => {
  const livePlayers = React.useMemo(() => Object.keys(totals).sort((a, b) => parseInt(a.replace(/\D/g, ''), 10) - parseInt(b.replace(/\D/g, ''), 10)), [totals])
  const panelLabels = React.useMemo(() => Array.from({ length: MAX_PLAYERS }, (_, i) => `Player ${i + 1}`), [])

  return (
    <div style={{
      display: 'grid',
      gridTemplateColumns: 'repeat(3, 1fr)',
      gap: `${PAD * 1.5}vmin`,
      padding: `${PAD}vmin`,
      width: '100%',
      overflowX: 'hidden',
      boxSizing: 'border-box'
    }}>
      {panelLabels.map(label => (
        <PlayerPanel key={label} label={label} totals={totals[label] ?? ZERO_BAG} gained={gained[label] ?? ZERO_BAG} dim={!livePlayers.includes(label)} />
      ))}
    </div>
  )
}

export default ResourcesDisplay