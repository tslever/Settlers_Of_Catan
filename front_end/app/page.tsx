'use client'

import { HexID, edges, hexes, idToColor, tokenMapping, vertices } from './utilities/board';
import React, { useEffect, useState } from 'react';


const portMapping: { [vertexLabel: string]: string } = {
  "V01": "3:1",
  "V06": "3:1",
  "V07": "Grain",
  "V08": "Grain",
  "V13": "Ore",
  "V23": "Ore",
  "V36": "3:1",
  "V37": "3:1",
  "V46": "Wool",
  "V47": "Wool",
  "V51": "3:1",
  "V52": "3:1",
  "V49": "3:1",
  "V50": "3:1",
  "V41": "Brick",
  "V27": "Brick",
  "V17": "Wood",
  "V18": "Wood",
};

type Settlement = {
  id: number;
  player: number;
  vertex: string;
};

type Road = {
  id: number;
  player: number;
  edge: string;
}

export default function Home() {
  const [serverMessage, setServerMessage] = useState<string>('');
  const [settlements, setSettlements] = useState<Settlement[]>([]);
  const [roads, setRoads] = useState<Road[]>([]);

  useEffect(() => {
    async function loadSettlements() {
      try {
        const response = await fetch("http://localhost:5000/settlements");
        if (!response.ok) {
          throw new Error(`Server error: ${response.status}`)
        }
        const data = await response.json();
        setSettlements(data.settlements);
      } catch (error: any) {
        console.error("Error fetching settlements:", error.message);
      }
    }
    loadSettlements();
  }, []);

  useEffect(() => {
    async function loadRoads() {
      try {
        const response = await fetch("http://localhost:5000/roads");
        if (!response.ok) {
          throw new Error(`Server error: ${response.status}`);
        }
        const data = await response.json();
        setRoads(data.roads);
      } catch (error: any) {
        console.error("Error fetching roads:", error.message);
      }
    }
    loadRoads();
  }, []);

  async function handleNext() {
    try {
      const response = await fetch("http://localhost:5000/next", {
        method: "POST",
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({})
      });
      if (!response.ok) {
        throw new Error(`Server error: ${response.status}`);
      }
      const data = await response.json();
      if (data.settlement) {
        setSettlements(prev => [...prev, data.settlement]);
      } else if (data.road) {
        setRoads(prev => [...prev, data.road]);
      }
      setServerMessage(data.message);
    } catch (error: any) {
      setServerMessage(`Error: ${error.message}`);
    }
  }

  return (
    <div>
      <div className="outer-container">
        <Ocean />
        <div className="board-container">
          <div className="board">
            {hexes.map(({ id, x, y }) => (
              <HexTile
                key={id}
                id={id}
                color={idToColor[id]}
                token={tokenMapping[id]}
                style={{ left: `${x}vmin`, top: `${y}vmin` }}
              />
            ))}
            <svg className="edge-layer" viewBox="0 0 100 100" preserveAspectRatio="none">
              {edges.map((edge, index) => {
                const midX = (edge.x1 + edge.x2) / 2;
                const midY = (edge.y1 + edge.y2) / 2;
                const label = `E${(index + 1).toString().padStart(2, '0')}`;
                return (
                  <g key={index}>
                    <line
                      x1={edge.x1}
                      y1={edge.y1}
                      x2={edge.x2}
                      y2={edge.y2}
                      stroke="blue"
                      strokeWidth="0.5"
                    />
                    <text
                      x={midX}
                      y={midY}
                      className="edge-label"
                      textAnchor="middle"
                      alignmentBaseline="middle"
                    >
                      {label}
                    </text>
                  </g>
                );
              })}
            </svg>
            <svg className = "road-layer" viewBox = "0 0 100 100" preserveAspectRatio = "none">
              {roads.map((road, index) => {
                const parts = road.edge.split('_');
                const [x1, y1] = parts[0].split('-').map(Number);
                const [x2, y2] = parts[1].split('-').map(Number);
                const colorMapping: { [key: number]: string } = { 1: 'red', 2: 'orange', 3: 'green' };
                const strokeColor = colorMapping[road.player] || 'gray';
                return (
                  <line
                    key = {index}
                    x1 = {x1}
                    y1 = {y1}
                    x2 = {x2}
                    y2 = {y2}
                    stroke = {strokeColor}
                    strokeWidth = "1"
                  />
                );
              })}
            </svg>
            {vertices.map((v, i) => {
              const vLabel = `V${(i + 1).toString().padStart(2, '0')}`;
              return (
                <React.Fragment key={i}>
                  <Vertex x={v.x} y={v.y} label={vLabel} />
                  {portMapping[vLabel] && (
                    <Port x={v.x} y={v.y} type={portMapping[vLabel]} />
                  )}
                </React.Fragment>
              );
            })}
            {settlements.map(s => {
              const vertexIndex = parseInt(s.vertex.substring(1)) - 1;
              const v = vertices[vertexIndex];
              if (!v) return null;
              return (
                <SettlementMarker key={s.id} x={v.x} y={v.y} player={s.player} />
              );
            })}
          </div>
        </div>
      </div>
      <div style={{ textAlign: 'center', marginTop: '1rem' }}>
        <button onClick={handleNext}>Next</button>
        {serverMessage && <p>{serverMessage}</p>}
      </div>
    </div>
  );
}

