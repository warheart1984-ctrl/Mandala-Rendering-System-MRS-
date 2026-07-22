/**
 * Evidence Service — acquire and lightly validate evidence bundles.
 */

export class EvidenceService {
  fromRenderer(renderer, extras = {}) {
    const surfaceId = renderer.surfaceId ?? "tesseract";
    return {
      id: `evidence-${Date.now().toString(36)}`,
      source: "renderer",
      claims: [`surface:${surfaceId}`, "projection-params", "4d-renderer"],
      timestamp: new Date().toISOString(),
      surfaceId,
      vertexCount: renderer.vertices4D.length,
      edgeCount: renderer.edges.length,
      theta: renderer.theta,
      d4: renderer.d4,
      d3: renderer.d3,
      speed: renderer.speed,
      scale: renderer.scale,
      weights: { ...renderer.weights },
      ...extras,
    };
  }

  fromWorld(world) {
    return {
      id: `evidence-world-${world.id}`,
      source: "world",
      claims: [`world:${world.id}`, `constitution:${world.constitution}`],
      timestamp: new Date().toISOString(),
      worldId: world.id,
      entityCount: world.entities?.length ?? 0,
      assetCount: world.assets?.length ?? 0,
    };
  }
}
