/**
 * DecisionEngine — applies CKL policies (JS).
 * Authoritative implementation: resolveDecision in ConstitutionalKnowledgeLayer.js
 * C# mirror: engine/governance/DecisionEngine.cs
 */

import { resolveDecision } from "./ConstitutionalKnowledgeLayer.js";

export class DecisionEngine {
  static resolve(intent, evidence, policySet, precedents = []) {
    return resolveDecision(intent, evidence, policySet, precedents);
  }
}

export { resolveDecision };
