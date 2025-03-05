import styled from "styled-components";

type VertexProps = {
    x: number;
    y: number;
    label: string;
};

const VertexContainer = styled.div<{ x: number; y: number }>`
    position: absolute;
    left: ${({ x }) => `${x}vmin`};
    top: ${({ y }) => `${y}vmin`};
`;

const VertexLabel = styled.span``;

const Vertex: React.FC<VertexProps> = ({ x, y, label }) => {
    return (
        <VertexContainer x = {x} y = {y} className = "vertex">
            <VertexLabel className = "vertex-label">{label}</VertexLabel>
        </VertexContainer>
    );
}

export default Vertex;