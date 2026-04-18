import { Suspense, lazy } from 'react';
import { Canvas } from '@react-three/fiber';
import { OrbitControls } from '@react-three/drei';

const SlamScene = lazy(() =>
  import('@/components/scene/SlamScene').then((m) => ({ default: m.SlamScene }))
);

export function SlamMapPanel() {
  return (
    <div style={{ width: '100%', height: '100%', position: 'relative' }}>
      <div className="camera-label" style={{ position: 'absolute', top: 0, left: 0, zIndex: 1 }}>
        SLAM 室内平面图
      </div>
      <Canvas
        camera={{ position: [0, 8, 6], fov: 50 }}
        dpr={[1, 1.5]}
        style={{ background: '#0a0e14' }}
      >
        <Suspense fallback={null}>
          <SlamScene />
        </Suspense>
        <OrbitControls
          maxPolarAngle={Math.PI / 2.2}
          minDistance={4}
          maxDistance={18}
          enablePan
        />
      </Canvas>
    </div>
  );
}
