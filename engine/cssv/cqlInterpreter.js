/**
 * CQL interpreter — executes queries over a CSSV ledger.
 */

import { loadLedger } from "./ledger.js";

function getSource(ledger, from) {
  switch (from) {
    case "artifact":
      return ledger.artifacts;
    case "transition":
      return ledger.transitions;
    case "frame":
      return ledger.frames;
    default:
      throw new Error(`Unknown FROM source: ${from}`);
  }
}

function getField(row, fieldPath) {
  if (!fieldPath) return undefined;
  const aliases = {
    "frame.timeline": "timeline",
    "frame.intent": "intent",
    "frame.world": "world",
    "frame.timestamp": "timestamp",
    "frame.host": "host",
    "frame.frameIndex": "frameIndex",
  };
  const path = aliases[fieldPath] ?? fieldPath;
  if (path.startsWith("frame.params."))
    return row.params?.[path.slice("frame.params.".length)];
  if (path.startsWith("intent.")) {
    const key = path.slice("intent.".length);
    return row.intent?.[key];
  }
  if (path === "evidence.count")
    return Array.isArray(row.evidence) ? row.evidence.length : 0;
  if (path === "decision.allowed") return row.decision?.allowed;
  if (path === "decision.reasons")
    return (row.decision?.reasons ?? []).join(" ");
  return path.split(".").reduce((acc, key) => acc?.[key], row);
}

function applyOp(v, op, value) {
  switch (op) {
    case "=":
      return String(v) === String(value);
    case "!=":
      return String(v) !== String(value);
    case ">":
      return Number(v) > Number(value);
    case "<":
      return Number(v) < Number(value);
    case ">=":
      return Number(v) >= Number(value);
    case "<=":
      return Number(v) <= Number(value);
    case "CONTAINS":
      return String(v ?? "").includes(String(value));
    default:
      throw new Error(`Unknown op: ${op}`);
  }
}

function evalExpr(expr, row) {
  if (!expr) return true;
  if (expr.type === "binary") {
    const v = getField(row, expr.field);
    return applyOp(v, expr.op, expr.value);
  }
  if (expr.type === "logical") {
    const l = evalExpr(expr.left, row);
    const r = evalExpr(expr.right, row);
    return expr.op === "AND" ? l && r : l || r;
  }
  return true;
}

function compareField(a, b, orderBy) {
  const va = getField(a, orderBy.field);
  const vb = getField(b, orderBy.field);
  if (va < vb) return orderBy.direction === "ASC" ? -1 : 1;
  if (va > vb) return orderBy.direction === "ASC" ? 1 : -1;
  return 0;
}

export async function executeCql(queryAst, ledgerOverride) {
  const ledger = ledgerOverride ?? (await loadLedger());
  let rows = getSource(ledger, queryAst.from).filter((row) =>
    evalExpr(queryAst.where, row),
  );
  if (queryAst.orderBy)
    rows = rows.slice().sort((a, b) => compareField(a, b, queryAst.orderBy));
  if (queryAst.limit != null) rows = rows.slice(0, queryAst.limit);
  return rows;
}
