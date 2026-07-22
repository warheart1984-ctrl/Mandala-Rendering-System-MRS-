/**
 * Ensure CSSV ledger files exist with valid empty structure.
 * Usage: npm run init:cssv
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const cssvDir = path.join(root, "cssv");

if (!fs.existsSync(cssvDir)) fs.mkdirSync(cssvDir, { recursive: true });

const artifactsPath = path.join(cssvDir, "artifacts.json");
if (!fs.existsSync(artifactsPath)) {
  fs.writeFileSync(artifactsPath, "[]\n", "utf8");
  console.log("Created cssv/artifacts.json");
}

for (const name of ["transitions.ndjson", "frames.ndjson"]) {
  const p = path.join(cssvDir, name);
  if (!fs.existsSync(p)) {
    fs.writeFileSync(p, "", "utf8");
    console.log(`Created cssv/${name}`);
  }
}

console.log("CSSV ledger ready.");
