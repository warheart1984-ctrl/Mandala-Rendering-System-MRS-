/**
 * Start browser static server + CSSV dashboard together.
 * Usage: npm start
 */

import { spawn } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const node = process.execPath;

spawn(node, [path.join(root, "scripts/init-cssv-ledger.mjs")], {
  cwd: root,
  stdio: "inherit",
});

const children = [
  spawn(node, [path.join(root, "scripts/serve-static.mjs")], {
    cwd: root,
    stdio: "inherit",
    env: { ...process.env, PORT: process.env.PORT ?? "8080" },
  }),
  spawn(node, [path.join(root, "cssv/server.js")], {
    cwd: root,
    stdio: "inherit",
    env: { ...process.env, CSSV_PORT: process.env.CSSV_PORT ?? "3000" },
  }),
];

function shutdown() {
  for (const c of children) c.kill("SIGTERM");
  process.exit(0);
}

process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);

console.log("Dev stack: browser :8080 · CSSV dashboard :3000");
console.log("Set window.__CSSV_SYNC_URL = 'http://localhost:3000/ingest' in console to sync ledger.");
