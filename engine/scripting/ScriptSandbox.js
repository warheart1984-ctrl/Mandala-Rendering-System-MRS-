/**
 * Scripting sandbox — **not** a security boundary.
 *
 * Provides a denylist scan of ISL source text (substring match only). It can be
 * bypassed (e.g. `window["fetch"]`) and does not isolate execution. Real
 * protection is architectural: ISL may only emit intents; hosts must call
 * GovernedRuntime / ExecutionOrchestrator — never Unity/Unreal APIs directly.
 */

import { createIslEngine } from "./IslInterpreter.js";

const FORBIDDEN = [
  "fetch",
  "XMLHttpRequest",
  "WebSocket",
  "localStorage",
  "indexedDB",
  "eval",
  "Function",
  "importScripts",
  "Deno",
  "process",
  "require",
  "fs",
];

export class ScriptSandbox {
  constructor({ intentService, orchestrator } = {}) {
    this.isl = createIslEngine();
    this.intentService = intentService ?? null;
    this.orchestrator = orchestrator ?? null;
    this.capabilities = Object.freeze([
      "isl.evaluate",
      "intent.enqueue",
      "orchestrator.execute",
    ]);
  }

  /**
   * Compile ISL inside sandbox. Does not mutate world state.
   */
  evaluateIsl(islSource, context = {}) {
    this._assertNoForbidden(islSource);
    const contextJson =
      typeof context === "string" ? context : JSON.stringify(context);
    return this.isl.CompileAndEvaluate(islSource, contextJson);
  }

  /**
   * Evaluate ISL and enqueue intent via IntentService only.
   */
  submitIsl(islSource, context = {}) {
    const intent = this.evaluateIsl(islSource, context);
    if (!this.intentService) {
      throw new Error("Sandbox: IntentService not bound");
    }
    this.intentService.queue.push(intent);
    return intent;
  }

  /**
   * Only path for mutations: through ExecutionOrchestrator.
   */
  async executeGoverned({ intent, evidence, action, run }) {
    if (!this.orchestrator) {
      throw new Error("Sandbox: ExecutionOrchestrator not bound");
    }
    return this.orchestrator.execute({ intent, evidence, action, run });
  }

  _assertNoForbidden(source) {
    // Cosmetic denylist only — not a sandbox VM. Prefer ISL→intent-only architecture.
    for (const word of FORBIDDEN) {
      if (source.includes(word)) {
        throw new Error(`Sandbox denied: ISL source must not reference '${word}'`);
      }
    }
  }
}
