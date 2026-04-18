const http = require('http');

const PORT = Number(process.env.OPENCLAW_PORT || 9012);
const HOST = process.env.OPENCLAW_HOST || '127.0.0.1';
const DEFAULT_PROVIDER = process.env.OPENCLAW_PROVIDER || 'openai';
const DEFAULT_ENDPOINT = process.env.OPENCLAW_ENDPOINT || 'https://api.openai.com/v1';
const DEFAULT_MODEL = process.env.OPENCLAW_MODEL || 'gpt-5.4';
const SERVER_API_KEY = process.env.OPENCLAW_API_KEY || '';
const EXECUTOR_MODE = process.env.OPENCLAW_EXECUTOR_MODE || 'queue';
const DEVICE_API_URL = process.env.OPENCLAW_DEVICE_API_URL || '';
const DEVICE_API_TOKEN = process.env.OPENCLAW_DEVICE_API_TOKEN || '';
const DEVICE_ID = process.env.OPENCLAW_DEVICE_ID || 'sr-robot-01';
const DEVICE_TOKEN = process.env.OPENCLAW_DEVICE_TOKEN || '';
const DEFAULT_CHAT_MODE = process.env.OPENCLAW_DEFAULT_CHAT_MODE === 'control' ? 'control' : 'assistant';
const AUTO_EXECUTE = ['1', 'true', 'yes', 'on'].includes(String(process.env.OPENCLAW_AUTO_EXECUTE || '').toLowerCase());
const STARTED_AT = Date.now();
const SESSION_TTL_MS = 1000 * 60 * 60 * 12;
const TARGETS = ['遥控器', '钥匙', '玩具', '拖鞋', '袜子', '充电线', '数据线', '耳机', '手机', '积木', '杂物'];
const ZONES = [
  ['sofa_under', '沙发底', /(沙发底|沙发下|沙发下面|沙发)/],
  ['bed_under', '床底', /(床底|床下|床下面)/],
  ['cabinet_under', '柜底', /(柜底|柜下|柜子底)/],
  ['corner', '墙角', /(墙角|角落|边角)/],
  ['desk_under', '桌底', /(桌底|桌下)/],
];
const RETURNS = [
  ['user_feet', '用户脚边', /(脚边|脚下)/],
  ['user_front', '用户前方', /(前方|面前|手边)/],
  ['drop_zone', '回收区', /(回收区|收纳盒|收纳区)/],
  ['dock', '停靠点', /(停靠点|基站|起点)/],
];

const deviceQueues = new Map();
const deviceRegistry = new Map();
const chatSessions = new Map();

const setCors = res => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-OpenClaw-Token');
};
const json = (res, data, status = 200) => {
  setCors(res);
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(data));
};
const sanitize = (v, d = '', n = 1200) => String(v || d).trim().replace(/\s+/g, ' ').slice(0, n);
const bool = v => v === true || ['1', 'true', 'yes', 'on'].includes(String(v).toLowerCase());
const unique = list => list.filter((item, i) => item && list.indexOf(item) === i);
const modeOf = v => (v === 'control' ? 'control' : 'assistant');
const wait = ms => new Promise(resolve => setTimeout(resolve, ms));
const paramsOf = req => new URL(req.url, `http://${req.headers.host || 'localhost'}`).searchParams;
const readBody = req =>
  new Promise((resolve, reject) => {
    const chunks = [];
    req.on('data', chunk => chunks.push(chunk));
    req.on('end', () => {
      try {
        const raw = Buffer.concat(chunks).toString();
        resolve(raw ? JSON.parse(raw) : {});
      } catch (error) {
        reject(error);
      }
    });
    req.on('error', reject);
  });

function pruneSessions() {
  const now = Date.now();
  for (const [key, session] of chatSessions.entries()) if (now - session.updatedAt > SESSION_TTL_MS) chatSessions.delete(key);
}

function getSession(id = 'default-session') {
  pruneSessions();
  const key = sanitize(id, 'default-session', 80).replace(/[^\w-]/g, '-');
  if (!chatSessions.has(key)) chatSessions.set(key, { id: key, messages: [], notes: [], lastActions: [], lastPlan: null, chatMode: DEFAULT_CHAT_MODE, updatedAt: Date.now() });
  return chatSessions.get(key);
}

