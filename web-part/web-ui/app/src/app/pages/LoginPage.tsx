import { useState } from 'react';
import '@/styles/login.css';

export function LoginPage() {
  const [flipped, setFlipped] = useState(false);

  return (
    <div className="login-page">
      <div className={`login-wrap ${flipped ? 'flipped' : ''}`}>
        <form className="login-form" onSubmit={(e) => e.preventDefault()}>
          <h1>登录</h1>
          <input className="login-input" type="text" placeholder="用户名" />
          <input className="login-input" type="password" placeholder="密码" />
          <button className="login-submit" type="submit">LOGIN</button>
          <div className="login-toggle">
            还没有账号？<a onClick={() => setFlipped(true)}>注册</a>
          </div>
        </form>
        <form className="register-form" onSubmit={(e) => e.preventDefault()}>
          <h1>注册</h1>
          <input className="login-input" type="text" placeholder="用户名" />
          <input className="login-input" type="password" placeholder="密码" />
          <input className="login-input" type="password" placeholder="确认密码" />
          <button className="login-submit" type="submit">REGISTER</button>
          <div className="login-toggle">
            已有账号？<a onClick={() => setFlipped(false)}>登录</a>
          </div>
        </form>
      </div>
    </div>
  );
}
