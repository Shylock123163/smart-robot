import type { OpenClawStatus } from '@/lib/api/openclaw';

type Props = {
  cameraOnline: boolean;
  chassisMode: string;
  gripMode: string;
  sensorHealth: string;
  pendingDeviceCommands: number | undefined;
  activeSessions: number | undefined;
  statusLine: string;
};

export function DeviceStatus({
  cameraOnline,
  chassisMode,
  gripMode,
  sensorHealth,
  pendingDeviceCommands,
  activeSessions,
  statusLine,
}: Props) {
  return (
    <section className="panel">
      <div className="panel-title">设备状态</div>
      <div className="status-grid">
        <div className="status-card">
          <span>相机链路</span>
          <strong>{cameraOnline ? '设备在线' : '等待设备'}</strong>
        </div>
        <div className="status-card">
          <span>底盘模式</span>
          <strong>{chassisMode}</strong>
        </div>
        <div className="status-card">
          <span>夹持状态</span>
          <strong>{gripMode}</strong>
        </div>
        <div className="status-card">
          <span>传感器健康</span>
          <strong>{sensorHealth}</strong>
        </div>
        <div className="status-card">
          <span>待执行命令</span>
          <strong>{pendingDeviceCommands ?? '-'}</strong>
        </div>
        <div className="status-card">
          <span>活跃会话</span>
          <strong>{activeSessions ?? '-'}</strong>
        </div>
      </div>
      <div className="status-summary">{statusLine}</div>
    </section>
  );
}
