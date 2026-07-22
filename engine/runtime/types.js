/**
 * Shared ID / record helpers (JS host).
 * C# / C++ mirrors: engine/scripting/IIslEngine.cs, engine/scripting/FIslEngine.h
 */

let _seq = 0;

/**
 * Monotonic-ish id: prefix + time + sequence (avoids same-ms collisions).
 */
export function makeId(prefix) {
  _seq = (_seq + 1) >>> 0;
  return `${prefix}-${Date.now().toString(36)}-${_seq.toString(36)}`;
}

export function nowIso() {
  return new Date().toISOString();
}