function resetSession(session) {
  session.messages = [];
  session.notes = [];
  session.lastActions = [];
  session.lastPlan = null;
  session.chatMode = DEFAULT_CHAT_MODE;
  session.updatedAt = Date.now();
}

function hydrateSession(session, history = []) {
  if (!Array.isArray(history) || !history.length) return;
  session.messages = history
    .map(item => ({ role: item?.role === 'assistant' ? 'assistant' : 'user', content: sanitize(item?.content || item?.text || '') }))
    .filter(item => item.content)
    .slice(-18);
  session.updatedAt = Date.now();
}

function pushMessage(session, role, content) {
  const text = sanitize(content);
  if (!text) return;
  const last = session.messages[session.messages.length - 1];
  if (last && last.role === role && last.content === text) return;
  session.messages.push({ role, content: text });
  session.messages = session.messages.slice(-18);
  session.updatedAt = Date.now();
}

function remember(session, note) {
  const text = sanitize(note, '', 160);
  if (text && !session.notes.includes(text)) session.notes.push(text);
  session.notes = session.notes.slice(-8);
  session.updatedAt = Date.now();
}

function queueOf(deviceId = DEVICE_ID) {
  const key = deviceId || DEVICE_ID;
  if (!deviceQueues.has(key)) deviceQueues.set(key, []);
  return deviceQueues.get(key);
}

function queueCommands(deviceId, commands, meta = {}) {
  const queue = queueOf(deviceId);
  const item = { id: `${Date.now()}-${Math.random().toString(16).slice(2, 8)}`, commands: unique(commands), createdAt: new Date().toISOString(), meta };
  queue.push(item);
  return item;
}

function peekDevice(deviceId = DEVICE_ID) {
  const state = deviceRegistry.get(deviceId) || null;
  return { deviceId, connected: Boolean(state), lastSeenAt: state?.lastSeenAt || null, lastTelemetry: state?.telemetry || null, pendingCount: queueOf(deviceId).length };
}

function requireDeviceToken(req, res) {
  if (!DEVICE_TOKEN) return true;
  const auth = req.headers.authorization || '';
  const bearer = auth.startsWith('Bearer ') ? auth.slice(7) : '';
  const alt = req.headers['x-openclaw-token'] || '';
  if (bearer === DEVICE_TOKEN || alt === DEVICE_TOKEN) return true;
  json(res, { error: 'Unauthorized' }, 401);
  return false;
}

function detectEntry(list, text) {
  for (const [id, label, regex] of list) if (regex.test(text)) return { id, label };
  return null;
}

function detectTarget(message, context = {}) {
  const ctx = sanitize(context.target || context.objective || '', '', 80);
  for (const item of TARGETS) if (message.includes(item) || ctx.includes(item)) return item;
  const match = message.match(/(?:找|寻找|搜|搜寻|定位|夹取|抓取|拿出|取出|回收)([^，。,.!?！？]{1,18})/);
  return match?.[1] ? sanitize(match[1].replace(/^(一下|一个|一只|一台|这个|那个|的)/, ''), '', 40) : ctx;
}

function buildActions(message, context = {}, session = null) {
  const zone = detectEntry(ZONES, `${message} ${sanitize(context.zone || context.area || '')}`);
  const ret = detectEntry(RETURNS, `${message} ${sanitize(context.returnPoint || context.returnTarget || '')}`);
  const target = detectTarget(message, context);
  if (/(停止|急停|停下|暂停|别动)/.test(message)) return [{ type: 'stop' }];
  const actions = [];
  if (/(找|寻找|搜|搜寻|定位|扫描|巡拢|检查|观察)/.test(message) || zone || target) actions.push({ type: 'search', zone, target });
  if (/(推拢|聚拢|拨拢|归拢|扫到一起)/.test(message)) actions.push({ type: 'push_align', target });
  if (/(夹|抓|夹取|抓取|取出|拿出|拿回来|回收)/.test(message)) actions.push({ type: 'grab', target });
  if (/(退出|撤回|退出来|出来|撤离)/.test(message)) actions.push({ type: 'exit_zone', zone });
  if (/(停在|回到|送到|带到|脚边|前方|面前|回收区|停靠点)/.test(message) || ret) actions.push({ type: 'return_to_user', returnPoint: ret });
  if (!actions.length && /(继续|接着|然后|下一步)/.test(message) && session?.lastActions?.length) return session.lastActions;
  return actions;
}

