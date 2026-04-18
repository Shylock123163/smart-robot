import { Component, lazy, Suspense, useEffect, useState, type ReactNode } from 'react';

const RobotScene = lazy(() =>
  import('@/components/scene/RobotScene').then((module) => ({ default: module.RobotScene }))
);

class SceneErrorBoundary extends Component<{ children: ReactNode }, { hasError: boolean }> {
  constructor(props: { children: ReactNode }) {
    super(props);
    this.state = { hasError: false };
  }

  static getDerivedStateFromError() {
    return { hasError: true };
  }

  componentDidCatch(error: unknown) {
    console.error('RobotScene failed to render:', error);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="scene-fallback-card">
          <strong>3D 主视图区已切到安全模式</strong>
          <span>当前设备或浏览器 WebGL 渲染不稳定，页面主体功能不受影响。</span>
          <p>你仍然可以继续使用任务输入、OpenClaw 对话、状态查看和手动控制区。</p>
        </div>
      );
    }
    return this.props.children;
  }
}

export function ScenePanel() {
  const [sceneMounted, setSceneMounted] = useState(false);

  useEffect(() => {
    const timer = window.setTimeout(() => setSceneMounted(true), 250);
    return () => window.clearTimeout(timer);
  }, []);

  return (
    <section className="scene-panel panel">
      <div className="panel-title">3D 主视图区</div>
      <div className="panel-subtitle">
        react-three-fiber + drei + postprocessing 承载设备姿态与后续数字孪生。
      </div>
      <div className="scene-wrap">
        <SceneErrorBoundary>
          {sceneMounted ? (
            <Suspense fallback={<div className="scene-loading">正在加载机器人 3D 主视图区…</div>}>
              <RobotScene />
            </Suspense>
          ) : (
            <div className="scene-fallback-card">
              <strong>3D 主视图区准备中</strong>
              <span>页面主功能已先加载，3D 区块会在空闲时延后挂载，避免整页卡死。</span>
              <p>如果当前设备性能较弱，这个区域会自动保持轻量模式。</p>
            </div>
          )}
        </SceneErrorBoundary>
      </div>
    </section>
  );
}
