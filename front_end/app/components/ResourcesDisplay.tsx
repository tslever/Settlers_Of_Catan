import React from 'react';

/* ─────── types ─────────────────────────────────────────────────────────── */

export type ResourcesByKind = Record<
  'brick' | 'grain' | 'lumber' | 'ore' |
  'wool'  | 'cloth' | 'coin'   | 'paper',
  number
>;
export type Totals = Record<string, ResourcesByKind>;

interface Props {
  totals : Totals;
  gained : Totals;
}

/* ─────── constants ─────────────────────────────────────────────────────── */

export const ZERO_BAG: ResourcesByKind = {
  brick:0, grain:0, lumber:0, ore:0,
  wool:0, cloth:0, coin:0, paper:0
};

const COLORS: Record<keyof ResourcesByKind,string> = {
  brick:'#aa4a44', grain:'#fadb5e', lumber:'#228b22', ore:'gray',
  wool:'#79d021', cloth:'#e682b4', coin:'gold', paper:'wheat'
};

const ORDER: (keyof ResourcesByKind)[] = [
  'brick','grain','lumber','ore','wool','cloth','coin','paper'
];

const PAD      = 1;       // vmin
const FS       = 2.3;     // baseline font-size
const STAT_H   = 3;       // vmin
const STAT_PAD = 0.75;    // vmin
const CARD_W   = `calc((100% - ${PAD*7}vmin)/8)`; // 8 cards + 7 gaps

/* ─────── tiny building blocks ──────────────────────────────────────────── */

const baseCell:React.CSSProperties = {
  boxSizing:'border-box',
  display:'flex', alignItems:'center', justifyContent:'center',
  fontSize:`calc(${FS}vmin)`
};

const rectCss = (r:keyof ResourcesByKind, active:boolean):React.CSSProperties=>({
  ...baseCell,
  width:'100%', aspectRatio:'2/3',
  background: active ? COLORS[r] : '#bdbdbd',
  borderRadius:'0.4vmin', color:'#fff', fontWeight:600
});

const statBoxCss:React.CSSProperties = {
  ...baseCell,
  display:'grid', gridTemplateColumns:'1fr 1fr',
  columnGap:`${PAD}vmin`,
  minHeight:`${STAT_H}vmin`,
  border:'0.15vmin solid #ddd',
  padding:`${STAT_PAD}vmin`
};

function StatCell({ total, gain, active }:{total:number;gain:number;active:boolean}) {
  return (
    <div style={statBoxCss}>
      <span style={{fontWeight:700,opacity:active?1:0.4}}>{total}</span>
      <span style={{textAlign:'right',opacity:active?1:0.4}}>
        {gain>=0?`+${gain}`:gain}
      </span>
    </div>
  );
}

/* ─────── resource card (rectangle + stats) ─────────────────────────────── */

function ResourceCard(
  { res, total, gain, active }:{
    res:keyof ResourcesByKind; total:number; gain:number; active:boolean
  }
){
  return (
    <div style={{
      display:'flex', flexDirection:'column',
      rowGap:`${PAD*0.25}vmin`,
      flex:`0 0 ${CARD_W}`,
      width:CARD_W               // makes 8 fit exactly
    }}>
      <div style={rectCss(res,active)} title={res}/>
      <StatCell total={total} gain={gain} active={active}/>
    </div>
  );
}

/* ─────── per-player panel ─────────────────────────────────────────────── */

function PlayerPanel(
  { label, totals, gained, active }:{
    label:string; totals:ResourcesByKind; gained:ResourcesByKind; active:boolean;
  }
){
  return (
    <div style={{
      border:'0.2vmin solid #ccc',
      padding:`${PAD}vmin`,
      display:'flex', flexDirection:'column', rowGap:`${PAD}vmin`,
      background:active?'transparent':'#efefef',
      opacity:active?1:0.35, filter:active?'none':'grayscale(100%)'
    }}>
      {/* player label */}
      <div style={{
        textAlign:'center', fontWeight:700,
        fontSize:`calc(${FS*1.2}vmin)`
      }}>
        {label}
      </div>

      {/* horizontal scroller of resource cards */}
      <div style={{
        display:'flex',
        flexWrap:'nowrap',
        columnGap:`${PAD}vmin`,
        overflowX:'auto',
        paddingBottom:`${PAD}vmin`,
        scrollbarWidth:'thin'          // Firefox
        /* WebKit scrollbars are thin by default in most browsers;
           add custom CSS if you want to style them further. */
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
      </div>
    </div>
  );
}

/* ─────── main export ───────────────────────────────────────────────────── */

const ResourcesDisplay:React.FC<Props> = ({ totals, gained }) => {
  /* numeric sort → Player 1, Player 2, … */
  const players = React.useMemo(
    ()=>Object.keys(totals)
             .sort((a,b)=>parseInt(a.replace(/\D/g,''))-parseInt(b.replace(/\D/g,''))),
    [totals]
  );

  return (
    <div style={{
      flex:1,
      display:'flex', flexDirection:'column',
      rowGap:`${PAD*2}vmin`,
      padding:`${PAD}vmin`,
      overflowY:'auto'  // vertical scroll for many players
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
  );
};

export default ResourcesDisplay;