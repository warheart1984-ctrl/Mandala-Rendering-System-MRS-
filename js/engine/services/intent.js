/**
 * Intent Service — declare / queue intents (ISL-lite JSON).
 */

let seq = 0;

function makeId() {
  seq += 1;
  return `intent-${Date.now().toString(36)}-${seq.toString(36)}`;
}

export class IntentService {
  constructor() {
    this.queue = [];
  }

  declare({ type, kind, goal, actor, payload = {}, constraints = {} }) {
    const intent = {
      id: makeId(),
      actor: actor ?? "4dce.renderer",
      type: type ?? kind ?? "render_scene",
      kind: kind ?? type,
      goal: goal ?? "",
      payload,
      constraints,
      timestamp: new Date().toISOString(),
    };
    this.queue.push(intent);
    return intent;
  }

  collect() {
    const out = this.queue.slice();
    this.queue.length = 0;
    return out;
  }
}
