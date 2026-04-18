type Props = {
  eventLog: string[];
};

export function EventLog({ eventLog }: Props) {
  return (
    <section className="panel">
      <div className="panel-title">任务日志</div>
      <div className="log-list">
        {eventLog.map((line, i) => (
          <div className="log-line" key={`${line}-${i}`}>
            {line}
          </div>
        ))}
      </div>
    </section>
  );
}
