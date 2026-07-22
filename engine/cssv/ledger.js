/**

 * CSSV ledger loader — host-agnostic read/write of artifacts + NDJSON streams.

 */



import fs from "node:fs";

import readline from "node:readline";

import { fileURLToPath } from "node:url";

import { dirname, resolve } from "node:path";



const __dirname = dirname(fileURLToPath(import.meta.url));

const DEFAULT_ROOT = resolve(__dirname, "../../cssv");



export function ledgerPaths(root = DEFAULT_ROOT) {

  return {

    root,

    artifacts: resolve(root, "artifacts.json"),

    transitions: resolve(root, "transitions.ndjson"),

    frames: resolve(root, "frames.ndjson"),

  };

}



/** Create cssv/ directory and empty ledger files if missing. */

export function ensureLedgerInitialized(root = DEFAULT_ROOT) {

  if (!fs.existsSync(root)) fs.mkdirSync(root, { recursive: true });

  const paths = ledgerPaths(root);

  if (!fs.existsSync(paths.artifacts)) {

    fs.writeFileSync(paths.artifacts, "[]\n", "utf8");

  }

  for (const p of [paths.transitions, paths.frames]) {

    if (!fs.existsSync(p)) fs.writeFileSync(p, "", "utf8");

  }

  return paths;

}



export function loadArtifacts(path) {

  const p = path ?? ledgerPaths().artifacts;

  if (!fs.existsSync(p)) return [];

  const raw = JSON.parse(fs.readFileSync(p, "utf8"));

  return Array.isArray(raw) ? raw : raw.artifacts ?? [];

}



export async function loadNdjson(path) {

  if (!fs.existsSync(path)) return [];

  const stream = fs.createReadStream(path, "utf8");

  const rl = readline.createInterface({ input: stream });

  const rows = [];

  for await (const line of rl) {

    const trimmed = line.trim();

    if (!trimmed) continue;

    try {

      rows.push(JSON.parse(trimmed));

    } catch (err) {

      console.warn(

        `[CSSV] Skipping malformed NDJSON line in ${path}:`,

        err instanceof Error ? err.message : String(err),

      );

    }

  }

  return rows;

}



export async function loadLedger(root = DEFAULT_ROOT) {

  ensureLedgerInitialized(root);

  const paths = ledgerPaths(root);

  const [artifacts, transitions, frames] = await Promise.all([

    Promise.resolve(loadArtifacts(paths.artifacts)),

    loadNdjson(paths.transitions),

    loadNdjson(paths.frames),

  ]);

  return { artifacts, transitions, frames, paths };

}



export function appendNdjson(path, record) {

  ensureLedgerInitialized(dirname(path));

  fs.appendFileSync(path, `${JSON.stringify(record)}\n`, "utf8");

}



export function appendNdjsonBatch(path, records) {

  if (!records?.length) return;

  ensureLedgerInitialized(dirname(path));

  const chunk = records.map((r) => JSON.stringify(r)).join("\n") + "\n";

  fs.appendFileSync(path, chunk, "utf8");

}



export function saveArtifacts(path, artifacts) {

  ensureLedgerInitialized(dirname(path));

  fs.writeFileSync(path, JSON.stringify(artifacts, null, 2), "utf8");

}



export function mergeArtifacts(path, incoming) {

  const existing = loadArtifacts(path);

  for (const record of incoming) {

    const idx = existing.findIndex((a) => a.id === record.id);

    if (idx >= 0) existing[idx] = record;

    else existing.push(record);

  }

  saveArtifacts(path, existing);

  return existing.length;

}


