import { useState, useEffect, useCallback } from 'react';
import { useSplashStore } from '@/stores/splashStore';

export function SplashScreen() {
  const phase = useSplashStore((s) => s.phase);
  const setPhase = useSplashStore((s) => s.setPhase);
  const setSplashDone = useSplashStore((s) => s.setSplashDone);
  const [whiteOut, setWhiteOut] = useState(false);
  const bgUrl = `${import.meta.env.BASE_URL}robot.jpg`;

  const handleEnter = useCallback(() => {
    setPhase('entering');
    setTimeout(() => setWhiteOut(true), 300);
    setTimeout(() => setSplashDone(), 1200);
  }, [setPhase, setSplashDone]);

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

  // Auto transition: loading -> ready after 2s
  useEffect(() => {
    if (phase !== 'loading') return;
    const timer = setTimeout(() => setPhase('door'), 2000);
    return () => clearTimeout(timer);
  }, [phase, setPhase]);

  // Auto enter after 4s on door phase
  useEffect(() => {
    if (phase !== 'door') return;
    const timer = setTimeout(() => handleEnter(), 4000);
    return () => clearTimeout(timer);
  }, [phase, handleEnter]);

  return (
    <div className="splash-container">
      <div className="splash-bg" style={{ backgroundImage: `url(${bgUrl})` }} />
      <div className="splash-gradient" />

      <div className="splash-overlay">
        <h1 className="splash-title">暗域捕手</h1>
        <p className="splash-subtitle">基于云服务器的智能终端</p>
        {phase === 'loading' && <div className="splash-loader" />}
        {phase === 'door' && (
          <button className="splash-enter-inline" onClick={handleEnter}>
            进 入 系 统
          </button>
        )}
        {phase === 'entering' && (
          <p className="splash-hint">正在加载界面…</p>
        )}
      </div>

      <button className="splash-skip" onClick={handleSkip}>
        跳过
      </button>

      <div className={`splash-white ${whiteOut ? 'active' : ''}`} />
    </div>
  );
}
