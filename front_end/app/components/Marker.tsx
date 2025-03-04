import React from "react";
import { CityMarker, SettlementMarker } from "./MarkerComponents";

type MarkerProps = {
    x: number;
    y: number;
    player: number;
    type: 'city' | 'settlement';
};

const Marker: React.FC<MarkerProps> = ({ x, y, player, type }) => {
    return type === 'city'
        ? <CityMarker x = {x} y = {y} player = {player} />
        : <SettlementMarker x = {x} y = {y} player = {player} />;
};

export default Marker;