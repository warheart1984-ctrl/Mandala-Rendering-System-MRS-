# Mythar – Opening 4D Reveal (demo notes)

**Status:** Browser demo is **enforced** via `demo/worlds/mythar_plains.world.json` + `demo/timelines/opening_4d_reveal.timeline.json`. Unity `.unity` / Unreal `.umap` scene files are **declared** (create in editor from skeletons).

## Tracks (browser)

1. Camera `d3` dolly: 5.5 → 3.2 over 0–5s  
2. Tesseract `speed` ramp: 0.4 → 1.6 over 2–10s  
3. `d4` breathe: 3.5 → 5.0 over 0–12s  

## How to run

1. Serve repo root (`python -m http.server 8765`)
2. Open http://127.0.0.1:8765/
3. Click **Play Opening 4D Reveal**
4. Optionally **Make movie** while timeline plays

## Engine-native movie (Unity / Unreal — partial)

| Host | How | Output |
| --- | --- | --- |
| Browser | **Make movie** button | WebM download |
| Unity | `GovernedMovieBootstrap` (Play Mode); prefers Unity Recorder MP4 when package present | `persistentDataPath/Movies/<session>/` MP4 or PNG |
| Unreal | PIE `GovernedEngine.MakeMovie` (PNG) or Editor `GovernedEngine.MakeMovieMRQ` (ProRes) | `Saved/Movies/<session>/` |

Optional ffmpeg mux remains for PNG sessions (`muxHint` / Unity **Mux Movie Sequence**).
