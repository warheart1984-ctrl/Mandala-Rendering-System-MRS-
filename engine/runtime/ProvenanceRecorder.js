/**
 * Frame provenance + recorder (JS / browser).
 */

export function createFrameProvenance({
  intentId,
  timelineId,
  worldId,
  timeSeconds,
  parameters = {},
}) {
  return {
    intentId: intentId ?? null,
    timelineId: timelineId ?? null,
    worldId: worldId ?? null,
    timeSeconds: timeSeconds ?? 0,
    parameters: { ...parameters },
  };
}

export class ProvenanceRecorder {
  constructor() {
    this.frames = [];
  }

  record(frame) {
    this.frames.push(frame);
  }

  getFrames() {
    return this.frames.slice();
  }

  clear() {
    this.frames.length = 0;
  }

  get count() {
    return this.frames.length;
  }

  exportJson() {
    return JSON.stringify({ frames: this.frames }, null, 2);
  }
}
