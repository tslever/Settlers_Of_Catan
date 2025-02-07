import { HexID } from '../utilities/board';

type HexTileProps = {
    id: HexID;
    token: number | null;
    color: string;
    style?: React.CSSProperties;
};

const HexTile: React.FC<HexTileProps> = ({ id, token, color, style }) => {
    const getTokenFontSize = (token: number): string => {
        const baseFontSize = 4;
        const mapping: { [key: number]: number } = {
            2: 3/11,
            3: 5/11,
            4: 7/11,
            5: 9/11,
            6: 1,
            8: 1,
            9: 9/11,
            10: 7/11,
            11: 5/11,
            12: 3/11
        };
        return `${baseFontSize * (mapping[token] || 1)}vmin`;
    };
    const tokenDotMapping: { [key: number]: number } = {
        2: 1,
        3: 2,
        4: 3,
        5: 4,
        6: 5,
        8: 5,
        9: 4,
        10: 3,
        11: 2,
        12: 1
    };
    return (
        <div className = "hex-tile" style = {{ ...style, backgroundColor: color }}>
            {token !== null && (
                <div
                    className="hex-token"
                    style = {{
                        fontSize: getTokenFontSize(token),
                        display: 'flex',
                        flexDirection: 'column',
                        alignItems: 'center',
                        justifyContent: 'center'
                    }}
                >
                    <div className="token-number">{token}</div>
                    <div className="token-dots">
                        { Array.from( { length: tokenDotMapping[token] || 0 }, (_, i) => (
                            <span key = {i} className = "dot" />
                        ))}
                    </div>
                </div>
            )}
        </div>
    );
}

export default HexTile;