function actionText(actions, chatMode) {
  if (!actions.length) return chatMode === 'control' ? '请直接说明区域、目标物和结束位置。' : '可以直接告诉我要去哪里找什么、取到后停在哪里。';
  const lines = [];
  for (const action of actions) {
    if (action.type === 'search') lines.push(`先进入${action.zone?.label || '目标区域'}低速巡拢${action.target ? `，重点找 ${action.target}` : ''}`);
    if (action.type === 'push_align') lines.push('发现目标后先推拢并对齐，减少偏抓');
    if (action.type === 'grab') lines.push(`目标稳定后再闭合夹爪${action.target ? `夹取 ${action.target}` : '执行夹取'}`);
    if (action.type === 'exit_zone') lines.push('取物后立即退出低矮空间，避免二次碰撞');
    if (action.type === 'return_to_user') lines.push(`最后回到${action.returnPoint?.label || '预设交付点'}待命`);
    if (action.type === 'stop') lines.push('立即停止当前动作并保持安全待机');
  }
  return `${chatMode === 'control' ? '执行顺序' : '建议顺序'}：${unique(lines).join('；')}。`;
}

function detectIntent(message, context = {}, session = null, chatMode = DEFAULT_CHAT_MODE) {
  const text = sanitize(message);
  if (!text) return { reason: 'empty', reply: '请先描述任务目标，例如“去床底找拖鞋，夹到后退回脚边”。', actions: [], allowUpstream: false };
  if (/^(清记录|清空记录|清空会话|新会话|重置会话|清空记忆|重置记忆)$/i.test(text)) {
    if (session) resetSession(session);
    return { reason: 'session_reset', reply: '当前会话记录和记忆偏好已清空。', actions: [], allowUpstream: false };
  }
  const memory = text.match(/^(?:记住|记下|以后)(?:[:：\s]+)(.+)$/);
  if (memory?.[1]) return { reason: 'memory', reply: `已记住你的偏好：${sanitize(memory[1], '', 160)}。`, actions: [], allowUpstream: false, memoryNote: sanitize(memory[1], '', 160) };
  if (/^(你好|您好|hi|hello|hey|在吗|有人吗)$/i.test(text)) return { reason: 'greeting', reply: '我在。你可以直接说“去哪里找什么、夹到后停在哪里”。', actions: [], allowUpstream: false };
  if (/(怎么用|如何用|帮助|help|能做什么|你会什么)/i.test(text)) return { reason: 'help', reply: '我负责高层任务理解。你可以让我规划搜寻、推拢、夹取、退出和回到用户，也可以让我解释为什么这样排顺序。', actions: [], allowUpstream: false };
  if (/^(test|测试|1|2|3|ok|okay)$/i.test(text)) return { reason: 'probe', reply: '链路是通的。可以直接给我一条真实任务。', actions: [], allowUpstream: false };
  const actions = buildActions(text, context, session);
  if (actions.length) return { reason: 'task_plan', reply: actionText(actions, chatMode), actions, allowUpstream: true };
  return { reason: 'assistant_chat', reply: chatMode === 'control' ? '控制模式下请尽量直接说区域、目标物和结束位置。' : '可以继续追问任务拆解、策略取舍、风险点，或者直接给我一条完整任务。', actions: [], allowUpstream: true };
}

function buildPlan(actions = []) {
  const commands = [];
  const modeHints = [];
  for (const action of actions) {
    if (action.type === 'search') { modeHints.push('auto_search'); commands.push('MODE AUTO', 'TASK SEARCH'); if (action.zone?.id) commands.push(`ZONE ${action.zone.id}`); if (action.target) commands.push(`TARGET ${action.target}`); commands.push('SEARCH_SPEED LOW'); }
    if (action.type === 'push_align') { modeHints.push('push_align'); commands.push('TASK PUSH_ALIGN'); }
    if (action.type === 'grab') { modeHints.push('grab'); commands.push('TASK GRAB'); if (action.target) commands.push(`TARGET ${action.target}`); }
    if (action.type === 'exit_zone') { modeHints.push('exit_zone'); commands.push('TASK EXIT'); if (action.zone?.id) commands.push(`ZONE ${action.zone.id}`); }
    if (action.type === 'return_to_user') { modeHints.push('return_to_user'); commands.push(`TASK RETURN ${action.returnPoint?.id || 'user_front'}`); }
    if (action.type === 'stop') { modeHints.push('stop'); commands.push('TASK STOP'); }
  }
  return { transport: EXECUTOR_MODE === 'queue' || !DEVICE_API_URL ? 'queue' : 'direct', commands: unique(commands), warnings: [], modeHints: unique(modeHints), targetConfigured: Boolean(DEVICE_API_URL) || EXECUTOR_MODE === 'queue' };
}

