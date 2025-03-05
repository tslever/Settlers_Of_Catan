import styled from "styled-components";

type PortProps = {
    x: number;
    y: number;
    type: string;
};

const PortContainer = styled.div<{x: number; y: number}>`
    position: absolute;
    left: ${({ x }) => `${x}vmin`};
    top: ${({ y }) => `${y}vmin`};
`;

const Port: React.FC<PortProps> = ({ x, y, type }) => {
    return (
        <PortContainer x = {x} y = {y} className = "port">
            <span className="port-label">{type}</span>
        </PortContainer>
    );
};

export default Port;