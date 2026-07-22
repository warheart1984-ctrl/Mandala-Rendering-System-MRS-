/**
 * Static verification: Unity/Unreal adapter files declare all profile check ids.
 * Does not execute host runtimes — complements browser Node conformance test.
 */

import { readFile } from "node:fs/promises";
import { readFileSync, existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

const profilePath = path.join(root, "engine/conformance/default.conformance-profile.json");
const profile = JSON.parse(await readFile(profilePath, "utf-8"));
const checkIds = profile.checks.map((c) => c.id);

const hostFiles = [
  "unity/GovernedUnityProject/Assets/Engine/Shared/Conformance/UnityRuntimeAdapter.cs",
  "unreal/GovernedEnginePlugin/Source/GovernedEngine/Private/UnrealRuntimeAdapter.cpp",
];

let failed = 0;

for (const rel of hostFiles) {
  const full = path.join(root, rel);
  if (!existsSync(full)) {
    console.error(`✗ missing adapter file: ${rel}`);
    failed++;
    continue;
  }
  const text = readFileSync(full, "utf-8");
  const missing = checkIds.filter((id) => !text.includes(`"${id}"`) && !text.includes(`'${id}'`));
  if (missing.length) {
    console.error(`✗ ${rel} missing probes: ${missing.join(", ")}`);
    failed++;
  } else {
    console.log(`✓ ${rel} declares all ${checkIds.length} probes`);
  }
}

if (failed) {
  console.error(`\n${failed} host adapter file(s) incomplete.`);
  process.exit(1);
}

console.log("\n✅ Host probe coverage verified.");
process.exit(0);
