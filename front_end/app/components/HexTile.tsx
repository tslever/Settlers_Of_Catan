import styled from "styled-components";
import { ID_Of_Hex } from '../types'

type HexTileProps = {
    id: ID_Of_Hex;
    token: number | null;
    color: string;
    style?: React.CSSProperties; // TODO: Consider ceasing to pass styles and keeping styles in styled components.
};

const HexTileContainer = styled.div<{ $bgColor: string }>`
    position: absolute;
    background-color: ${({ $bgColor }) => $bgColor};
`;

const HexTokenContainer = styled.div<{ $fontSize: string }>`
    font-size: ${({ $fontSize }) => $fontSize};
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
`;

const TokenNumber = styled.div``;
const TokenDots = styled.div``;
const Dot = styled.span``;

const HexTile: React.FC<HexTileProps> = ({ token, color, style }) => {
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
        <HexTileContainer $bgColor = {color} className = "hex-tile" style = {style}>
            {token !== null && (
                <HexTokenContainer
                    $fontSize = {getTokenFontSize(token)}
                    className="hex-token"
                >
                    <TokenNumber className = "token-number">{token}</TokenNumber>
                    <TokenDots className = "token-dots">
                        { Array.from(
                            { length: tokenDotMapping[token] || 0 },
                            (_, i) => <Dot key = {i} className = "dot" />
                        )}
                    </TokenDots>
                </HexTokenContainer>
            )}
        </HexTileContainer>
    );
};

export default HexTile;