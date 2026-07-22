/**
 * ReplayService — re-drive a target from recorded provenance frames.
 */

export class ReplayService {
  static replay(frames, target) {
    if (!frames || !target?.applyFrame) return;
    for (const frame of frames) {
      target.applyFrame(frame);
    }
  }
}
