/**
 * Quick verify: ISL parse Opening4DReveal + CKL deny missing world / allow with evidence.
 * Run: node engine/tests/verify_isl_ckl.mjs
 * (from repo root: G:\New folder)
 */

import { readFileSync } from "fs";
import { fileURLToPath } from "url";
import { dirname, join } from "path";
import { createIslEngine } from "../scripting/IslInterpreter.js";
import {
  ConstitutionalKnowledgeLayer,
  resolveDecision,
} from "../governance/ConstitutionalKnowledgeLayer.js";

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = join(__dirname, "..", "..");

const islPath = join(root, "engine/scripting/scripts/Opening4DReveal.isl");
const policyPath = join(root, "engine/governance/policies/default.policies.json");

const islSource = readFileSync(islPath, "utf8");
const policies = JSON.parse(readFileSync(policyPath, "utf8"));

const isl = createIslEngine();
const intent = isl.CompileAndEvaluate(islSource, JSON.stringify({
  actor: "4dce.timeline",
  worldId: "world-mythar-plains",
}));

if (intent.type !== "play_timeline") throw new Error("Expected play_timeline");
if (intent.world !== "world-mythar-plains") throw new Error("Expected world id");
if (!intent.timeline) throw new Error("Expected timeline arg");

const ckl = new ConstitutionalKnowledgeLayer(policies);
const policySet = ckl.GetPoliciesForWorld(intent.world);

const denyWorld = resolveDecision(
  { ...intent, world: null, constraints: {} },
  { id: "ev" },
  policySet,
);
if (denyWorld.ok) throw new Error("Expected deny when world missing");
if (!denyWorld.violations.includes("policy-play-timeline-requires-world")) {
  throw new Error("Expected policy-play-timeline-requires-world in violations");
}

const denyEvidence = resolveDecision(intent, null, policySet);
if (denyEvidence.ok) throw new Error("Expected deny when evidence missing");

const allow = resolveDecision(intent, { id: "ev-open-4d-001" }, policySet);
if (!allow.ok) throw new Error("Expected allow with world + evidence: " + allow.reason);

const hasWorldPolicy = policies.some((p) => p.id === "policy-play-timeline-requires-world");
if (!hasWorldPolicy) throw new Error("Policy JSON missing policy-play-timeline-requires-world");

console.log("PASS: ISL parse + CKL deny(missing world) + deny(missing evidence) + allow");
console.log(JSON.stringify({
  intentId: intent.id,
  type: intent.type,
  world: intent.world,
  timeline: intent.timeline,
  evidenceId: intent.evidenceId,
  allowDecision: allow.decisionId,
}, null, 2));
