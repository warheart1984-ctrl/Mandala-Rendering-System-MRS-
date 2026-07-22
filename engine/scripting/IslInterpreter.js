/**
 * ISL v2.0 interpreter — AST + context → IntentRecord
 */

import { makeId, nowIso } from "../runtime/types.js";
import { parseIsl } from "./IslParser.js";

const VERB_TO_TYPE = {
  play_timeline: "play_timeline",
  render_4d_tesseract: "render_4d_tesseract",
  update_world: "update_world",
  render_scene: "render_scene",
};

/**
 * @param {object} stmt IntentStmt AST
 * @param {object} context parsed context JSON
 */
export function evaluateIntentStmt(stmt, context = {}) {
  const type = VERB_TO_TYPE[stmt.verb] ?? stmt.verb;
  const primary = stmt.args[0];
  const record = {
    id: makeId("intent"),
    actor: context.actor ?? "4dce.isl",
    type,
    kind: type,
    world: stmt.worldId,
    timestamp: nowIso(),
    source: "isl-v2",
    goal: `ISL ${stmt.verb}`,
    payload: {},
    constraints: {
      worldId: stmt.worldId,
    },
  };

  if (type === "play_timeline") {
    record.timeline = typeof primary === "string" ? primary : undefined;
    record.payload.timeline = record.timeline;
  } else if (type === "render_4d_tesseract") {
    record.entity = typeof primary === "string" ? primary : undefined;
    record.payload.entity = record.entity;
  } else if (primary !== undefined) {
    record.payload.arg0 = primary;
  }

  if (stmt.at) {
    record.at = stmt.at;
    record.constraints.at = stmt.at;
  }

  if (stmt.evidence?.kind === "evidence") {
    record.evidenceId = stmt.evidence.id;
    record.constraints.evidenceId = stmt.evidence.id;
  }
  if (stmt.evidence?.kind === "params") {
    record.params = stmt.evidence.object;
    record.payload.params = stmt.evidence.object;
  }

  if (context.worldId && context.worldId !== stmt.worldId) {
    record.constraints.worldMismatch = true;
  }

  return record;
}

export function interpretProgram(ast, context = {}) {
  if (!ast?.statements?.length) {
    throw new Error("ISL program has no intent statements");
  }
  // Semantics: only the last IntentStmt is evaluated (one primary intent per script).
  // Earlier statements are parsed but ignored — see ISL_V2_GRAMMAR.md.
  const stmt = ast.statements[ast.statements.length - 1];
  return evaluateIntentStmt(stmt, context);
}

/**
 * IIslEngine-compatible API (JS).
 */
export class IslEngine {
  CompileAndEvaluate(islSource, contextJson = "{}") {
    let context = {};
    if (typeof contextJson === "string") {
      context = contextJson.trim() ? JSON.parse(contextJson) : {};
    } else if (contextJson && typeof contextJson === "object") {
      context = contextJson;
    }
    const ast = parseIsl(islSource);
    return interpretProgram(ast, context);
  }
}

export function createIslEngine() {
  return new IslEngine();
}
