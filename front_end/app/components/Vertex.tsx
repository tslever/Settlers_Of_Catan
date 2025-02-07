type VertexProps = {
    x: number;
    y: number;
    label: string;
};

const Vertex: React.FC<VertexProps> = ({ x, y, label }) => {
    return (
        <div className="vertex" style = {{ left: `${x}vmin`, top: `${y}vmin` }}>
            <span className="vertex-label">{label}</span>
        </div>
    );
}

export default Vertex;