function stateText(context = {}, session = null) {
  const device = peekDevice(context.deviceId || DEVICE_ID);
  return [
    `机器人: ${sanitize(context.robot || '智能巡拢家居机器人', '智能巡拢家居机器人', 80)}`,
    `目标: ${sanitize(context.target || context.objective || '未指定', '未指定', 80)}`,
    `底盘模式: ${sanitize(context.chassisMode || '未知', '未知', 80)}`,
    `夹持模式: ${sanitize(context.gripMode || '未知', '未知', 80)}`,
    `传感器健康: ${sanitize(context.sensorHealth || '未知', '未知', 80)}`,
    `设备在线: ${device.connected ? '是' : '否'}`,
    `待执行命令: ${device.pendingCount}`,
    `最近计划: ${session?.lastPlan?.commands?.join(' | ') || '无'}`,
    `会话记忆: ${session?.notes?.length ? session.notes.join('；') : '无'}`,
  ].join('\n');
}

async function callUpstream(payload, heuristic, session, chatMode) {
  if (!heuristic.allowUpstream) return { reply: heuristic.reply, source: 'local', usedUpstream: false, fallbackReason: null };
  const provider = payload.provider || DEFAULT_PROVIDER;
  const endpoint = (payload.endpoint || DEFAULT_ENDPOINT).replace(/\/+$/, '');
  const model = payload.model || DEFAULT_MODEL;
  if (!SERVER_API_KEY || provider === 'local') return { reply: heuristic.reply, source: 'local', usedUpstream: false, fallbackReason: !SERVER_API_KEY ? 'missing_server_api_key' : null };
  const prompt = {
    system: chatMode === 'control'
      ? '你是 OpenClaw 控制模式任务中枢，服务对象是智能巡拢家居机器人。用中文给出清晰、可执行、简洁的任务建议，不要输出 JSON。'
      : '你是 OpenClaw 助手模式任务中枢，服务对象是智能巡拢家居机器人。像成熟的机器人系统工程助手一样回答，中文、自然、具体，不要输出 JSON。',
    context: `当前状态:\n${stateText(payload.context || {}, session)}\n\n候选动作链:\n${actionText(heuristic.actions || [], 'control')}`,
  };
  const doFetch = async () => {
    if (provider === 'claude') {
      const res = await fetch(`${endpoint}/messages`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json', 'x-api-key': SERVER_API_KEY, 'anthropic-version': '2023-06-01' },
        body: JSON.stringify({ model, max_tokens: chatMode === 'assistant' ? 420 : 240, temperature: chatMode === 'assistant' ? 0.45 : 0.2, system: `${prompt.system}\n\n${prompt.context}`, messages: (session?.messages || []).slice(-10).map(item => ({ role: item.role === 'assistant' ? 'assistant' : 'user', content: item.content })) }),
        signal: AbortSignal.timeout(chatMode === 'assistant' ? 18000 : 14000),
      });
      if (!res.ok) throw new Error(`Anthropic upstream failed: ${res.status} ${await res.text()}`);
      const data = await res.json();
      return data.content?.map(item => item.text).join('\n').trim() || '';
    }
    const res = await fetch(`${endpoint}/chat/completions`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', Authorization: `Bearer ${SERVER_API_KEY}` },
      body: JSON.stringify({ model, temperature: chatMode === 'assistant' ? 0.45 : 0.2, max_tokens: chatMode === 'assistant' ? 420 : 240, messages: [{ role: 'system', content: prompt.system }, { role: 'system', content: prompt.context }, ...(session?.messages || []).slice(-10)] }),
      signal: AbortSignal.timeout(chatMode === 'assistant' ? 18000 : 14000),
    });
    if (!res.ok) throw new Error(`OpenAI-compatible upstream failed: ${res.status} ${await res.text()}`);
    const data = await res.json();
    return data.choices?.[0]?.message?.content?.trim() || '';
  };
  try {
    let reply = '';
    for (let i = 0; i < 2; i += 1) {
      try { reply = await doFetch(); break; } catch (error) { if (i === 1 || !/timeout|timed out|aborted|socket|ECONNRESET|502|503|504|429/i.test(String(error.message || error))) throw error; await wait(500 * (i + 1)); }
    }
    if (!sanitize(reply)) return { reply: heuristic.reply, source: 'fallback', usedUpstream: false, fallbackReason: 'empty_upstream_reply' };
    return { reply: sanitize(reply), source: 'upstream', usedUpstream: true, fallbackReason: null };
  } catch (error) {
    const reason = String(error?.message || error);
    console.error(`[${new Date().toISOString()}] Upstream AI fallback: ${reason}`);
    if (/401|invalid_api_key/i.test(reason)) return { reply: '上游模型鉴权失败，当前没有拿到真实模型回复。请检查 OPENCLAW_API_KEY。', source: 'fallback', usedUpstream: false, fallbackReason: reason };
    if (/429|quota/i.test(reason)) return { reply: '上游模型当前限流或配额不足，可以稍后重试。', source: 'fallback', usedUpstream: false, fallbackReason: reason };
    return { reply: heuristic.reply, source: 'fallback', usedUpstream: false, fallbackReason: reason };
  }
}

