const RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = Math.tan(Math.PI / 6);
const RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON;

const Ocean: React.FC = () => {
    const oceanWidth = 90;
    const oceanHeight = oceanWidth * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX;
    return (
        <div
            className = "ocean"
            style = {{
                left: '50%',
                top: '50%',
                width: `${oceanWidth}vmin`,
                height: `${oceanHeight}vmin`,
                transform: 'translate(-50%, -50%) rotate(30deg)'
            }}
        />
    );
};

export default Ocean;