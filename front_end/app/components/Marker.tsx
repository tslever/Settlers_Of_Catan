import React from "react";
import CityMarker from "./CityMarker";
import SettlementMarker from "./SettlementMarker";

type MarkerProps = {
    x: number;
    y: number;
    player: number;
    type: 'city' | 'settlement';
}

const Marker: React.FC<MarkerProps> = ({ x, y, player, type }) => {
    if (type === 'city') {
        return <CityMarker x = {x} y = {y} player = {player} />
    } else {
        return <SettlementMarker x = {x} y = {y} player = {player} />
    }
};

export default Marker;