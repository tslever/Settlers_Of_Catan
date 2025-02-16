import React from 'react';
import { edges } from './board';
import { vertices } from './board';
import { useEffect } from 'react';
import { useMemo } from 'react';
import { useRef } from 'react';


const CanvasLayer: React.FC = () => {
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const memoizedEdges = useMemo(() => edges, [edges]);
    const memoizedVertices = useMemo(() => vertices, [vertices]);
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) { return };
        const context = canvas.getContext('2d');
        if (!context) { return };
        const width = canvas.clientWidth;
        const height = canvas.clientHeight;
        canvas.width = width;
        canvas.height = height;
        context.clearRect(0, 0, width, height);
        const toCanvasX = (x: number) => (x / 100) * width;
        const toCanvasY = (y: number) => (y / 100) * height;
        context.strokeStyle = 'blue';
        context.lineWidth = 0.5;
        memoizedEdges.forEach(edge => {
            context.beginPath();
            context.moveTo(toCanvasX(edge.x1), toCanvasY(edge.y1));
            context.lineTo(toCanvasX(edge.x2), toCanvasY(edge.y2));
            context.stroke();
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