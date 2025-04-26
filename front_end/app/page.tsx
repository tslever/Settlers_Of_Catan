'use client';

import { API, apiFetch } from './api';
import { Board } from './BoardLayout';
import { Dice, RecommendMoveResponse, Totals, WallInformation } from './types';
import HexTile from './components/HexTile';
import { ID_Of_Hex, ResetResponse } from './types';
import { AutomateAndMakeMoveResponse } from './types';
import Ocean from './components/Ocean';
import { OuterContainer } from './BoardLayout';
import { BoardContainer } from './BoardLayout';
import Port from './components/Port';
import React from 'react';
import { Road } from './types';
import { RoadLayer } from './BoardLayout';
import { Settlement } from './types';
import { getPositionStyles } from './BoardLayout';
import { hexes } from './board';
import { idToColor } from './types';
import { jsonArrayOfEdgeInformation } from './board';
import { portMapping } from './types';
import { tokenMapping } from './types';
import { vertices } from './board';
import { useMutation } from '@tanstack/react-query';
import { useQueryClient } from '@tanstack/react-query';
import { useState } from 'react';
import CanvasLayer from './CanvasLayer';
import Marker from './components/Marker';
import { useCentralQuery } from './hooks/useCentralQuery';
import QueryBoundary from './components/QueryBoundary';
import ResourcesDisplay, { ZERO_BAG } from './components/ResourcesDisplay';
import Wall from './components/Wall';


const vertexMapping: Record<string, { x: number, y: number }> =
    vertices.reduce((acc, v, i) => {
        const label = `V${(i + 1).toString().padStart(2, '0')}`;
        acc[label] = v;
        return acc;
    }, {} as Record<string, { x: number; y: number }>);


