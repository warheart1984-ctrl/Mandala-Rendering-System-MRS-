/**
 * Export surface meshes as JSON for Unity/Unreal hosts.
 * SoT remains 4d-renderer sampleSurface; hosts load wireframe-ready meshes.
 *
 * Usage: node scripts/export-surface-meshes.mjs [--resolution 24]
 */

import { mkdirSync, writeFileSync } from "node:fs";
import { createHash } from "node:crypto";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const pkg = path.join(root, "4d-renderer");
const { listSurfaces, getSurface, sampleSurface } = await import(
  pathToFileURL(path.join(pkg, "src/index.js")).href,
);

const resArg = process.argv.find((a) => a.startsWith("--resolution"));
const resolution = resArg
  ? Number(resArg.split("=")[1] ?? process.argv[process.argv.indexOf(resArg) + 1])
  : 24;

const outDir = path.join(root, "engine/surfaces/meshes");
mkdirSync(outDir, { recursive: true });

const index = [];

for (const { id, name } of listSurfaces()) {
  const surface = getSurface(id);
  const mesh = sampleSurface(surface, resolution);
  const payload = {
    contractVersion: "1.0",
    id,
    name,
    source: "4d-renderer",
    resolution: typeof surface.sample === "function" ? null : resolution,
    vertexCount: mesh.vertices.length,
    edgeCount: mesh.edges.length,
    faceCount: mesh.faces?.length ?? 0,
    vertices: mesh.vertices.map((v) => ({
      x: round4(v.x),
      y: round4(v.y),
      z: round4(v.z),
      w: round4(v.w),
    })),
    edges: mesh.edges,
    faces: mesh.faces ?? [],
  };
  payload.bounds4D = bounds4D(payload.vertices);
  payload.contentHash = createHash("sha256").update(JSON.stringify({ id:payload.id,vertices:payload.vertices,edges:payload.edges,faces:payload.faces })).digest("hex");
  const fileName = `${id}.mesh.json`;
  writeFileSync(path.join(outDir, fileName), JSON.stringify(payload));
  index.push({
    id,
    name,
    file: fileName,
    vertices: payload.vertexCount,
    edges: payload.edgeCount,
    faces: payload.faceCount,
  });
  console.log(
    `✓ ${id} — ${payload.vertexCount} verts, ${payload.edgeCount} edges, ${payload.faceCount} faces`,
  );
}

writeFileSync(
  path.join(outDir, "index.json"),
  JSON.stringify({ source: "4d-renderer", resolution, surfaces: index }, null, 2),
);
console.log(`\nExported ${index.length} meshes → ${outDir}`);

function round4(n) {
  return Math.round(n * 1e6) / 1e6;
}

function bounds4D(vertices) {
  const axes=["x","y","z","w"],min={},max={};
  for(const axis of axes){min[axis]=Math.min(...vertices.map((v)=>v[axis]));max[axis]=Math.max(...vertices.map((v)=>v[axis]));}
  return {min,max};
}
