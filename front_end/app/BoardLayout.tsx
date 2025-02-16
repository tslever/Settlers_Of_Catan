import { Edge } from './types';
import { HexInformation } from './types';
import { VertexInformation } from './types';
import styled from 'styled-components';


export const OuterContainer = styled.div`
    position: relative;
    width: 100vmin;
    height: 100vmin;
`;


export const BoardContainer = styled.div`
    width: 100vmin;
    height: 100vmin;
    position: relative;
    overflow: hidden;
`;


export const Board = styled.div`
    position: relative;
    width: 100%;
    height: 100%;
`;


export const EdgeLayer = styled.svg`
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
`;


export const RoadLayer = styled.svg`
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
`;

export function getPositionStyles(x: number, y: number): { left: string; top: string } {
    return {
        left: `${x}vmin`,
        top: `${y}vmin`
    };
}