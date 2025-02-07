const Ocean: React.FC = () => {
    const oceanWidth = 90;
    const oceanHeight = oceanWidth * 1.1547;
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