import { styled } from 'styled-components';

type CityMarkerProps = {
    x: number;
    y: number;
    player: number;
};

const colorMapping: { [key: number]: string } = {
    1: 'red',
    2: 'orange',
    3: 'green'
};

const CityContainer = styled.div`
    position: absolute;
    transform: translate(-75%, -50%);
    display: flex;
    flex-direction: column;
    align-items: center;
`;

const SettlementPart = styled.div<{ $bgColor: string }>`
    width: 3vmin;
    height: 3vmin;
    background-color: ${(props) => props.$bgColor};
    clip-path: polygon(50% 0%, 100% 40%, 100% 100%, 0% 100%, 0% 40%);
`;

const BasePart = styled.div<{ $bgColor: string }>`
    margin-top: 0.2vmin;
    width: 6vmin;
    height: 3vmin;
    background-color: ${(props) => props.$bgColor};
    transform: translate(25%, -10%);
`;

const CityMarker: React.FC<CityMarkerProps> = ({ x, y, player }) => {
    const playerColor = colorMapping[player] || 'gray';
    return (
        <CityContainer style = {{ left: `${x}vmin`, top: `${y}vmin` }}>
            <SettlementPart $bgColor = {playerColor} />
            <BasePart $bgColor = {playerColor} />
        </CityContainer>
    );
};

export default CityMarker;