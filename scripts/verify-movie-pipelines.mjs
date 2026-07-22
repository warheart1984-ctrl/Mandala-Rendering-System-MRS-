/**
 * Verify Unity/Unreal movie pipeline source files exist and declare key APIs.
 */

import { existsSync, readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

const checks = [
  {
    file: "unity/GovernedUnityProject/Assets/Engine/Runtime/GovernedMovieCapture.cs",
    mustInclude: ["StartGovernedRecord", "TryContainerEncode", "movie-manifest.json"],
  },
  {
    file: "unity/GovernedUnityProject/Assets/Engine/Editor/GovernedMovieRecorderBridge.cs",
    mustInclude: ["RecorderController", "MovieRecorderSettings", "CoreEncoderSettings"],
  },
  {
    file: "unity/GovernedUnityProject/Packages/manifest.json",
    mustInclude: ["com.unity.recorder"],
  },
  {
    file: "unity/GovernedUnityProject/Assets/Engine/Runtime/ExecutionOrchestrator.cs",
    mustInclude: ["IsMovieIntent", "artifact.movie", "movieCapture"],
  },
  {
    file: "unreal/GovernedEnginePlugin/Source/GovernedEngine/Private/GovernedMovieCapture.cpp",
    mustInclude: ["StartGovernedRecord", "ReadPixels", "movie-manifest.json"],
  },
  {
    file: "unreal/GovernedEnginePlugin/Source/GovernedEngine/Private/ExecutionOrchestrator.cpp",
    mustInclude: ["ExecuteMovie", "IsMovieIntent", "FGovernedMovieCapture::TryStart"],
  },
  {
    file: "unreal/GovernedEnginePlugin/Source/GovernedEngineEditor/Private/GovernedMovieRenderQueue.cpp",
    mustInclude: ["StartProResCapture", "MoviePipelineAppleProResOutput", "prores-mov"],
  },
  {
    file: "unity/GovernedUnityProject/Assets/Engine/Rendering/SurfaceMeshLoader.cs",
    mustInclude: ["TryLoad", "StreamingAssets"],
  },
  {
    file: "unreal/GovernedEnginePlugin/Source/GovernedEngine/Private/FourDRendererComponent.cpp",
    mustInclude: ["LoadMeshFromJson", "ReloadMesh", ".mesh.json"],
  },
  {
    file: "unreal/GovernedEnginePlugin/Source/GovernedEngine/Public/FourDRendererComponent.h",
    mustInclude: ["SurfaceId", "SetSurface"],
  },
];

let failed = 0;
for (const c of checks) {
  const full = path.join(root, c.file);
  if (!existsSync(full)) {
    console.error(`✗ missing ${c.file}`);
    failed++;
    continue;
  }
  const text = readFileSync(full, "utf-8");
  const missing = c.mustInclude.filter((s) => !text.includes(s));
  if (missing.length) {
    console.error(`✗ ${c.file} missing: ${missing.join(", ")}`);
    failed++;
  } else {
    console.log(`✓ ${c.file}`);
  }
}

if (failed) {
  process.exit(1);
}
console.log("\n✅ Movie pipeline sources verified.");
