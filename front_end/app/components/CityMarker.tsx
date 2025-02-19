type CityMarkerProps = {
    x: number;
    y: number;
    player: number;
};

const CityMarker: React.FC<CityMarkerProps> = ({ x, y, player }) => {
    const settlementWidth = 3;
    const cityRectWidth = settlementWidth * 2;
    const cityRectHeight = settlementWidth * 0.6;
    const colorMapping: { [key: number]: string } = {
        1: 'red',
        2: 'orange',
        3: 'green'
    };
    return (
        <div
            style = {{
                position: 'absolute',
                left: `${x}vmin`,
                top: `${y}vmin`,
                width: `${cityRectWidth}vmin`,
                height: `${settlementWidth}vmin`,
                transform: 'translate(-50%, -50%)',
            }}
        >
            {/* Draw the "settlement shape (e.g., house like) on top." */}
            <div
                className = "settlement"
                style = {{
                    position: 'absolute',
                    left: '50%',
                    top: '0%',
                    transform: 'translate(-50%, 0%)',
                    width: `${settlementWidth}vmin`,
                    height: `${settlementWidth}vmin`,
                    backgroundColor: colorMapping[player] || 'gray'
                }}
            />
            {/* Draw the rectangle under the settlement. */}
            <div
                style = {{
                    position: 'absolute',
                    left: '50%',
                    bottom: '0%',
                    transform: 'translate(-50%, 0%)',
                    width: `${cityRectWidth}vmin`,
                    height: `${cityRectHeight}vmin`,
                    backgroundColor: colorMapping[player] ? `${colorMapping[player]}AA` : 'gray',
                    border: '1 px solid black',
                    zIndex: -1
                }}
            />
        </div>
    );
};

export default CityMarker;