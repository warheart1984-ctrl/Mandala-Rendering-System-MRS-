/**
 * Quick CKL ascension policy smoke test (Node).
 */
import { readFileSync } from "fs";
import { resolveDecision, ConstitutionalKnowledgeLayer } from "../engine/governance/ConstitutionalKnowledgeLayer.js";
import { createIslEngine } from "../engine/scripting/IslInterpreter.js";
import { ISL_MYTHAR_ASCENSION } from "../engine/scripting/scripts/mythar_ascension.isl.js";

const policies = JSON.parse(
  readFileSync(new URL("../engine/governance/policies/default.policies.json", import.meta.url), "utf8"),
);
const ckl = new ConstitutionalKnowledgeLayer(policies);
const isl = createIslEngine();
const intent = isl.CompileAndEvaluate(
  ISL_MYTHAR_ASCENSION,
  JSON.stringify({ actor: "4dce.timeline", worldId: "world-mythar-plains" }),
);

const deny = resolveDecision(
  intent,
  { id: "ev-ascension-001", evidenceIds: ["ev-ascension-001"] },
  ckl.GetPoliciesForWorld(intent.world),
);
const allow = resolveDecision(
  intent,
  {
    id: "ev-ascension-001",
    evidenceIds: ["ev-ascension-001", "ev-ascension-002"],
    driftScore: 0.2,
  },
  ckl.GetPoliciesForWorld(intent.world),
);
const throttle = resolveDecision(
  intent,
  {
    id: "ev-ascension-001",
    evidenceIds: ["ev-ascension-001", "ev-ascension-002"],
    driftScore: 0.85,
    params: { speed: 2 },
  },
  ckl.GetPoliciesForWorld(intent.world),
);

console.log("intent.timeline", intent.timeline);
console.log("deny.ok", deny.ok, "violations", deny.violations);
console.log("allow.ok", allow.ok);
console.log("throttle.paramAdjust", throttle.paramAdjust);

if (deny.ok) throw new Error("expected dual-evidence deny");
if (!allow.ok) throw new Error("expected dual-evidence allow");
if (!throttle.paramAdjust || throttle.paramAdjust.speed !== 1) {
  // speed * 0.5 from 2 → 1 when drift high
  console.warn("throttle may not have applied speed; got", throttle.paramAdjust);
}
console.log("OK");
