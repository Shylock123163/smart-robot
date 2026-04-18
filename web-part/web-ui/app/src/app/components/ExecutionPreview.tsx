import type { ResponseMeta } from '@/app/types';

type Props = {
  responseMeta: ResponseMeta;
};

export function ExecutionPreview({ responseMeta }: Props) {
  return (
    <section className="panel">
      <div className="panel-title">执行预览</div>
      <div className="plan-grid">
        <div className="plan-card">
          <span>回复来源</span>
          <strong>{responseMeta.aiSource}</strong>
        </div>
        <div className="plan-card">
          <span>策略原因</span>
          <strong>{responseMeta.reason}</strong>
        </div>
        <div className="plan-card">
          <span>当前形态</span>
          <strong>{responseMeta.fallback ? 'fallback' : '正常链路'}</strong>
        </div>
      </div>
      <div className="command-section">
        <div className="command-title">设备命令预览</div>
        {responseMeta.commands.length ? (
          <div className="command-list">
            {responseMeta.commands.map((command, i) => (
              <code key={`${command}-${i}`}>{command}</code>
            ))}
          </div>
        ) : (
          <div className="plan-empty">当前还没有设备命令预览，先发送一条真实任务。</div>
        )}
      </div>
      <div className="memory-section">
        <div className="command-title">记忆注记</div>
        {responseMeta.memoryNotes.length ? (
          <div className="memory-list">
            {responseMeta.memoryNotes.map((note, index) => (
              <div className="memory-item" key={`${note}-${index}`}>{note}</div>
            ))}
          </div>
        ) : (
          <div className="plan-empty">当前会话还没有新增记忆注记。</div>
        )}
      </div>
    </section>
  );
}
