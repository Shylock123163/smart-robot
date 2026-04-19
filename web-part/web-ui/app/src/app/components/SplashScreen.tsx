import { useState, useEffect, useCallback, useRef } from 'react';
import { useSplashStore } from '@/stores/splashStore';

function playAudio(src: string, volume = 0.5) {
  const audio = new Audio(src);
  audio.volume = volume;
  audio.play().catch(() => {});
  return audio;
}

function SplashParticles() {
  const count = 40;
  return (
    <div className="splash-particles">
      {Array.from({ length: count }, (_, i) => (
        <div
          key={i}
          className="splash-particle"
          style={{
            left: `${Math.random() * 100}%`,
            animationDelay: `${Math.random() * 6}s`,
            animationDuration: `${4 + Math.random() * 4}s`,
            width: `${2 + Math.random() * 4}px`,
            height: `${2 + Math.random() * 4}px`,
            opacity: 0.3 + Math.random() * 0.5,
          }}
        />
      ))}
    </div>
  );
}

export function SplashScreen() {
  const phase = useSplashStore((s) => s.phase);
  const setPhase = useSplashStore((s) => s.setPhase);
  const setSplashDone = useSplashStore((s) => s.setSplashDone);
  const [whiteOut, setWhiteOut] = useState(false);
  const [doorOpen, setDoorOpen] = useState(false);
  const [lightBurst, setLightBurst] = useState(false);
  const bgmRef = useRef<HTMLAudioElement | null>(null);
  const base = import.meta.env.BASE_URL;

  const handleEnter = useCallback(() => {
    setPhase('entering');
    playAudio(`${base}genshin/Genshin Impact [Duang].mp3`, 0.4);
    setLightBurst(true);

    setTimeout(() => {
      setDoorOpen(true);
      playAudio(`${base}genshin/Genshin Impact [DoorComeout].mp3`, 0.5);
    }, 400);

    setTimeout(() => {
      playAudio(`${base}genshin/Genshin Impact [DoorThrough].mp3`, 0.6);
    }, 1200);

    setTimeout(() => setWhiteOut(true), 1800);

    setTimeout(() => {
      if (bgmRef.current) {
        bgmRef.current.pause();
        bgmRef.current = null;
      }
      setSplashDone();
    }, 2800);
  }, [setPhase, setSplashDone, base]);

  const handleSkip = useCallback(() => {
    if (bgmRef.current) {
      bgmRef.current.pause();
      bgmRef.current = null;
    }
    setSplashDone();
  }, [setSplashDone]);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (phase === 'door' && (e.key === 'Enter' || e.key === ' ')) handleEnter();
      if (e.key === 'Escape') handleSkip();
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [phase, handleEnter, handleSkip]);

  useEffect(() => {
    if (phase !== 'loading') return;
    const timer = setTimeout(() => setPhase('door'), 2500);
    return () => clearTimeout(timer);
  }, [phase, setPhase]);

  useEffect(() => {
    if (phase !== 'door') return;
    bgmRef.current = playAudio(`${base}genshin/BGM.mp3`, 0.3);
    if (bgmRef.current) bgmRef.current.loop = true;
    return () => {
      if (bgmRef.current) {
        bgmRef.current.pause();
      }
    };
  }, [phase, base]);

  useEffect(() => {
    if (phase !== 'door') return;
    const timer = setTimeout(() => handleEnter(), 6000);
    return () => clearTimeout(timer);
  }, [phase, handleEnter]);

  return (
    <div className="splash-container">
      <div className="splash-bg-genshin" />

      <SplashParticles />

      {lightBurst && <div className="splash-light-burst" />}

      <div className={`splash-door splash-door-left ${doorOpen ? 'open' : ''}`}>
        <div className="splash-door-pattern" />
        <div className="splash-door-edge" />
      </div>
      <div className={`splash-door splash-door-right ${doorOpen ? 'open' : ''}`}>
        <div className="splash-door-pattern" />
        <div className="splash-door-edge left" />
      </div>

      <div className="splash-door-glow" />

      <div className="splash-overlay-genshin">
        <div className={`splash-logo-group ${phase === 'entering' ? 'fade-out' : ''}`}>
          <div className="splash-logo-diamond">
            <div className="splash-diamond-inner" />
          </div>
          <h1 className="splash-title-genshin">暗域捕手</h1>
          <p className="splash-subtitle-genshin">Dark Zone Catcher</p>
          <div className="splash-divider" />
          <p className="splash-tagline">基于云服务器的智能终端</p>
        </div>

        {phase === 'loading' && (
          <div className="splash-loading-bar">
            <div className="splash-loading-fill" />
          </div>
        )}

        {phase === 'door' && (
          <button className="splash-click-enter" onClick={handleEnter}>
            <span className="splash-click-icon" />
            <span>点击任意位置进入</span>
          </button>
        )}
      </div>

      <button className="splash-skip" onClick={handleSkip}>
        跳过
      </button>

      <div className={`splash-white ${whiteOut ? 'active' : ''}`} />
    </div>
  );
}
