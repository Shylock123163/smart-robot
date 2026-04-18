import { missionPresets } from '@/app/constants';

type MissionFlowItem = {
  title: string;
  state: string;
  detail: string;
};

type EnvironmentCard = {
  label: string;
  text: string;
};

type Props = {
  missionFlow: MissionFlowItem[];
  environmentCards: EnvironmentCard[];
  onSelectPreset: (task: string) => void;
};

export function MissionOpsStrip({ missionFlow, environmentCards, onSelectPreset }: Props) {
  return (
    <section className="ops-strip panel">
      <div className="ops-grid">
        <div className="ops-column">
          <div className="panel-title">任务模板</div>
          <div className="preset-list">
            {missionPresets.map((preset) => (
              <button
                className="preset-card"
                key={preset.label}
                type="button"
                onClick={() => onSelectPreset(preset.task)}
              >
                <span>{preset.label}</span>
                <strong>{preset.task}</strong>
              </button>
            ))}
          </div>
        </div>
        <div className="ops-column">
          <div className="panel-title">闭环执行链路</div>
          <div className="flow-list">
            {missionFlow.map((item) => (
              <div className="flow-card" key={item.title}>
                <span>{item.title}</span>
                <strong>{item.state}</strong>
                <p>{item.detail}</p>
              </div>
            ))}
          </div>
        </div>
        <div className="ops-column">
          <div className="panel-title">场景约束</div>
          <div className="rule-list">
            {environmentCards.map((item) => (
              <div className="rule-card" key={item.label}>
                <span>{item.label}</span>
                <strong>{item.text}</strong>
              </div>
            ))}
          </div>
        </div>
      </div>
    </section>
  );
}
