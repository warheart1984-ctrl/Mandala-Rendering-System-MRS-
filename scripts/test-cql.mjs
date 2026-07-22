/**
 * CQL smoke test — runs a sample query over the CSSV ledger.
 * Usage: node scripts/test-cql.mjs
 */

import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";
import { parseCql } from "../engine/cssv/cqlParser.js";
import { executeCql } from "../engine/cssv/cqlInterpreter.js";
import { loadLedger } from "../engine/cssv/ledger.js";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");

const ledger = await loadLedger(resolve(root, "cssv"));
const ast = parseCql(
  'SELECT frames FROM frame WHERE frame.timeline = "mythar_ascension"',
);
const rows = await executeCql(ast, ledger);

console.log(`CQL returned ${rows.length} row(s) (expected 0+ from seed ledger)`);
console.log("CQL parser + interpreter: OK");
process.exit(0);
