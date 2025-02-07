type PortProps = {
    x: number;
    y: number;
    type: string;
};

const Port: React.FC<PortProps> = ({ x, y, type }) => {
    return (
        <div className = "port" style = {{ left: `${x}vmin`, top: `${y}vmin` }}>
            <span className="port-label">{type}</span>
        </div>
    );
};

export default Port;