export default function Home() {


    const [menuAnchor, setMenuAnchor] = useState<{
        label: string;
        x: number;
        y: number;
        isEdge?: boolean;
    } | null>(null);


    const handleVertexClick = (label: string) => {
        const { x, y } = vertexMapping[label];
        setMenuAnchor(prev => prev?.label === label && !prev.isEdge ? null : { label, x, y });
    };


    const handleEdgeClick = (label: string) => {
        const index = parseInt(label.slice(1), 10) - 1;
        const { x1, y1, x2, y2 } = jsonArrayOfEdgeInformation[index];
        const midX = (x1 + x2) / 2;
        const midY = (y1 + y2) / 2;
        setMenuAnchor(prev => prev?.label === label && prev.isEdge ? null : { label, x: midX, y: midY, isEdge: true });
    };


    const queryClient = useQueryClient();


    const { data: stateData, isLoading: stateLoading, error: stateError } = useCentralQuery<AutomateAndMakeMoveResponse>(
        ["state"],
        () => apiFetch<AutomateAndMakeMoveResponse>(API.endpoints.state)
    );


    const phase = stateData?.phase;


    const possibleNextMoves = stateData?.possibleNextMoves;


    const { data: recommendation, isLoading: recommendationLoading, error: recommendationError, refetch: refetchRecommendation } = useCentralQuery<RecommendMoveResponse>(
        ['recommendMove'],
        () => apiFetch<RecommendMoveResponse>(API.endpoints.recommendMove),
        { enabled: false }
    );


    const {
        data: settlementsData,
        isLoading: settlementsLoading,
        error: settlementsError
    } = useCentralQuery<{ settlements: Settlement[] }>(
        ["settlements"],
        () => apiFetch<{ settlements: Settlement[] }>(API.endpoints.settlements)
    );


    const {
        data: citiesData,
        isLoading: citiesLoading,
        error: citiesError
    } = useCentralQuery<{ cities: { id: Number; player: Number; vertex: string }[] }>(
        ["cities"],
        () => apiFetch<{ cities: { id: Number; player: Number; vertex: string }[] }>(API.endpoints.cities)
    );


    const { data: wallsData, isLoading: wallsLoading } = useCentralQuery<{ walls: WallInformation[] }>(
        ['walls'],
        () => apiFetch<{ walls: WallInformation[] }>(API.endpoints.walls)
    );


    const {
        data: roadsData,
        isLoading: roadsLoading,
        error: roadsError
    } = useCentralQuery<{ roads: Road[] }>(
        ["roads"],
        () => apiFetch<{ roads: Road[] }>(API.endpoints.roads)
    );


    const EMPTY_TOTALS: Totals = {};


    const dice: Dice | null = stateData?.dice ?? null;
    const message = stateData?.message ?? "";
    const gainedResources = stateData?.gainedResources ?? EMPTY_TOTALS;
    const totals = stateData?.totalResources ?? EMPTY_TOTALS;


    const { mutate: postAutomateMove, isPending: automateMoveLoading } = useMutation<AutomateAndMakeMoveResponse, Error>(
        {
            mutationFn: () => apiFetch<AutomateAndMakeMoveResponse>(
                API.endpoints.automateMove,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({})
                }
            ),
            onSuccess: (data) => {
                queryClient.invalidateQueries({ queryKey: ["state"] });
                queryClient.invalidateQueries({ queryKey: ["cities"] });
                queryClient.invalidateQueries({ queryKey: ["settlements"] });
                queryClient.invalidateQueries({ queryKey: ["roads"] });
                queryClient.invalidateQueries({ queryKey: ["walls"] });
                queryClient.removeQueries({ queryKey: ["recommendMove"] });
            }
        }
    );


    const { mutate: postMakeMove, isPending: makeMoveLoading } = useMutation<AutomateAndMakeMoveResponse, Error, { move: string; moveType: string }>(
        {
            mutationFn: ({ move, moveType }) => apiFetch<AutomateAndMakeMoveResponse>(
                API.endpoints.makeMove,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ move, moveType })
                }
            ),
            onSuccess: data => {
                queryClient.invalidateQueries( { queryKey: ['state'] });
                queryClient.invalidateQueries( { queryKey: ['cities'] });
                queryClient.invalidateQueries( { queryKey: ['settlements'] });
                queryClient.invalidateQueries( { queryKey: ['roads'] });
                queryClient.invalidateQueries( { queryKey: ['walls'] });
                setMenuAnchor(null);
                queryClient.removeQueries({ queryKey: ["recommendMove"] });
            }
        }
    );


    const { mutate: resetGame, isPending: resetLoading } = useMutation<ResetResponse, Error>(
        {
            mutationFn: () => apiFetch<ResetResponse>(
                API.endpoints.reset,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({})
                }
            ),
            onSuccess: () => {
                queryClient.invalidateQueries({ queryKey: ["state"] });
                queryClient.invalidateQueries({ queryKey: ["cities"] });
                queryClient.invalidateQueries({ queryKey: ["settlements"] });
                queryClient.invalidateQueries({ queryKey: ["roads"] });
                queryClient.invalidateQueries({ queryKey: ["walls"] });
                queryClient.removeQueries({ queryKey: ["recommendMove"] });
                setMenuAnchor(null);
            }
        }
    );

    const isBoardLoading = stateLoading || settlementsLoading || citiesLoading || roadsLoading || wallsLoading;
    const boardError = stateError || settlementsError || citiesError || roadsError;
    
    const colorMapping: Record<number, string> = {
        1: "red",
        2: "orange",
        3: "green"
    };

    const eventColor: Record<Dice["whiteEventDie"], string> = {
        yellow: "yellow",
        green: "green",
        blue: "blue",
        black: "black"
    };

    const pillStyle: React.CSSProperties = {
        padding: '0.5rem 1rem',
        borderRadius: '0.5rem',
        background: 'white',
        border: '1px solid #333',
        fontWeight: 'bold',
        cursor: 'pointer',
        width: '6rem',
        textAlign: 'center'
    };

    return (
        <div style = {{ display: 'grid', gridTemplateColumns: '100vmin 1fr', gridTemplateRows: "1fr", alignItems: 'stretch', height: "100dvh", columnGap: '1rem', padding: '1rem' }}>
            <QueryBoundary isLoading = {isBoardLoading} error = {boardError}>
                <OuterContainer>
                    <Ocean />
                    <BoardContainer>
                        <Board>
                            {hexes.map(({ id, x, y }) => {
                                const idOfHex = id as ID_Of_Hex;
                                return (
                                    <HexTile
                                        key = {idOfHex}
                                        id = {idOfHex}
                                        color = {idToColor[idOfHex]}
                                        token = {tokenMapping[idOfHex]}
                                        style = {getPositionStyles(x, y)}
                                    />
                                );
                            })}
                            <CanvasLayer />
                            {settlementsData?.settlements.map((s) => {
                                const v = vertexMapping[s.vertex];
                                return v ? <Marker key = {s.id} x = {v.x} y = {v.y} player = {s.player} type = "settlement" /> : null;
                            })}
                            {citiesData?.cities.map((c) => {
                                const v = vertexMapping[c.vertex];
                                return v ? <Marker key = {c.id.toString()} x = {v.x} y = {v.y} player = {c.player as number} type = "city" /> : null;
                            })}
                            {wallsData?.walls.map(w => {
                                const v = vertexMapping[w.vertex];
                                return v ? <Wall key = {w.id} x = {v.x} y = {v.y} player = {w.player} /> : null;
                            })}
                            {vertices.map((v, i) => {
                                const labelOfVertex = `V${(i + 1).toString().padStart(2, '0')}`;
                                return portMapping[labelOfVertex] ? <Port key = {labelOfVertex} x = {v.x} y = {v.y} type = {portMapping[labelOfVertex]} /> : null;
                            })}
                            <RoadLayer viewBox = "0 0 100 100" preserveAspectRatio = "none">
                                {roadsData?.roads.map((road, index) => {
                                    // road.edge is like "E01".
                                    const indexOfEdge = parseInt(road.edge.slice(1), 10) - 1;
                                    const { x1, y1, x2, y2 } = jsonArrayOfEdgeInformation[indexOfEdge];
                                    const strokeColor = colorMapping[road.player] || "gray";
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
                                {possibleNextMoves?.edges?.map(labelOfEdge => {
                                    const indexOfEdge = parseInt(labelOfEdge.slice(1), 10) - 1;
                                    const { x1, y1, x2, y2 } = jsonArrayOfEdgeInformation[indexOfEdge];
                                    return (
                                        <line
                                            key = {labelOfEdge}
                                            x1 = {x1}
                                            y1 = {y1}
                                            x2 = {x2}
                                            y2 = {y2}
                                            stroke = {colorMapping[possibleNextMoves.player]}
                                            strokeWidth = {3}
                                            opacity = {0.4}
                                            style = {{ cursor: 'pointer' }}
                                            pointerEvents = "stroke"
                                            onClick = {() => handleEdgeClick(labelOfEdge)}
                                        />
                                    );
                                })}
                            </RoadLayer>
                            {possibleNextMoves?.vertices && Object.keys(possibleNextMoves.vertices).map(labelOfVertex => {
                                const { x, y } = vertexMapping[labelOfVertex];
                                return (
                                    <div
                                        key = {labelOfVertex}
                                        onClick = { () => handleVertexClick(labelOfVertex) }
                                        style = {{
                                            position: 'absolute',
                                            left: `${x}vmin`,
                                            top: `${y}vmin`,
                                            width: '2vmin',
                                            height: '2vmin',
                                            borderRadius: '50%',
                                            backgroundColor: colorMapping[possibleNextMoves.player],
                                            opacity: 0.8,
                                            transform: 'translate(-50%, -50%)',
                                            border: '0.2vmin solid black',
                                            cursor: 'pointer'
                                        }}
                                    />
                                );
                            })}

                        </Board>
                    </BoardContainer>
                </OuterContainer>
            </QueryBoundary>

            <div style = {{ display: "flex", flexDirection: "column", alignItems: "flex-start", rowGap: "1rem", padding: "1rem", background: "#000", color: "#fff", height: "100%", minHeight: 0, overflowY: "auto", boxSizing: "border-box" }}>
                <p style = {{ margin: 0 }}>{message}</p>
                <div style = {{ display: "flex", columnGap: "1rem" }}>
                    <button
                        style = {pillStyle}
                        onClick = {() => postAutomateMove()}
                        disabled = {!possibleNextMoves?.nextPlayerWillRollDice}
                    >
                        Roll Dice
                    </button>
                    <button
                        style = {pillStyle}
                        onClick = {() => postMakeMove({ move: "pass", moveType: "pass" })}
                        disabled = {phase != "turn"}
                    >
                        Pass
                    </button>
                </div>
                <div style = {{ display: "flex", columnGap: "1rem" }}>
                    <div style = {{ width: "3rem", height: "3rem", background: "yellow", border: "1px solid #333", display: "flex", alignItems: "center", justifyContent: "center", fontWeight: "bold" }}>
                        {dice?.yellowProductionDie ?? ""}
                    </div>
                    <div style = {{ width: "3rem", height: "3rem", background: "red", border: "1px solid #333", display: "flex", alignItems: "center", justifyContent: "center", fontWeight: "bold" }}>
                        {dice?.redProductionDie ?? ""}
                    </div>
                    <div style = {{ width: "3rem", height: "3rem", background: "white", border: "1px solid #333", display: "flex", alignItems: "center", justifyContent: "center" }}>
                        {dice?.whiteEventDie && (
                            <div style = {{width: "60%", height: "60%", borderRadius: "50%", background: eventColor[dice.whiteEventDie]}}/>
                        )}
                    </div>
                </div>

                <div style = {{ flex: 1, display: "flex", flexDirection: "column", minHeight: 0 }}>
                    <ResourcesDisplay totals = {totals} gained = {gainedResources}/>
                </div>
            </div>

            {menuAnchor && (
                <ul
                    style = {{
                        position: 'absolute',
                        left: `${menuAnchor.x}vmin`,
                        top: `${menuAnchor.y}vmin`,
                        background: 'white',
                        border: '1px solid #333',
                        borderRadius: '0.25rem',
                        padding: '0.5rem',
                        listStyle: 'none',
                        zIndex: 10,
                        transform: 'translate(-50%, -100%)'
                    }}
                >
                    {menuAnchor.isEdge
                            ? (
                                <li
                                    key = "road"
                                    onClick = {() => postMakeMove({ move: menuAnchor.label, moveType: 'road' })}
                                    style = {{ opacity: makeMoveLoading ? 0.5 : 1, cursor: 'pointer' }}
                                >
                                    road
                                </li>
                            )
                            : (
                                (possibleNextMoves?.vertices[menuAnchor.label] ?? []).map(mt => (
                                    <li
                                        key = {mt}
                                        onClick = {() => postMakeMove({ move: menuAnchor.label, moveType: mt })}
                                        style = {{ opacity: makeMoveLoading ? 0.5 : 1, cursor: 'pointer' }}
                                    >
                                        {mt}
                                    </li>
                                ))
                            )
                    }
                </ul>
            )}
            <button onClick = {() => refetchRecommendation()} disabled = {recommendationLoading}>
                {recommendationLoading ? "Thinking..." : "Recommend Move"}
            </button>
            {recommendationError && <p style = {{ color: "red" }}>Error: {(recommendationError as Error).message}</p>}
            {recommendation && (
                <p>
                    Recommendation: <strong>{recommendation.moveType}</strong> at <strong>{recommendation.move}</strong>
                </p>
            )}
            <button onClick = {() => postAutomateMove()} disabled = {automateMoveLoading}>
                {automateMoveLoading ? "Loading..." : "Automate Move"}
            </button>
            <button onClick = {() => resetGame()} disabled = {resetLoading}>
                {resetLoading ? "Resetting..." : "Reset Game"}
            </button>
        </div>
    );
}