'use client';

import { API, apiFetch } from './api';
import { Board } from './BoardLayout';
import { Dice, ResourcesByKind, Totals, WallInformation } from './types';
import HexTile from './components/HexTile';
import { ID_Of_Hex, ResetResponse } from './types';
import { NextResponse } from './types';
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
import { useEffect } from 'react';
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


function isEmpty(obj: object) {
    return Object.keys(obj).length == 0;
}


export default function Home() {
    const queryClient = useQueryClient();

    const { data: stateData, isLoading: stateLoading, error: stateError } = useCentralQuery<NextResponse>(
        ["state"],
        () => apiFetch<NextResponse>(API.endpoints.state)
    );


    const [movesToHighlight, setMovesToHighlight] = useState<{
        player: number;
        verticesToHighlight: string[];
        edgesToHighlight: string[];
    } | null>(null);


    useEffect(() => {
        const movesToHighlight = stateData?.movesToHighlight;
        if (movesToHighlight && Array.isArray(movesToHighlight.verticesToHighlight) && Array.isArray(movesToHighlight.edgesToHighlight)) {
            setMovesToHighlight(movesToHighlight);
        }
    }, [stateData]);


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


    const INITIAL_TOTALS: Totals = {
        "Player 1": ZERO_BAG,
        "Player 2": ZERO_BAG,
        "Player 3": ZERO_BAG
    }


    const dice = stateData?.dice ?? null;
    const message = stateData?.message ?? "";
    const gainedResources = stateData?.gainedResources && !isEmpty(stateData.gainedResources) ? stateData.gainedResources : INITIAL_TOTALS;
    const totals = stateData?.totalResources && !isEmpty(stateData?.totalResources) ? stateData?.totalResources : INITIAL_TOTALS;


    const { mutate: postNextMove, isPending: nextLoading } = useMutation<NextResponse, Error>(
        {
            mutationFn: () => apiFetch<NextResponse>(
                API.endpoints.next,
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({})
                }
            ),
            onSuccess: (data) => {
                setMovesToHighlight(data.movesToHighlight);
                queryClient.invalidateQueries({ queryKey: ["state"] });
                queryClient.invalidateQueries({ queryKey: ["cities"] });
                queryClient.invalidateQueries({ queryKey: ["settlements"] });
                queryClient.invalidateQueries({ queryKey: ["roads"] });
                queryClient.invalidateQueries({ queryKey: ["walls"] });
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
                setMovesToHighlight(null);
                queryClient.invalidateQueries({ queryKey: ["state"] });
                queryClient.invalidateQueries({ queryKey: ["cities"] });
                queryClient.invalidateQueries({ queryKey: ["settlements"] });
                queryClient.invalidateQueries({ queryKey: ["roads"] });
                queryClient.invalidateQueries({ queryKey: ["walls"] });
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

    return (
        <div style = {{ display: 'grid', gridTemplateColumns: '100vmin auto', alignItems: 'start', columnGap: '1rem', padding: '1rem' }}>
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
                                {movesToHighlight?.edgesToHighlight.map((labelOfEdge, i) => {
                                    const indexOfEdge = parseInt(labelOfEdge.slice(1), 10) - 1;
                                    const { x1, y1, x2, y2 } = jsonArrayOfEdgeInformation[indexOfEdge];
                                    return (
                                        <line
                                            key = {i}
                                            x1 = {x1}
                                            y1 = {y1}
                                            x2 = {x2}
                                            y2 = {y2}
                                            stroke = {colorMapping[movesToHighlight.player]}
                                            strokeWidth = {3}
                                            opacity = {0.4}
                                        />
                                    );
                                })}
                            </RoadLayer>
                            {movesToHighlight?.verticesToHighlight.map((labelOfVertex, i) => {
                                const {x, y} = vertexMapping[labelOfVertex];
                                return (
                                    <div
                                        key = {i}
                                        style = {{
                                            position: 'absolute',
                                            left: `${x}vmin`,
                                            top: `${y}vmin`,
                                            width: '2vmin',
                                            height: '2vmin',
                                            borderRadius: '50%',
                                            backgroundColor: colorMapping[movesToHighlight.player],
                                            opacity: 0.8,
                                            transform: 'translate(-50%, -50%)',
                                            border: '0.2vmin solid black'
                                        }}
                                    />
                                );
                            })}
                        </Board>
                    </BoardContainer>
                </OuterContainer>
            </QueryBoundary>
            <div style = {{ display: "flex", flexDirection: "column", alignItems: "flex-start" }}>
                <button onClick = {() => postNextMove()} disabled = {nextLoading}>
                    {nextLoading ? "Loading..." : "Next"}
                </button>
                <button onClick = {() => resetGame()} disabled = {resetLoading} style = {{ marginLeft: "0.5rem" }}>
                    { resetLoading ? "Resetting..." : "Reset Game" }
                </button>
                <div className = "dice-display" style = {{ marginTop: "1rem" }}>
                    <p>Yellow production die: {dice?.yellowProductionDie ?? '?'}</p>
                    <p>Red production die: {dice?.redProductionDie ?? '?'}</p>
                    <p>White event die: {dice?.whiteEventDie ?? '?'}</p>
                </div>
                <p>{message}</p>
                <ResourcesDisplay totals = {totals} gained = {gainedResources} />
            </div>
        </div>
    );
}