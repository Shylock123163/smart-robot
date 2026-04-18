import { useState, useEffect, useCallback } from 'react';
import { Canvas } from '@react-three/fiber';
import { SplashScene } from '@/components/scene/SplashScene';
import { useSplashStore } from '@/stores/splashStore';

function ErrorFallback({ onSkip }: { onSkip: () => void }) {
  return (
    <div className="splash-container">
      <div className="splash-overlay">
        <h1 className="splash-title">暗域捕手</h1>
        <p className="splash-subtitle">基于云服务器的智能终端</p>
      </div>
      <button className="splash-enter" onClick={onSkip}>
        点 击 进 入
      </button>
    </div>
  );
}

export function SplashScreen() {
  const phase = useSplashStore((s) => s.phase);
  const setPhase = useSplashStore((s) => s.setPhase);
  const setSplashDone = useSplashStore((s) => s.setSplashDone);
  const [whiteOut, setWhiteOut] = useState(false);
  const [sceneError, setSceneError] = useState(false);

  const handleEnter = useCallback(() => {
    if (phase !== 'door') return;
    setPhase('entering');
    setTimeout(() => setWhiteOut(true), 300);
    setTimeout(() => setSplashDone(), 1500);
  }, [phase, setPhase, setSplashDone]);

  const handleSkip = useCallback(() => {
    setSplashDone();
  }, [setSplashDone]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Enter' || e.key === ' ') handleEnter();
      if (e.key === 'Escape') handleSkip();
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [handleEnter, handleSkip]);

  useEffect(() => {
    const timer = setTimeout(() => {
      if (phase === 'loading') handleSkip();
    }, 8000);
    return () => clearTimeout(timer);
  }, [phase, handleSkip]);

  if (sceneError) return <ErrorFallback onSkip={handleSkip} />;

  return (
    <div className="splash-container">
      <Canvas
        className="splash-canvas"
        camera={{ fov: 50, near: 0.1, far: 200 }}
        dpr={[1, 1.5]}
        gl={{ antialias: true, powerPreference: 'default' }}
        onError={() => setSceneError(true)}
        fallback={<div style={{ width: '100%', height: '100%', background: '#001c54' }} />}
      >
        <SplashScene />
      </Canvas>

      <div className={`splash-overlay ${phase === 'flying' || phase === 'door' || phase === 'entering' ? 'fade-out' : ''}`}>
        {(phase === 'loading' || phase === 'flying') && (
          <>
            <h1 className="splash-title">暗域捕手</h1>
            <p className="splash-subtitle">基于云服务器的智能终端</p>
            {phase === 'loading' && <div className="splash-loader" />}
          </>
        )}
      </div>

      {phase === 'door' && (
        <button className="splash-enter" onClick={handleEnter}>
          点 击 进 入
        </button>
      )}

      <button className="splash-skip" onClick={handleSkip}>
        跳过
      </button>

      <div className={`splash-white ${whiteOut ? 'active' : ''}`} />
    </div>
  );
}
