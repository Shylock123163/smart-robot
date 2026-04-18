import { useRef } from 'react';
import { useFrame } from '@react-three/fiber';
import * as THREE from 'three';

function Floor() {
  return (
    <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, 0, 0]} receiveShadow>
      <planeGeometry args={[10, 8]} />
      <meshStandardMaterial color="#1a1f2e" />
    </mesh>
  );
}

function Walls() {
  const wallColor = '#2a3040';
  const wallH = 0.15;
  const wallThick = 0.12;
  return (
    <group>
      <mesh position={[0, wallH / 2, -4]}>
        <boxGeometry args={[10, wallH, wallThick]} />
        <meshStandardMaterial color={wallColor} />
      </mesh>
      <mesh position={[0, wallH / 2, 4]}>
        <boxGeometry args={[10, wallH, wallThick]} />
        <meshStandardMaterial color={wallColor} />
      </mesh>
      <mesh position={[-5, wallH / 2, 0]}>
        <boxGeometry args={[wallThick, wallH, 8]} />
        <meshStandardMaterial color={wallColor} />
      </mesh>
      <mesh position={[5, wallH / 2, 0]}>
        <boxGeometry args={[wallThick, wallH, 8]} />
        <meshStandardMaterial color={wallColor} />
      </mesh>
    </group>
  );
}

function Sofa() {
  return (
    <group position={[-3.2, 0, -2.8]}>
      <mesh position={[0, 0.2, 0]}>
        <boxGeometry args={[2.2, 0.4, 0.9]} />
        <meshStandardMaterial color="#4a6fa5" />
      </mesh>
      <mesh position={[0, 0.45, -0.35]}>
        <boxGeometry args={[2.2, 0.5, 0.2]} />
        <meshStandardMaterial color="#3d5f8f" />
      </mesh>
    </group>
  );
}

function Bed() {
  return (
    <group position={[3, 0, -2]}>
      <mesh position={[0, 0.15, 0]}>
        <boxGeometry args={[2, 0.3, 2.5]} />
        <meshStandardMaterial color="#5c4a3a" />
      </mesh>
      <mesh position={[0, 0.35, -1.1]}>
        <boxGeometry args={[2, 0.4, 0.15]} />
        <meshStandardMaterial color="#4a3a2a" />
      </mesh>
    </group>
  );
}

function Cabinet() {
  return (
    <group position={[3.8, 0, 2.5]}>
      <mesh position={[0, 0.35, 0]}>
        <boxGeometry args={[1.2, 0.7, 0.5]} />
        <meshStandardMaterial color="#6b5b4a" />
      </mesh>
    </group>
  );
}

function Table() {
  return (
    <group position={[-1, 0, 1.5]}>
      <mesh position={[0, 0.3, 0]}>
        <boxGeometry args={[1.4, 0.05, 0.8]} />
        <meshStandardMaterial color="#3a4a5a" />
      </mesh>
      {[[-0.6, -0.3], [0.6, -0.3], [-0.6, 0.3], [0.6, 0.3]].map(([x, z], i) => (
        <mesh key={i} position={[x, 0.15, z]}>
          <boxGeometry args={[0.05, 0.28, 0.05]} />
          <meshStandardMaterial color="#2a3a4a" />
        </mesh>
      ))}
    </group>
  );
}

function Robot() {
  const ref = useRef<THREE.Group>(null);

  useFrame((state) => {
    if (!ref.current) return;
    const t = state.clock.elapsedTime;
    ref.current.position.x = Math.sin(t * 0.3) * 1.5;
    ref.current.position.z = Math.cos(t * 0.3) * 1.2 + 0.5;
    ref.current.rotation.y = -t * 0.3 + Math.PI / 2;
  });

  return (
    <group ref={ref} position={[0, 0.08, 0]}>
      <mesh position={[0, 0.06, 0]}>
        <boxGeometry args={[0.35, 0.12, 0.5]} />
        <meshStandardMaterial color="#00e5ff" emissive="#00e5ff" emissiveIntensity={0.5} />
      </mesh>
      <pointLight color="#00e5ff" intensity={2} distance={2} position={[0, 0.2, 0]} />
    </group>
  );
}

function GridOverlay() {
  return (
    <gridHelper
      args={[10, 20, '#1a3050', '#1a3050']}
      position={[0, 0.005, 0]}
    />
  );
}

export function SlamScene() {
  return (
    <>
      <ambientLight color="#4488cc" intensity={0.6} />
      <directionalLight color="#ffffff" intensity={1} position={[3, 8, 4]} />
      <Floor />
      <Walls />
      <GridOverlay />
      <Sofa />
      <Bed />
      <Cabinet />
      <Table />
      <Robot />
    </>
  );
}