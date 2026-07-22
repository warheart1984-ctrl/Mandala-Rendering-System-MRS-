/**
 * Temporal Replay Timeline (TRT) — parameter replay from CSR / timeline seek.
 * Status: partial — params and timeline time, not pixel-identical frames.
 */

export class ReplayService {
  constructor(cse) {
    this.cse = cse;
  }

  latestExportCsr() {
    return (
      this.cse.latestCsr("artifact.movie.export") ||
      this.cse.latestCsr("artifact.picture.export")
    );
  }

  applyParams(renderer, evidence) {
    if (!evidence) return false;
    if (typeof evidence.theta === "number") renderer.theta = evidence.theta;
    if (typeof evidence.speed === "number") renderer.speed = evidence.speed;
    if (typeof evidence.d4 === "number") renderer.d4 = evidence.d4;
    if (typeof evidence.d3 === "number") renderer.d3 = evidence.d3;
    if (typeof evidence.scale === "number") renderer.scale = evidence.scale;
    if (evidence.weights) renderer.weights = { ...evidence.weights };
    return true;
  }

  replayLastExport(renderer) {
    const csr = this.latestExportCsr();
    if (!csr) return { ok: false, reason: "No export CSR" };
    this.applyParams(renderer, csr.evidence);
    return { ok: true, csrId: csr.id, evidence: csr.evidence };
  }
}
