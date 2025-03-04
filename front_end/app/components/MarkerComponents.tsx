import React from 'react';
import styled from 'styled-components';

export const colorMapping: { [key: number]: string } = {
    1: 'red',
    2: 'orange',
    3: 'green'
}

type MarkerContainerProps = {
    x: number;
    y: number;
    $customTransform?: string;
}

export const MarkerContainer = styled.div<MarkerContainerProps>`
    position: absolute;
    left: ${({ x }) => `${x}vmin`};
    top: ${({ y }) => `${y}vmin`};
    transform: ${({ $customTransform }) => $customTransform || `translate(-50%, -50%)`};
`;

const SettlementPart = styled.div<{ $bgColor: string }>`
    width: 3vmin;
    height: 3vmin;
    background-color: ${({ $bgColor }) => $bgColor};
    clip-path: polygon(50% 0%, 100% 40%, 100% 100%, 0% 100%, 0% 40%);
`;

const BasePart = styled.div<{ $bgColor: string }>`
    margin-top: 0.2vmin;
    width: 6vmin;
    height: 3vmin;
    background-color: ${({ $bgColor }) => $bgColor};
    transform: translate(25%, -10%);
`;

export const CityMarker: React.FC<{ x: number; y: number; player: number }> = ({ x, y, player }) => {
    const color = colorMapping[player] || 'gray';
    return (
        <MarkerContainer x = {x} y = {y} $customTransform = "translate(-75%, -75%)" style = {{ display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
            <SettlementPart $bgColor = {color} />
            <BasePart $bgColor = {color} />
        </MarkerContainer>
    );
};

export const SettlementMarker: React.FC<{ x: number; y: number; player: number }> = ({ x, y, player }) => {
    const color = colorMapping[player] || 'gray';
    return (
        <MarkerContainer
            x = {x}
            y = {y}
            style = {{
                backgroundColor: color,
                width: '3vmin',
                height: '3vmin',
                clipPath: 'polygon(50% 0%, 100% 40%, 100% 100%, 0% 100%, 0% 40%)'
            }}
        />
    );
};