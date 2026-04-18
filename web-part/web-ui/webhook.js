const http = require('http');
const crypto = require('crypto');
const { execFile } = require('child_process');
const path = require('path');

const SECRET = process.env.WEBHOOK_SECRET || 'change-me-before-production';
const REPO_DIR = process.env.REPO_DIR || path.resolve(__dirname, '..', '..');
const PORT = Number(process.env.PORT || 9010);
const HOST = process.env.HOST || '127.0.0.1';
const DEPLOY_BRANCH = process.env.DEPLOY_BRANCH || 'main';
const GIT_REMOTE = process.env.GIT_REMOTE || 'origin';
const WEBHOOK_PATH = process.env.WEBHOOK_PATH || '/sr-webhook';
const POST_DEPLOY_COMMAND =
  process.env.POST_DEPLOY_COMMAND ||
  'npm --prefix web-part/web-ui install && npm --prefix web-part/web-ui/app install && npm --prefix web-part/web-ui/app run build';
const shell =
  process.platform === 'win32'
    ? { command: 'cmd.exe', args: ['/d', '/s', '/c'] }
    : { command: '/bin/bash', args: ['-lc'] };

if (SECRET === 'change-me-before-production') {
  console.warn('WARNING: WEBHOOK_SECRET is using the default placeholder value.');
}

function verifySignature(payload, signature) {
  if (!signature) return false;
  const expected = 'sha256=' + crypto.createHmac('sha256', SECRET).update(payload).digest('hex');
  const expectedBuffer = Buffer.from(expected);
  const signatureBuffer = Buffer.from(signature);
  if (expectedBuffer.length !== signatureBuffer.length) return false;
  return crypto.timingSafeEqual(expectedBuffer, signatureBuffer);
}

function runShell(command, cwd, callback) {
  execFile(shell.command, [...shell.args, command], { cwd }, callback);
}

const server = http.createServer((req, res) => {
  if (req.method !== 'POST' || req.url !== WEBHOOK_PATH) {
    res.writeHead(404);
    res.end('Not Found');
    return;
  }

  const chunks = [];
  req.on('data', (chunk) => chunks.push(chunk));
  req.on('end', () => {
    const body = Buffer.concat(chunks);
    const signature = req.headers['x-hub-signature-256'];

    if (!verifySignature(body, signature)) {
      console.log(`[${new Date().toISOString()}] Invalid signature, rejected`);
      res.writeHead(403);
      res.end('Forbidden');
      return;
    }

    let payload;
    try {
      payload = JSON.parse(body);
    } catch {
      res.writeHead(400);
      res.end('Bad Request');
      return;
    }

    const ref = payload.ref || '';
    const expectedRef = `refs/heads/${DEPLOY_BRANCH}`;
    if (ref !== expectedRef) {
      console.log(`[${new Date().toISOString()}] Ignored push to ${ref}`);
      res.writeHead(200);
      res.end(`Ignored: not ${DEPLOY_BRANCH} branch`);
      return;
    }

    console.log(
      `[${new Date().toISOString()}] Deploying smart-gathering web: push to ${DEPLOY_BRANCH} by ${
        payload.pusher?.name || 'unknown'
      }`
    );
    res.writeHead(200);
    res.end('Deploying...');

    execFile('git', ['pull', '--ff-only', GIT_REMOTE, DEPLOY_BRANCH], { cwd: REPO_DIR }, (err, stdout, stderr) => {
      if (err) {
        console.error(`[${new Date().toISOString()}] Deploy failed:`, stderr);
        return;
      }

      console.log(`[${new Date().toISOString()}] Git pull success:`, stdout.trim());
      runShell(POST_DEPLOY_COMMAND, REPO_DIR, (postErr, postStdout, postStderr) => {
        if (postErr) {
          console.error(`[${new Date().toISOString()}] Post deploy failed:`, postStderr);
          return;
        }
        console.log(`[${new Date().toISOString()}] Post deploy success:`, postStdout.trim());
      });
    });
  });
});

server.listen(PORT, HOST, () => {
  console.log(`Smart gathering webhook listening on ${HOST}:${PORT}`);
  console.log(`Repo dir: ${REPO_DIR}`);
  console.log(`Deploy branch: ${DEPLOY_BRANCH}`);
  console.log(`Webhook path: ${WEBHOOK_PATH}`);
});
