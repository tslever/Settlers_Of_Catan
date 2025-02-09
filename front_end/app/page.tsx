'use client'

import HexTile from './components/HexTile';
import Ocean from './components/Ocean';
import Port from './components/Port';
import React from 'react';
import SettlementMarker from './components/SettlementMarker';
import Vertex from './components/Vertex';
import { edges } from './utilities/board';
import { hexes, } from './utilities/board';
import { idToColor } from './utilities/board';
import { tokenMapping } from './utilities/board';
import { vertices } from './utilities/board';
import { URL_OF_BACK_END } from './config';
import { useApi } from './hooks/useApi';
import { useEffect } from 'react';
import { useState } from 'react';

type Road = {
    id: number;
    player: number;
    edge: string;
};

type Settlement = {
    id: number;
    player: number;
    vertex: string;
};

type MoveType = "settlement" | "road";

type NextResponse =
    | { moveType: "settlement"; message: string; settlement: Settlement }
    | { moveType: "road"; message: string; road: Road };

function isErrorResponse(
    data: NextResponse | { error: string }
): data is { error: string } {
    return 'error' in data;
}

const vertexMapping: { [label: string]: { x: number; y: number } } =
    vertices.reduce((acc, v, i) => {
        const label = `V${(i + 1).toString().padStart(2, "0")}`;
        acc[label] = v;
        return acc;
    }, {} as { [label: string]: { x: number; y: number } });

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

export default function Home() {
    const [serverMessage, setServerMessage] = useState<string>('');
    const [nextLoading, setNextLoading] = useState<boolean>(false);

    const {
        data: settlementsData,
        error: settlementsError,
        loading: settlementsLoading,
        refetch: refetchSettlements
    } = useApi<{ settlements: Settlement[] }>(`${URL_OF_BACK_END}/settlements`);

    const {
        data: roadsData,
        error: roadsError,
        loading: roadsLoading,
        refetch: refetchRoads
    } = useApi<{ roads: Road[] }>(`${URL_OF_BACK_END}/roads`);

    useEffect(() => {
        refetchSettlements();
        refetchRoads();
    }, [refetchSettlements, refetchRoads]);

    const settlements = settlementsData ? settlementsData.settlements : [];
    const roads = roadsData ? roadsData.roads : [];

    async function handleSettlementCreated(settlement: Settlement, message: string) {
        setServerMessage(message);
    }

    async function handleRoadCreated(road: Road, message: string) {
        setServerMessage(message);
    }

    async function handleNext() {
        setNextLoading(true);
        try {
            const response = await fetch(`${URL_OF_BACK_END}/next`, {
                method: "POST",
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({})
            });
            const jsonData: NextResponse | { error: string } = await response.json();
            if (isErrorResponse(jsonData)) {
                setServerMessage(jsonData.error || `Server error: ${response.status}`);
            } else {
                if (jsonData.moveType === "settlement") {
                    await handleSettlementCreated(jsonData.settlement, jsonData.message);
                } else if (jsonData.moveType === "road") {
                    await handleRoadCreated(jsonData.road, jsonData.message);
                } else {
                    setServerMessage("Unknown move type.");
                }
                refetchSettlements();
                refetchRoads();
            }
        } catch (error: unknown) {
            if (error instanceof Error) {
                setServerMessage(`Error: ${error.message}`);
            } else {
                setServerMessage(`Error: ${error}`);
            }
        } finally {
            setNextLoading(false);
        }
    }

    return (
        <div>
            <div className = "outer-container">
                <Ocean />
                <div className = "board-container">
                    <div className = "board">
                        {hexes.map(({ id, x, y }) => (
                            <HexTile
                                key = {id}
                                id = {id}
                                color = {idToColor[id]}
                                token = {tokenMapping[id]}
                                style = {{ left: `${x}vmin`, top: `${y}vmin` }}
                            />
                        ))}
                        <svg className = "edge-layer" viewBox="0 0 100 100" preserveAspectRatio = "none">
                            {edges.map((edge, index) => {
                                const midX = (edge.x1 + edge.x2) / 2;
                                const midY = (edge.y1 + edge.y2) / 2;
                                const label = `E${(index + 1).toString().padStart(2, '0')}`;
                                return (
                                    <g key = {index}>
                                        <line
                                            x1 = {edge.x1}
                                            y1 = {edge.y1}
                                            x2 = {edge.x2}
                                            y2 = {edge.y2}
                                            stroke = "blue"
                                            strokeWidth = "0.5"
                                        />
                                        <text
                                            x = {midX}
                                            y = {midY}
                                            className = "edge-label"
                                            textAnchor = "middle"
                                            alignmentBaseline = "middle"
                                        >
                                            {label}
                                        </text>
                                    </g>
                                );
                            })}
                        </svg>
                        <svg className = "road-layer" viewBox = "0 0 100 100" preserveAspectRatio = "none">
                            {roads.map((road, index) => {
                                const [firstPart, secondPart] = road.edge.split('_');
                                const [x1, y1] = firstPart.split('-').map(Number);
                                const [x2, y2] = secondPart.split('-').map(Number);
                                const colorMapping: { [key: number]: string } = {
                                    1: 'red',
                                    2: 'orange',
                                    3: 'green'
                                };
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
                                <React.Fragment key = {i}>
                                    <Vertex x = {v.x} y = {v.y} label = {vLabel} />
                                    {portMapping[vLabel] && <Port x={v.x} y={v.y} type={portMapping[vLabel]} />}
                                </React.Fragment>
                            );
                        })}
                        {settlements.map((s) => {
                            const v = vertexMapping[s.vertex];
                            if (!v) return null;
                            return <SettlementMarker key = {s.id} x = {v.x} y = {v.y} player = {s.player} />
                        })}
                    </div>
                </div>
            </div>
            <div style = {{ textAlign: 'center', marginTop: '1rem' }}>
                <button onClick = {handleNext} disabled = {nextLoading}>
                    {nextLoading ? "Loading..." : "Next"}
                </button>
                {serverMessage && <p>{serverMessage}</p>}
                {(settlementsError || roadsError) && (
                    <p style = {{ color: "red "}}>
                        {settlementsError || roadsError}
                    </p>
                )}
            </div>
        </div>
    );
}