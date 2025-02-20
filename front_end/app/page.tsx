'use client';

import { Board } from './BoardLayout';
import HexTile from './components/HexTile';
import { ID_Of_Hex } from './types';
import { NextResponse } from './types';
import Ocean from './components/Ocean';
import { OuterContainer } from './BoardLayout';
import { BoardContainer } from './BoardLayout';
import Port from './components/Port';
import React from 'react';
import { Road } from './types';
import { RoadLayer } from './BoardLayout';
import { Settlement } from './types';
import { URL_OF_BACK_END } from './config';
import { getPositionStyles } from './BoardLayout';
import { hexes } from './board';
import { idToColor } from './types';
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


const vertexMapping: Record<string, { x: number, y: number }> =
    vertices.reduce((acc, v, i) => {
        const label = `V${(i + 1).toString().padStart(2, '0')}`;
        acc[label] = v;
        return acc;
    }, {} as Record<string, { x: number; y: number }>);


const portMapping: Record<string, string> = {
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
    const [mounted, setMounted] = useState(false);
    const queryClient = useQueryClient();

    useEffect(() => {
        setMounted(true);
    }, []);

    const {
        data: settlementsData,
        isLoading: settlementsLoading,
        error: settlementsError
    } = useCentralQuery<{ settlements: Settlement[] }>(
        ["settlements"],
        async() => {
            const response = await fetch(`${URL_OF_BACK_END}/settlements`);
            if (!response.ok) {
                throw new Error(`The following error occurred when fetching settlements: ${response.statusText}`);
            }
            return response.json();
        },
        { enabled: mounted }
    )

    const {
        data: citiesData,
        isLoading: citiesLoading,
        error: citiesError
    } = useCentralQuery<{ cities: { id: Number; player: Number; vertex: string }[] }>(
        ["cities"],
        async() => {
            const response = await fetch(`${URL_OF_BACK_END}/cities`);
            if (!response.ok) {
                throw new Error(`Error fetching cities: ${response.statusText}`);
            }
            return response.json();
        },
        { enabled: mounted }
    );

    const {
        data: roadsData,
        isLoading: roadsLoading,
        error: roadsError
    } = useCentralQuery<{ roads: Road[] }>(
        ["roads"],
        async () => {
            const response = await fetch(`${URL_OF_BACK_END}/roads`);
            if (!response.ok) {
                throw new Error(`Error fetching roads: ${response.statusText}`);
            }
            return response.json();
        },
        { enabled: mounted }
    );

    const {
        mutate: postNextMove,
        isPending: nextLoading,
        error: nextError,
        data: nextData
    } = useMutation<NextResponse, Error>({
        mutationFn: async () => {
            const response = await fetch(`${URL_OF_BACK_END}/next`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            if (!response.ok) {
                const errorData = await response.json();
                throw new Error(errorData.error || `Error: ${response.status}`);
            }
            return response.json();
        },
        onSuccess: () => {
            queryClient.invalidateQueries({ queryKey: ["cities"] });
            queryClient.invalidateQueries({ queryKey: ["settlements"] });
            queryClient.invalidateQueries({ queryKey: ["roads"] });
        }
    });

    if (!mounted) {
        return null;
    }

    const handleNext = () => {
        postNextMove();
    }

    const serverMessage = nextError ? nextError.message : nextData?.message || '';

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
                                        {portMapping[vLabel] && <Port x={v.x} y={v.y} type={portMapping[vLabel]} />}
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
                                    const [firstPart, secondPart] = road.edge.split('_');
                                    const [x1, y1] = firstPart.split('-').map(Number);
                                    const [x2, y2] = secondPart.split('-').map(Number);
                                    const colorMapping: Record<number, string> = {
                                        1: "red",
                                        2: "orange",
                                        3: "green"
                                    };
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
                            </RoadLayer>
                        </Board>
                    </BoardContainer>
                </OuterContainer>
            </QueryBoundary>
            <div style = {{ textAlign: "center", marginTop: "1rem" }}>
                <button onClick = {handleNext} disabled = {nextLoading}>
                    {nextLoading ? "Loading..." : "Next"}
                </button>
                {serverMessage && <p>{serverMessage}</p>}
            </div>
        </div>
    );
}