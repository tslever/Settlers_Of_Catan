import React from 'react'

/* ─────── types ─────────────────────────────────────────────────────────── */

export type ResourcesByKind = Record<
  'brick' | 'grain' | 'lumber' | 'ore' |
  'wool'  | 'cloth' | 'coin'   | 'paper',
  number
>
export type Totals = Record<string, ResourcesByKind>

interface Props {
  totals : Totals
  gained : Totals
}

/* ─────── constants ─────────────────────────────────────────────────────── */

export const ZERO_BAG: ResourcesByKind = {
  brick:0, grain:0, lumber:0, ore:0,
  wool:0, cloth:0, coin:0, paper:0
}

const COLORS: Record<keyof ResourcesByKind,string> = {
  brick:'#aa4a44', grain:'#fadb5e', lumber:'#228b22', ore:'gray',
  wool:'#79d021', cloth:'#e682b4', coin:'gold',  paper:'wheat'
}

// order of the eight real resources
const ORDER:(keyof ResourcesByKind)[] = [
  'brick','grain','lumber','ore','wool','cloth','coin','paper'
]

// number of additional empty slots to pad to the right
const PLACEHOLDER_COUNT = 5

/* –––––– tuned sizes so **13** cards fit in 100 vmin ––––––––––––––––––––– */

const PAD          = 0.65       // gap between cards / rows
const FS           = 1.9        // base font-size (vmin)
const STAT_H       = 2.2        // stat-box height  (vmin)
const STAT_PAD     = 0.45

const MAX_PER_ROW  = 13         // 8 resources + 5 placeholders
/* card width = (100 % – gaps) ÷ 13   with a practical min / max guard     */
const CARD_W       = `clamp(4vmin, calc((100% - ${PAD*(MAX_PER_ROW-1)}vmin) / ${MAX_PER_ROW}), 8vmin)`

/* ─────── tiny building blocks ─────────────────────────────────────────── */

const baseCell:React.CSSProperties = {
  boxSizing:'border-box',
  display:'flex', alignItems:'center', justifyContent:'center',
  fontSize:`calc(${FS}vmin)`
}

const rectCss = (bg:string, active:boolean):React.CSSProperties=>({
  ...baseCell,
  width:'100%',
  aspectRatio:'2/3',
  background:bg,
  borderRadius:'0.25vmin',
  color:'#fff', fontWeight:600,
  opacity:active?1:0.35
})

const statBoxCss:React.CSSProperties = {
  ...baseCell,
  display:'grid', gridTemplateColumns:'1fr 1fr',
  columnGap:`${PAD}vmin`,
  minHeight:`${STAT_H}vmin`,
  border:'0.15vmin solid #ddd',
  padding:`${STAT_PAD}vmin`
}

function StatCell({total,gain,active}:{total:number;gain:number;active:boolean}) {
  return(
    <div style={statBoxCss}>
      <span style={{fontWeight:700,opacity:active?1:0.4}}>{total}</span>
      <span style={{textAlign:'right',opacity:active?1:0.4}}>
        {gain>=0?`+${gain}`:gain}
      </span>
    </div>
  )
}

/* ─────── resource card (rectangle + stats) ─────────────────────────────── */

function ResourceCard(
  {res,total,gain,active}:{
    res:keyof ResourcesByKind,total:number,gain:number,active:boolean
  }
){
  return(
    <div style={{
      display:'flex', flexDirection:'column',
      rowGap:`${PAD*0.3}vmin`,
      flex:`0 0 ${CARD_W}`,
      width:CARD_W
    }}>
      <div style={rectCss(COLORS[res],active)} title={res}/>
      <StatCell total={total} gain={gain} active={active}/>
    </div>
  )
}

/* ─────── placeholder card (grey rectangle, empty stats) ───────────────── */

function PlaceholderCard({active}:{active:boolean}){
  return(
    <div style={{
      display:'flex', flexDirection:'column',
      rowGap:`${PAD*0.3}vmin`,
      flex:`0 0 ${CARD_W}`,
      width:CARD_W
    }}>
      <div style={rectCss('#bdbdbd',active)}/>
      <div style={{...statBoxCss,opacity:active?1:0.4}}></div>
    </div>
  )
}

/* ─────── per-player panel ─────────────────────────────────────────────── */

function PlayerPanel(
  {label,totals,gained,active}:{
    label:string, totals:ResourcesByKind, gained:ResourcesByKind, active:boolean
  }
){
  return(
    <div style={{
      border:'0.2vmin solid #ccc',
      padding:`${PAD}vmin`,
      display:'flex', flexDirection:'column', rowGap:`${PAD}vmin`,
      background:active?'transparent':'#efefef',
      opacity:active?1:0.35, filter:active?'none':'grayscale(100%)'
    }}>
      <div style={{textAlign:'center',fontWeight:700,fontSize:`calc(${FS*1.05}vmin)`}}>
        {label}
      </div>

      {/* fixed-width row – exactly MAX_PER_ROW cards fit */}
      <div style={{
        display:'flex',
        flexWrap:'nowrap',
        columnGap:`${PAD}vmin`,
        overflow:'hidden'
      }}>
        {ORDER.map(res=>(
          <ResourceCard
            key={`${label}-${res}`}
            res={res}
            total={totals[res]}
            gain={gained[res]}
            active={active}
          />
        ))}
        {Array.from({length:PLACEHOLDER_COUNT},(_,i)=>(
          <PlaceholderCard key={`ph-${label}-${i}`} active={active}/>
        ))}
      </div>
    </div>
  )
}

/* ─────── main export ───────────────────────────────────────────────────── */

const ResourcesDisplay:React.FC<Props> = ({ totals,gained }) => {

  /* numeric sort → Player 1, Player 2, … */
  const players = React.useMemo(
    ()=>Object.keys(totals)
          .sort((a,b)=>parseInt(a.replace(/\D/g,''))
                       -parseInt(b.replace(/\D/g,''))),
    [totals]
  )

  return(
    <div style={{
      display:'flex',
      flexDirection:'column',
      rowGap:`${PAD}vmin`,
      padding:`${PAD}vmin`
    }}>
      {players.map(label=>(
        <PlayerPanel
          key   ={label}
          label ={label}
          totals={totals[label] ?? ZERO_BAG}
          gained={gained[label] ?? ZERO_BAG}
          active={Boolean(totals[label])}
        />
      ))}
    </div>
  )
}

export default ResourcesDisplay
