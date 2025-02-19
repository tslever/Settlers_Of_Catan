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

    const settlementWidth = 3;
    const baseWidth = settlementWidth * 2;
    const baseHeight = settlementWidth;

    return (
        <div
            className = "city"
            style = {{
                position: 'absolute',
                left: `${x}vmin`,
                top: `${y}vmin`,
                transform: 'translate(-50%, -100%)',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center'
            }}
        >
            {/* Settlement part */}
            <div
                style = {{
                    width: `${settlementWidth}vmin`,
                    height: `${settlementWidth}vmin`,
                    backgroundColor: colorMapping[player] || 'gray',
                    clipPath: 'polygon(50% 0%, 100% 40%, 100% 100%, 0% 100%, 0% 40%'
                }}
            />
            {/* Base part */}
            <div
                style = {{
                    marginTop: '0.2vmin',
                    width: `${baseWidth}vmin`,
                    height: `${baseHeight}vmin`,
                    backgroundColor: colorMapping[player] || 'gray',
                    border: '1px solid black'
                }}
            />
        </div>
    );
};

export default CityMarker;