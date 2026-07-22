/**
 * Static file server for the browser host (repo root).
 * Usage: npm run serve  (default port 8080)
 */

import http from "node:http";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, "..");
const PORT = Number(process.env.PORT) || 8080;
const SHARED_FRAME_PATH = process.env.SOVEREIGNX_SHARED_FRAME_PATH;

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json",
  ".png": "image/png",
  ".svg": "image/svg+xml",
  ".md": "text/plain; charset=utf-8",
};

const server = http.createServer((req, res) => {
  const urlPath = decodeURIComponent(req.url.split("?")[0]);
  if (urlPath === "/__sovereignx/frame") {
    if (!SHARED_FRAME_PATH || !fs.existsSync(SHARED_FRAME_PATH)) { res.writeHead(404); res.end("Shared frame unavailable"); return; }
    const fd=fs.openSync(SHARED_FRAME_PATH,"r");
    try {
      const header=Buffer.alloc(32);fs.readSync(fd,header,0,32,0);const slot=header.readUInt32LE(24),slotBytes=header.readUInt32LE(28);
      if(header.readUInt32LE(0)!==0x58524653||slot>1||slotBytes<4||slotBytes>268435456)throw new Error("Invalid shared frame header");
      const pixels=Buffer.allocUnsafe(slotBytes);fs.readSync(fd,pixels,0,slotBytes,32+slot*slotBytes);header.writeUInt32LE(0,24);
      res.writeHead(200,{"Content-Type":"application/x-sovereignx-frame","Cache-Control":"no-store","Content-Length":32+slotBytes});res.end(Buffer.concat([header,pixels]));
    } catch(error) { res.writeHead(503);res.end(error instanceof Error?error.message:"Shared frame read failed"); } finally { fs.closeSync(fd); }
    return;
  }
  const rel = urlPath === "/" ? "index.html" : urlPath.replace(/^\//, "");
  const filePath = path.resolve(ROOT, rel);

  if (!filePath.startsWith(ROOT)) {
    res.writeHead(403);
    res.end("Forbidden");
    return;
  }

  if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
    res.writeHead(404);
    res.end("Not found");
    return;
  }

  const ext = path.extname(filePath);
  res.writeHead(200, {
    "Content-Type": MIME[ext] ?? "application/octet-stream",
    "Cache-Control": "no-cache",
  });
  fs.createReadStream(filePath).pipe(res);
});

server.listen(PORT, () => {
  console.log(`Browser host: http://localhost:${PORT}/`);
  console.log(`Open index.html via HTTP (required for ES modules + fetch).`);
});
