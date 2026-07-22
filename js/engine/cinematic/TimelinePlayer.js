/**
 * Cinematic timeline player — drives renderer params from governed clips.
 * Enforced actions: set_param, render_4d (speed ramp).
 */

function lerp(a, b, t) {
  return a + (b - a) * t;
}

function clipProgress(clip, timeSec) {
  const end = clip.startSec + clip.durationSec;
  if (timeSec < clip.startSec || timeSec > end) return null;
  if (clip.durationSec <= 0) return 1;
  return (timeSec - clip.startSec) / clip.durationSec;
}

export class TimelinePlayer {
  constructor(timeline) {
    this.timeline = timeline;
    this.timeSec = 0;
    this.playing = false;
    this.durationSec = timeline.durationSec ?? 12;
  }

  play() {
    this.playing = true;
  }

  pause() {
    this.playing = false;
  }

  seek(t) {
    this.timeSec = Math.max(0, Math.min(this.durationSec, t));
  }

  reset() {
    this.timeSec = 0;
    this.playing = false;
  }

  /**
   * Advance clock and apply active clips to renderer.
   * @returns {{ timeSec: number, applied: string[] }}
   */
  tick(dt, renderer) {
    if (this.playing) {
      this.timeSec += dt;
      if (this.timeSec >= this.durationSec) {
        this.timeSec = this.durationSec;
        this.playing = false;
      }
    }

    const applied = [];
    for (const track of this.timeline.tracks ?? []) {
      for (const clip of track.clips ?? []) {
        const p = clipProgress(clip, this.timeSec);
        if (p === null) continue;
        const payload = clip.payload ?? {};
        const param = payload.param;
        if (
          (clip.action === "set_param" || clip.action === "render_4d") &&
          param &&
          typeof payload.from === "number" &&
          typeof payload.to === "number"
        ) {
          const value = lerp(payload.from, payload.to, p);
          if (param in renderer) {
            renderer[param] = value;
            applied.push(`${clip.id}:${param}=${value.toFixed(3)}`);
          }
        }
      }
    }
    return { timeSec: this.timeSec, playing: this.playing, applied };
  }
}
