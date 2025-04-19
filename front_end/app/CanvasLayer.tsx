import React from 'react';
import { jsonArrayOfEdgeInformation } from './board';
import { vertices } from './board';
import { useEffect } from 'react';
import { useMemo } from 'react';
import { useRef } from 'react';


const CanvasLayer: React.FC = () => {
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const memoizedEdges = useMemo(() => jsonArrayOfEdgeInformation, [jsonArrayOfEdgeInformation]);
    const memoizedVertices = useMemo(() => vertices, [vertices]);
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) { return };
        const context = canvas.getContext('2d');
        if (!context) { return };

        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        const width = rect.width;
        const height = rect.height;
        canvas.width = width * dpr;
        canvas.height = height * dpr;
        context.scale(dpr, dpr);
        context.translate(0.5, 0.5);
        context.clearRect(0, 0, width, height);
        context.strokeStyle = 'blue';
        context.lineWidth = 1;
        const toCanvasX = (x: number) => (x / 100) * width;
        const toCanvasY = (y: number) => (y / 100) * height;
        memoizedEdges.forEach(edge => {
            context.beginPath();
            context.moveTo(toCanvasX(edge.x1), toCanvasY(edge.y1));
            context.lineTo(toCanvasX(edge.x2), toCanvasY(edge.y2));
            context.stroke();
        });
        context.fillStyle = 'red';
        context.font = '12px sans-serif';
        context.textAlign = 'center';
        context.textBaseline = 'middle';
        memoizedEdges.forEach((edge, index) => {
            const midX = (toCanvasX(edge.x1) + toCanvasX(edge.x2)) / 2;
            const midY = (toCanvasY(edge.y1) + toCanvasY(edge.y2)) / 2;
            const label = `E${(index + 1).toString().padStart(2, '0')}`;
            context.fillText(label, midX, midY);
        });
        memoizedVertices.forEach((vertex, index) => {
            const x = toCanvasX(vertex.x);
            const y = toCanvasY(vertex.y);
            context.fillStyle = 'black';
            context.beginPath();
            context.arc(x, y, 7, 0, 2 * Math.PI);
            context.fill();
            context.fillStyle = 'white';
            context.font = '7px sans-serif';
            context.textAlign = 'center';
            context.textBaseline = 'middle';
            context.fillText(`V${(index + 1).toString().padStart(2, '0')}`, x, y);
        });
    }, [memoizedEdges, memoizedVertices]);
    return (
        <canvas
            ref = {canvasRef}
            style = {{
                position: 'absolute',
                top: 0,
                left: 0,
                width: '100%',
                height: '100%',
                pointerEvents: 'none'
            }}
        />
    );
};

export default CanvasLayer;