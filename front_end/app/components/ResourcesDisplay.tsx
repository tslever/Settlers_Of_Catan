import React from 'react';

type ResourcesByKind = Record<'brick' | 'grain' | 'lumber' | 'ore' | 'wool' | 'cloth' | 'coin' | 'paper', number>;
type Totals  = Record<string, ResourcesByKind>;

interface Props {
  totals: Totals;
  gained: Totals;
}

export const ZERO_BAG: ResourcesByKind = {
  brick: 0,
  grain: 0,
  lumber: 0,
  ore: 0,
  wool: 0,
  cloth: 0,
  coin: 0,
  paper: 0
};

const ResourcesDisplay: React.FC<Props> = ({ totals, gained }) => (
  <div style={{ margin: '1rem 0', padding: '0.5rem', border: '1px solid #ccc' }}>
    <h3>Gained resources this roll</h3>
    <table>
      <thead>
        <tr>
          <th>Player</th>
          <th>Brick</th>
          <th>Grain</th>
          <th>Lumber</th>
          <th>Ore</th>
          <th>Wool</th>
          <th>Cloth</th>
          <th>Coin</th>
          <th>Paper</th>
        </tr>
      </thead>
      <tbody>
        {Object.entries(totals).map(([playerLabel]) => {
          const bag = gained[playerLabel] ?? ZERO_BAG;
          return (
            <tr key={playerLabel}>
              <td>{playerLabel}</td>
              <td>{bag.brick}</td>
              <td>{bag.grain}</td>
              <td>{bag.lumber}</td>
              <td>{bag.ore}</td>
              <td>{bag.wool}</td>
              <td>{bag.cloth}</td>
              <td>{bag.coin}</td>
              <td>{bag.paper}</td>
            </tr>
          );
        })}
      </tbody>
    </table>

    <h3>Total resources</h3>
    <table>
      <thead>
        <tr>
          <th>Player</th>
          <th>Brick</th>
          <th>Grain</th>
          <th>Lumber</th>
          <th>Ore</th>
          <th>Wool</th>
          <th>Cloth</th>
          <th>Coin</th>
          <th>Paper</th>
        </tr>
      </thead>
      <tbody>
        {Object.entries(totals).map(([playerLabel, bag]) => (
          <tr key={playerLabel}>
            <td>{playerLabel}</td>
            <td>{bag.brick}</td>
            <td>{bag.grain}</td>
            <td>{bag.lumber}</td>
            <td>{bag.ore}</td>
            <td>{bag.wool}</td>
            <td>{bag.cloth}</td>
            <td>{bag.coin}</td>
            <td>{bag.paper}</td>
          </tr>
        ))}
      </tbody>
    </table>
  </div>
);

export default ResourcesDisplay;