/**
 * Constitutional contracts — authority maps for actors/subsystems.
 * SoT under engine/ — js/constitution re-exports this module.
 */

import { CHARTER } from "./charter.js";

export const CONTRACTS = Object.freeze({
  "contract.cinematic4d.v1": Object.freeze({
    id: "contract.cinematic4d.v1",
    actor: "4dce.renderer",
    status: "enforced",
    allows: Object.freeze([
      "render.session.start",
      "render.frame.live",
      "artifact.picture.export",
      "artifact.movie.export",
      "csr.replay.params",
    ]),
    invariants: Object.freeze({
      vertexCount: CHARTER.cinematic4d.vertexCount,
      edgeCount: CHARTER.cinematic4d.edgeCount,
      mustProject: true,
    }),
  }),
  "contract.export.v1": Object.freeze({
    id: "contract.export.v1",
    actor: "4dce.export",
    status: "enforced",
    allows: Object.freeze([
      "artifact.picture.export",
      "artifact.movie.export",
      "artifact.provenance.download",
    ]),
  }),
  "contract.timeline.v1": Object.freeze({
    id: "contract.timeline.v1",
    actor: "4dce.timeline",
    status: "enforced",
    allows: Object.freeze([
      "timeline.play",
      "timeline.pause",
      "timeline.seek",
    ]),
  }),
});

export function resolveAuthority(actorId, action) {
  const contract = Object.values(CONTRACTS).find((c) => c.actor === actorId);
  if (!contract) {
    return { ok: false, reason: `No contract for actor ${actorId}` };
  }
  if (!contract.allows.includes(action)) {
    return {
      ok: false,
      reason: `Contract ${contract.id} does not authorize ${action}`,
      contractId: contract.id,
    };
  }
  return { ok: true, contractId: contract.id, contract };
}
