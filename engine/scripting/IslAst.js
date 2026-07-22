/**
 * ISL v2.0 — AST nodes.
 * Grammar: intent Verb(args) in world("id") [at "tc"] [with evidence("id") | params {...}]
 */

export function IntentNode(verb, args, worldId, at, evidence) {
  return { kind: "IntentStmt", verb, args, worldId, at, evidence };
}

export function ProgramNode(statements) {
  return { kind: "Program", statements };
}
