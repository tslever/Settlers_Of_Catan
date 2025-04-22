import React from 'react';
import styled from 'styled-components';
import { colorMapping } from './MarkerComponents';

type WallProps = {
    x: number;
    y: number;
    player: number;
};

const WallRect = styled.div<{ x: number; y: number; $color: string }>`
    position: absolute;
    left: ${({ x }) => `${x}vmin`};
    top: ${({ y }) => `${y + 2}vmin`};
    width: 10vmin;
    height: 1.5vmin;
    background-color: ${({ $color }) => $color};
    transform: translate(-50%, -20%);
`;

const Wall: React.FC<WallProps> = ({ x, y, player }) => {
    const color = colorMapping[player] || 'gray';
    return <WallRect x = {x} y = {y} $color = {color} />;
};

export default Wall;