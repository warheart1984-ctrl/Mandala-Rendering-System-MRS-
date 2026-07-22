# Unreal — Governed Engine setup

Requires **Unreal Engine 5.8** (detected at `C:\Program Files\Epic Games\UE_5.8`).

## One-time setup

From repo root:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-unreal.ps1
```

This creates a plugin junction under `unreal/GovernedUnrealProject/Plugins/GovernedEngine`.

## Build (command line)

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-unreal.ps1
```

Or open `unreal/GovernedUnrealProject/GovernedUnrealProject.uproject` in the Epic Launcher / Unreal Editor and allow it to compile.

## Play-in-Editor scene

1. Open **GovernedUnrealProject** in UE 5.8.
2. Create or open a level.
3. Add an actor with **FourDRendererComponent** (name/tag: `tesseract-hero`).
4. Add **IslIntentBootstrapActor** — set ISL path to plugin content:
   `Plugins/GovernedEngine/Content/Scripts/Opening4DReveal.isl`
5. Press **Play** — ISL → CKL → governed timeline should run; tesseract draws via `DrawDebugLine`.

Plugin timelines live in `Plugins/GovernedEngine/Content/Timelines/`.

## Conformance

Browser passes 16/16 via `npm run test:conformance`. Unreal host adapters are **planned** — PIE verification is the next proof step after a clean compile.
