/**
 * Scene graph (partial) — load governed world entities into a snapshot.
 */

export class SceneGraph {
  constructor() {
    this.world = null;
    this.entities = new Map();
  }

  loadWorld(world) {
    this.world = world;
    this.entities.clear();
    for (const e of world.entities ?? []) {
      this.entities.set(e.id, structuredClone(e));
    }
    return this.sample();
  }

  getComponent(entityId, type) {
    const e = this.entities.get(entityId);
    return e?.components?.find((c) => c.type === type) ?? null;
  }

  sample() {
    return {
      worldId: this.world?.id ?? null,
      name: this.world?.name ?? null,
      constitution: this.world?.constitution ?? null,
      entities: [...this.entities.values()],
      sampledAt: new Date().toISOString(),
    };
  }

  /** Apply FourDRenderer component config onto live canvas renderer. */
  bindFourDRenderer(renderer) {
    const comp = this.getComponent("tesseract-hero", "FourDRenderer");
    if (!comp?.config) return null;
    const c = comp.config;
    if (typeof c.d4 === "number") renderer.d4 = c.d4;
    if (typeof c.d3 === "number") renderer.d3 = c.d3;
    if (typeof c.scale === "number") renderer.scale = c.scale;
    if (typeof c.speed === "number") renderer.speed = c.speed;
    if (typeof c.surfaceId === "string" && typeof renderer.setSurface === "function") {
      renderer.setSurface(c.surfaceId, c.resolution ?? null);
    }
    if (typeof c.renderMode === "string") renderer.renderMode = c.renderMode;
    return c;
  }
}