function Ocean() {
  const oceanWidth = 90; // vmin
  const oceanHeight = oceanWidth * 1.1547;
  return (
    <div
      className="ocean"
      style={{
        left: '50%',
        top: '50%',
        width: `${oceanWidth}vmin`,
        height: `${oceanHeight}vmin`,
        transform: 'translate(-50%, -50%) rotate(30deg)'
      }}
    />
  );
}

function HexTile({
  id,
  token,
  color,
  style
}: {
  id: HexID;
  token: number | null;
  color: string;
  style?: React.CSSProperties;
}) {
  const getTokenFontSize = (token: number): string => {
    const baseFontSize = 4;
    const mapping: { [key: number]: number } = {
      2: 3/11,
      3: 5/11,
      4: 7/11,
      5: 9/11,
      6: 1,
      8: 1,
      9: 9/11,
      10: 7/11,
      11: 5/11,
      12: 3/11
    };
    return `${baseFontSize * (mapping[token] || 1)}vmin`;
  };

  const tokenDotMapping: { [key: number]: number } = {
    2: 1,
    3: 2,
    4: 3,
    5: 4,
    6: 5,
    8: 5,
    9: 4,
    10: 3,
    11: 2,
    12: 1
  };

  return (
    <div className="hex-tile" style={{ ...style, backgroundColor: color }}>
      {token !== null && (
        <div
          className="hex-token"
          style={{
            fontSize: getTokenFontSize(token),
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center'
          }}>
            <div
              className="token-number"
            >
              {token}
            </div>
            <div
              className="token-dots"
            >
              {Array.from({ length: tokenDotMapping[token] || 0 }, (_, i) => (
                <span key = {i} className = "dot" />
              ))}
            </div>
          </div>
        )}
    </div>
  );
}

function Vertex({ x, y, label }: { x: number; y: number; label: string }) {
  return (
    <div className="vertex" style={{ left: `${x}vmin`, top: `${y}vmin` }}>
      <span className="vertex-label">{label}</span>
    </div>
  );
}

function Port({ x, y, type }: { x: number; y: number; type: string }) {
  return (
    <div className="port" style={{ left: `${x}vmin`, top: `${y}vmin` }}>
      <span className="port-label">{type}</span>
    </div>
  );
}

function SettlementMarker({ x, y, player }: { x: number; y: number; player: number }) {
  const colorMapping: { [key: number]: string } = {
    1: 'red',
    2: 'orange',
    3: 'green'
  };
  return (
    <div
      className="settlement"
      style={{
        left: `${x}vmin`,
        top: `${y}vmin`,
        backgroundColor: colorMapping[player] || 'gray'
      }}
    />
  );
}