'use client';

import { API, apiFetch } from './api';
import { Board } from './BoardLayout';
import { Dice, ResourcesByKind, Totals } from './types';
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
import ResourcesDisplay from './components/ResourcesDisplay';


const vertexMapping: Record<string, { x: number, y: number }> =
    vertices.reduce((acc, v, i) => {
        const label = `V${(i + 1).toString().padStart(2, '0')}`;
        acc[label] = v;
        return acc;
    }, {} as Record<string, { x: number; y: number }>);


export default function Home() {

    const [mounted, setMounted] = useState(false);
    const [message, setMessage] = useState("");
    const [dice, setDice] = useState<Dice | null>(null);
    const [gained, setGained] = useState<Totals>({});

    const queryClient = useQueryClient();

    useEffect(() => {
        setMounted(true);
        apiFetch<{message:string}>(API.endpoints.message).then(data => setMessage(data.message)).catch(() => {});
    }, []);

    const {
        data: resourcesData,
        isLoading: resourcesLoading,
        error: resourcesError
    } = useCentralQuery<Record<string, ResourcesByKind>>(
        ["resources"],
        () => apiFetch<Record<string, ResourcesByKind>>(API.endpoints.resources),
        {enabled: mounted}
    );

    const totals: Totals = React.useMemo(() => {
        if (!resourcesData) {
            return {};
        }
        const out: Totals = {};
        Object.entries(resourcesData).forEach(([label, bag]) => {
            const m = label.match(/^Player (\d+)$/);
            if (m) {
                out[+m[1]] = bag;
            }
        });
        return out;
    }, [resourcesData]);

    const {
        data: settlementsData,
        isLoading: settlementsLoading,
        error: settlementsError
    } = useCentralQuery<{ settlements: Settlement[] }>(
        ["settlements"],
        () => apiFetch<{ settlements: Settlement[] }>(API.endpoints.settlements),
        { enabled: mounted }
    );

    const {
        data: citiesData,
        isLoading: citiesLoading,
        error: citiesError
    } = useCentralQuery<{ cities: { id: Number; player: Number; vertex: string }[] }>(
        ["cities"],
        () => apiFetch<{ cities: { id: Number; player: Number; vertex: string }[] }>(API.endpoints.cities),
        { enabled: mounted }
    );

    const {
        data: roadsData,
        isLoading: roadsLoading,
        error: roadsError
    } = useCentralQuery<{ roads: Road[] }>(
        ["roads"],
        () => apiFetch<{ roads: Road[] }>(API.endpoints.roads),
        { enabled: mounted }
    );


    const {
        mutate: postNextMove,
        isPending: nextLoading
    } = useMutation<NextResponse, Error>({
        mutationFn: () => apiFetch<NextResponse>(API.endpoints.next, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({})
        }),
        onSuccess: (data) => {
            setDice(data.dice ?? null);
            setMessage(data.message);

            if (data.gainedResources) {
                setGained(data.gainedResources);
            } else {
                setGained({});
            }

            queryClient.invalidateQueries({ queryKey: ["cities"] });
            queryClient.invalidateQueries({ queryKey: ["settlements"] });
            queryClient.invalidateQueries({ queryKey: ["roads"] });
            queryClient.invalidateQueries({ queryKey: ["resources"] });
        },
        onError: (error: Error) => {
            setMessage(error.message);
        }
    });


    const {
        mutate: resetGame,
        isPending: resetLoading,
        data: resetData
    } = useMutation<ResetResponse, Error>({
        mutationFn: () =>
            apiFetch<ResetResponse>(API.endpoints.reset, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            }),
        onSuccess: (data) => {
            setMessage(data.message);
            setDice(null);
            const zeroGains: Totals = {};
            Object.keys(totals).forEach(playerLabel => {
                zeroGains[playerLabel] = { brick: 0, grain: 0, lumber: 0, ore: 0, wool: 0 };
            });
            setGained(zeroGains);

            queryClient.invalidateQueries({ queryKey: ["cities"] });
            queryClient.invalidateQueries({ queryKey: ["settlements"] });
            queryClient.invalidateQueries({ queryKey: ["roads"] });
            queryClient.invalidateQueries({ queryKey: ["resources"] });
        },
        onError: (error: Error) => {
            setMessage(error.message);
        }
    });

    if (!mounted) {
        return null;
    }

    const handleNext = () => {
        postNextMove();
    }

    const handleReset = () => {
        resetGame();
    }

    const settlements = settlementsData?.settlements ?? [];
    const roads = roadsData?.roads ?? [];
    const cities = citiesData?.cities ?? [];

    const isBoardLoading = settlementsLoading || citiesLoading || roadsLoading;
    const boardError = settlementsError || citiesError || roadsError;

    return (
        <div>
            <QueryBoundary isLoading = {isBoardLoading} error = {boardError}>
                <OuterContainer>
                    <Ocean />
                    <BoardContainer>
                        <Board>
                            {hexes.map(({ id, x, y }) => {
                                const id_of_hex = id as ID_Of_Hex;
                                return (
                                    <HexTile
                                        key = {id_of_hex}
                                        id = {id_of_hex}
                                        color = {idToColor[id_of_hex]}
                                        token = {tokenMapping[id_of_hex]}
                                        style = {getPositionStyles(x, y)}
                                    />
                                );
                            })}
                            <CanvasLayer />
                            {vertices.map((v, i) => {
                                const vLabel = `V${(i + 1).toString().padStart(2, '0')}`;
                                return (
                                    <React.Fragment key = {i}>
                                        {portMapping[vLabel] && <Port x = {v.x} y = {v.y} type = {portMapping[vLabel]} />}
                                    </React.Fragment>
                                );
                            })}
                            {settlements.map((s) => {
                                const v = vertexMapping[s.vertex];
                                if (!v) { return null; }
                                return <Marker key = {s.id} x = {v.x} y = {v.y} player = {s.player} type = "settlement" />
                            })}
                            {cities.map((c) => {
                                const v = vertexMapping[c.vertex];
                                if (!v) { return null; }
                                return <Marker key = {c.id.toString()} x = {v.x} y = {v.y} player = {c.player as number} type = "city" />;
                            })}
                            <RoadLayer viewBox = "0 0 100 100" preserveAspectRatio = "none">
                                {roads.map((road, index) => {
                                    // road.edge is like "E01".
                                    const indexOfEdge = parseInt(road.edge.slice(1), 10) - 1;
                                    const jsonObjectOfEdgeInformation = jsonArrayOfEdgeInformation[indexOfEdge];
                                    const { x1, y1, x2, y2 } = jsonObjectOfEdgeInformation;
                                    const colorMapping: Record<number, string> = {
                                        1: "red",
                                        2: "orange",
                                        3: "green"
                                    };
                                    const strokeColor = colorMapping[road.player] ?? "gray";
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
                            </RoadLayer>
                        </Board>
                    </BoardContainer>
                </OuterContainer>
            </QueryBoundary>
            <div style = {{ textAlign: "center", marginTop: "1rem" }}>
                <button onClick = {handleNext} disabled = {nextLoading}>
                    {nextLoading ? "Loading..." : "Next"}
                </button>
                <button onClick = {handleReset} disabled = {resetLoading} style = {{ marginLeft: "1rem" }}>
                    { resetLoading ? "Resetting..." : "Reset Game" }
                </button>
                <div className = "dice-display">
                    <p>
                        Yellow production die:&nbsp;
                        {dice?.yellowProductionDie ?? '?'}
                    </p>
                    <p>
                        Red production die:&nbsp;
                        {dice?.redProductionDie ?? '?'}
                    </p>
                    <p>
                        White event die:&nbsp;
                        {dice?.whiteEventDie ?? '?'}
                    </p>
                </div>
                {message && <p>{message}</p>}
                <QueryBoundary isLoading = {resourcesLoading} error = {resourcesError}>
                    <div>
                        <ResourcesDisplay
                            totals = {resourcesData ?? {}}
                            gained = {gained}
                        />
                    </div>
                </QueryBoundary>
            </div>
        </div>
    );
}