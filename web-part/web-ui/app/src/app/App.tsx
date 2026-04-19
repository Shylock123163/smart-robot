import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { Navbar } from '@/app/components/Navbar';
import { FootNav } from '@/app/components/FootNav';
import { SplashScreen } from '@/app/components/SplashScreen';
import { FloatingParticles } from '@/app/components/FloatingParticles';
import { HomePage } from '@/app/pages/HomePage';
import { MonitorPage } from '@/app/pages/MonitorPage';
import { ChatPage } from '@/app/pages/ChatPage';
import { LoginPage } from '@/app/pages/LoginPage';
import { AboutPage } from '@/app/pages/AboutPage';
import { useSplashStore } from '@/stores/splashStore';

export function App() {
  const splashDone = useSplashStore((s) => s.done);

  if (!splashDone) {
    return <SplashScreen />;
  }

  return (
    <BrowserRouter basename="/sr">
      <FloatingParticles />
      <Navbar />
      <Routes>
        <Route path="/" element={<HomePage />} />
        <Route path="/monitor" element={<MonitorPage />} />
        <Route path="/chat" element={<ChatPage />} />
        <Route path="/login" element={<LoginPage />} />
        <Route path="/about" element={<AboutPage />} />
      </Routes>
      <FootNav />
    </BrowserRouter>
  );
}
