type CityMarkerProps = {
    x: number;
    y: number;
    player: number;
};

const CityMarker: React.FC<CityMarkerProps> = ({ x, y, player }) => {
    const colorMapping: { [key: number]: string } = {
        1: 'red',
        2: 'orange',
        3: 'green'
    };
    return (
        <div
            className = "city"
            style = {{
                left: `${x}vmin`,
                top: `${y}vmin`,
                position: 'absolute',
                transform: 'translate(-50%, -50%)',
                width: '4vmin',
                height: '4vmin',
                borderRadius: '50%',
                border: `2px solid ${colorMapping[player] || 'gray'}`,
                backgroundColor: 'white',
                zIndex: 3
            }}
        />
    );
};

export default CityMarker;