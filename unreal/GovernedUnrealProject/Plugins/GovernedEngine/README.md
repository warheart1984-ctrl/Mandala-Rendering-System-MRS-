# Unreal — Governed Engine Plugin

**Status: skeleton** (plugin + UE 5.8 project scaffold; compile requires VS2022 + Windows 10 SDK).

Browser JS under `/engine/` remains the authoritative Phase-1 runtime.

## Quick start (UE 5.8)

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-unreal.ps1
# After Visual Studio 2022 + Windows 10 SDK installed:
powershell -ExecutionPolicy Bypass -File scripts/build-unreal.ps1
```

Open `unreal/GovernedUnrealProject/GovernedUnrealProject.uproject` in Unreal Editor.

See `unreal/GovernedUnrealProject/README.md` for PIE scene setup.

## Option B (implemented as code — not Sequencer-authored)

Governed timeline JSON is the source of truth. Sequencer is optional for visualization only.

| Piece | Location |
| --- | --- |
| Shared DTOs | `engine/dto/*.h` |
| Binding model | `engine/runtime/GovernedBinding.h` + `Private/BindingResolver.cpp` |
| Timeline scheduler | `engine/runtime/TimelineScheduler.h` + `Private/TimelineScheduler.cpp` |
| World loader | `engine/world/GovernedWorldLoader.h` + `Private/GovernedWorldLoader.cpp` |
| Timeline store | `engine/timeline/GovernedTimelineStore.h` + `Private/GovernedTimelineStore.cpp` |
| Provenance | `engine/runtime/FrameProvenance.h` + tick in `FourDRendererComponent` |
| Replay | `engine/runtime/ReplayService.h` + `Public/UnrealReplayTarget.h` |
| ISL → CKL → execute | existing `IslIntentBootstrapActor` / `FExecutionOrchestrator` |

## Content

- `Content/Timelines/opening_4d_reveal.timeline.json`
- `Content/Timelines/mythar_ascension.timeline.json`
- `Content/Governed/default.policies.json` (incl. Ascension dual-evidence + drift throttle)

## Movie pipeline (partial)

| Path | Command / entry | Output |
| --- | --- | --- |
| PNG (runtime) | PIE + `GovernedEngine.MakeMovie` | `Saved/Movies/<session>/frame_*.png` |
| ProRes container (Editor) | `GovernedEngine.MakeMovieMRQ` | `Saved/Movies/<session>/*.mov` via Movie Render Queue |

Requires `MovieRenderPipeline` plugin (enabled in `.uproject` / plugin deps) and `UGovernedMovieCaptureComponent` for the PNG path.

CKL must allow `artifact.movie` (actor + evidence per default policies).

Do not claim Sequencer integration or PIE/MRQ verification until exercised in-editor.