async function executePlan(plan, payload = {}) {
  const deviceId = payload.deviceId || DEVICE_ID;
  if (!plan.commands.length) return { ok: true, transport: 'none', queued: false, pendingCount: queueOf(deviceId).length };
  if (EXECUTOR_MODE === 'queue' || !DEVICE_API_URL) {
    const item = queueCommands(deviceId, plan.commands, { actions: payload.actions || [], context: payload.context || {} });
    return { ok: true, transport: 'queue', queued: true, queueItemId: item.id, deviceId, pendingCount: queueOf(deviceId).length };
  }
  const headers = { 'Content-Type': 'application/json' };
  if (DEVICE_API_TOKEN) headers.Authorization = `Bearer ${DEVICE_API_TOKEN}`;
  const res = await fetch(DEVICE_API_URL, { method: 'POST', headers, body: JSON.stringify({ deviceId, commands: plan.commands, actions: payload.actions || [], context: payload.context || {} }), signal: AbortSignal.timeout(12000) });
  if (!res.ok) return { ok: false, transport: 'direct', deviceId, error: `Device API failed: ${res.status} ${await res.text()}` };
  return { ok: true, transport: 'direct', deviceId, response: await res.json().catch(() => ({})) };
}

const server = http.createServer(async (req, res) => {
  if (req.method === 'OPTIONS') { setCors(res); res.writeHead(204); res.end(); return; }
  if (req.method === 'GET' && req.url === '/api/openclaw/status') {
    pruneSessions();
    const device = peekDevice(DEVICE_ID);
    return json(res, { ok: true, service: 'openclaw-server', uptimeSec: Math.floor((Date.now() - STARTED_AT) / 1000), provider: DEFAULT_PROVIDER, endpoint: DEFAULT_ENDPOINT, hasServerApiKey: Boolean(SERVER_API_KEY), executorMode: EXECUTOR_MODE, deviceApiConfigured: Boolean(DEVICE_API_URL) || EXECUTOR_MODE === 'queue', autoExecute: AUTO_EXECUTE, deviceId: DEVICE_ID, deviceConnected: device.connected, deviceLastSeenAt: device.lastSeenAt, pendingDeviceCommands: device.pendingCount, activeSessions: chatSessions.size, defaultChatMode: DEFAULT_CHAT_MODE });
  }
  if (req.method === 'GET' && req.url.startsWith('/api/openclaw/device/status')) return json(res, { ok: true, ...peekDevice(sanitize(paramsOf(req).get('device_id') || paramsOf(req).get('deviceId') || DEVICE_ID, DEVICE_ID, 80)) });
  if (req.method === 'GET' && req.url.startsWith('/api/openclaw/device/next')) {
    if (!requireDeviceToken(req, res)) return;
    const deviceId = sanitize(paramsOf(req).get('device_id') || paramsOf(req).get('deviceId') || DEVICE_ID, DEVICE_ID, 80);
    const item = queueOf(deviceId).shift() || null;
    return json(res, { ok: true, deviceId, commands: item?.commands || [], queueItemId: item?.id || null, pendingCount: queueOf(deviceId).length, serverTime: new Date().toISOString() });
  }
  if (req.method === 'POST' && req.url === '/api/openclaw/device/telemetry') {
    if (!requireDeviceToken(req, res)) return;
    let payload; try { payload = await readBody(req); } catch { return json(res, { error: 'Invalid JSON' }, 400); }
    const deviceId = sanitize(payload.device_id || payload.deviceId || DEVICE_ID, DEVICE_ID, 80);
    deviceRegistry.set(deviceId, { lastSeenAt: new Date().toISOString(), telemetry: payload });
    return json(res, { ok: true, deviceId, pendingCount: queueOf(deviceId).length });
  }
  if (req.method === 'POST' && req.url === '/api/openclaw/session/reset') {
    let payload; try { payload = await readBody(req); } catch { return json(res, { error: 'Invalid JSON' }, 400); }
    const session = getSession(payload.sessionId || 'default-session');
    resetSession(session);
    return json(res, { ok: true, sessionId: session.id, defaultChatMode: DEFAULT_CHAT_MODE });
  }
  if (req.method === 'POST' && (req.url === '/api/openclaw/chat' || req.url === '/api/openclaw/recommend' || req.url === '/api/openclaw/execute')) {
    let payload; try { payload = await readBody(req); } catch { return json(res, { error: 'Invalid JSON' }, 400); }
    const session = getSession(payload.sessionId || (req.url.endsWith('/recommend') ? 'recommend-session' : 'default-session'));
    hydrateSession(session, payload.history || []);
    const chatMode = modeOf(payload.chatMode || session.chatMode);
    session.chatMode = chatMode;
    const message = sanitize(payload.message || (req.url.endsWith('/recommend') ? '请给出任务建议' : ''));
    const heuristic = req.url.endsWith('/execute') && Array.isArray(payload.actions) ? { reason: 'manual', reply: '', actions: payload.actions, allowUpstream: false } : detectIntent(message, payload.context || {}, session, chatMode);
    if (heuristic.memoryNote) remember(session, heuristic.memoryNote);
    const plan = buildPlan(heuristic.actions || []);
    if (req.url.endsWith('/recommend')) return json(res, { ok: true, recommendation: { actions: heuristic.actions, summary: heuristic.reply, reason: heuristic.reason }, devicePlan: plan, sessionId: session.id });
    if (req.url.endsWith('/execute')) {
      const deviceExecution = await executePlan(plan, { deviceId: payload.deviceId || DEVICE_ID, actions: heuristic.actions, context: payload.context || {} });
      return json(res, { ok: deviceExecution.ok, actions: heuristic.actions, devicePlan: plan, deviceExecution });
    }
    if (!message) return json(res, { error: 'Missing message' }, 400);
    pushMessage(session, 'user', message);
    const reply = await callUpstream(payload, heuristic, session, chatMode);
    const exec = bool(payload.execute) || AUTO_EXECUTE ? await executePlan(plan, { deviceId: payload.deviceId || DEVICE_ID, actions: heuristic.actions, context: payload.context || {} }) : null;
    pushMessage(session, 'assistant', reply.reply);
    session.lastActions = heuristic.actions || [];
    session.lastPlan = plan;
    session.updatedAt = Date.now();
    return json(res, { ok: true, reply: reply.reply, actions: heuristic.actions, reason: heuristic.reason, fallback: !reply.usedUpstream, aiSource: reply.source, fallbackReason: reply.fallbackReason, devicePlan: plan, deviceExecution: exec, sessionId: session.id, chatMode, memoryNotes: session.notes, history: session.messages });
  }
  return json(res, { error: 'Not Found' }, 404);
});

server.listen(PORT, HOST, () => {
  console.log(`OpenClaw server listening on ${HOST}:${PORT}`);
  console.log(`Default provider: ${DEFAULT_PROVIDER}`);
  console.log(`Default endpoint: ${DEFAULT_ENDPOINT}`);
  console.log(`Default device id: ${DEVICE_ID}`);
});
