import styled from "styled-components";


const RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON = Math.tan(Math.PI / 6);
const RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX = 2 * RATIO_OF_LENGTH_OF_SIDE_OF_HEXAGON_AND_WIDTH_OF_HEXAGON;

const OceanContainer = styled.div<{
    $oceanWidth: number;
    $oceanHeight: number;
}>`
    position: absolute;
    left: 50%;
    top: 50%;
    width: ${({ $oceanWidth }) => `${$oceanWidth}vmin`};
    height: ${({ $oceanHeight }) => `${$oceanHeight}vmin`};
    transform: translate(-50%, -50%) rotate(30deg);
    background-color: rgb(0, 150, 200);
    clip-path: polygon(50% 0%, 100% 25%, 100% 75%, 50% 100%, 0% 75%, 0% 25%);
    z-index: -1;
`;

const Ocean: React.FC = () => {
    const oceanWidth = 90;
    const oceanHeight = oceanWidth * RATIO_OF_HEIGHT_OF_HEX_AND_WIDTH_OF_HEX;
    return (
        <OceanContainer
            $oceanWidth = { oceanWidth }
            $oceanHeight = { oceanHeight }
            className = "ocean"
        />
    );
};

export default Ocean;