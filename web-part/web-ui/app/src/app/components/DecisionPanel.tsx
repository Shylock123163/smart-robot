type Props = {
  openClawRole: string;
  nextAction: string;
  missionStatement: string;
  connectionSummary: string;
};

export function DecisionPanel({ openClawRole, nextAction, missionStatement, connectionSummary }: Props) {
  return (
    <section className="panel">
      <div className="panel-title">决策解释</div>
      <ul className="decision-list">
        <li>
          <span>角色定位</span>
          <strong>{openClawRole}</strong>
        </li>
        <li>
          <span>下一动作</span>
          <strong>{nextAction}</strong>
        </li>
        <li>
          <span>系统目标</span>
          <strong>{missionStatement}</strong>
        </li>
        <li>
          <span>服务链路</span>
          <strong>{connectionSummary}</strong>
        </li>
      </ul>
    </section>
  );
}
