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
import SettlementMarker from './components/SettlementMarker';
import { URL_OF_BACK_END } from './config';
import { getPositionStyles } from './BoardLayout';
import { hexes } from './board';
import { idToColor } from './types';
import { tokenMapping } from './types';
import { vertices } from './board';
import { useQuery } from '@tanstack/react-query';
import { useMutation } from '@tanstack/react-query';
import { useQueryClient } from '@tanstack/react-query';
import { useEffect } from 'react';
import { useState } from 'react';
import CanvasLayer from './CanvasLayer';


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
    } = useQuery<{ settlements: Settlement[] }>({
        queryKey: ["settlements"],
        queryFn: async () => {
            const res = await fetch(`${URL_OF_BACK_END}/settlements`);
            if (!res.ok) {
                throw new Error(`Error fetching settlements: ${res.statusText}`);
            }
            return res.json();
        },
        enabled: mounted
    });

    const {
        data: roadsData,
        isLoading: roadsLoading,
        error: roadsError
    } = useQuery<{ roads: Road[] }>({
        queryKey: ["roads"],
        queryFn: async() => {
            const res = await fetch(`${URL_OF_BACK_END}/roads`);
            if (!res.ok) {
                throw new Error(`Error fetching roads: ${res.statusText}`);
            }
            return res.json();
        },
        enabled: mounted
    });

    const {
        mutate: postNextMove,
        isPending: nextLoading,
        error: nextError,
        data: nextData
    } = useMutation<NextResponse, Error>({
        mutationFn: async () => {
            const res = await fetch(`${URL_OF_BACK_END}/next`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            if (!res.ok) {
                const errorData = await res.json();
                throw new Error(errorData.error || `Error: ${res.status}`);
            }
            return res.json();
        },
        onSuccess: () => {
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

    return (
        <div>
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
                            if (!v) return null;
                            return <SettlementMarker key = {s.id} x = {v.x} y = {v.y} player = {s.player} />
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
            <div style = {{ textAlign: "center", marginTop: "1rem" }}>
                <button onClick = {handleNext} disabled = {nextLoading}>
                    {nextLoading ? "Loading..." : "Next"}
                </button>
                {serverMessage && <p>{serverMessage}</p>}
                {(settlementsError || roadsError) && (
                    <p style = {{ color: "red" }}>
                        {(settlementsError as Error)?.message || (roadsError as Error)?.message}
                    </p>
                )}
            </div>
        </div>
    );
}