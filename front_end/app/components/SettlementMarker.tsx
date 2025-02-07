type SettlementMarkerProps = {
    x: number;
    y: number;
    player: number;
};

const SettlementMarker: React.FC<SettlementMarkerProps> = ({ x, y, player }) => {
    const colorMapping: { [key: number]: string } = {
        1: 'red',
        2: 'orange',
        3: 'green'
    };
    return (
        <div
            className = "settlement"
            style = {{
                left: `${x}vmin`,
                top: `${y}vmin`,
                backgroundColor: colorMapping[player] || 'gray'
            }}
        />
    );
};

export default SettlementMarker;