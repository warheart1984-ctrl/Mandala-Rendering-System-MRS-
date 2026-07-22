# 4DCE — Constitutional Engine

Governed 4D cinematic runtime with portable constitutional evidence across Browser, Unity, and Unreal hosts.

**Namespace:** `SovereignX.CIEMS.Engine.*`  
**Evidence bound:** see `constitution/CHARTER.md` for enforced vs partial vs skeleton claims.

## Quick start (production dev stack)

Requires **Node.js 20+**.

```bash
npm test          # all smoke tests (conformance, CQL, CKL)
npm start         # browser :8080 + CSSV dashboard :3000
```

| Service | URL | Purpose |
|---------|-----|---------|
| Browser host | http://localhost:8080/ | 4D renderer + governed cinematics |
| CSSV dashboard | http://localhost:3000/ | CQL queries, trajectory charts |
| CSSV health | http://localhost:3000/health | Liveness probe |
| CSSV ingest | POST http://localhost:3000/ingest | Persist browser session ledger |

Open the browser host, play **Opening 4D Reveal** or **Mythar Ascension**, then click **Download CSSV ledger**. The session syncs to the CSSV server when it is running (`npm start`).

The renderer exposes all five surfaces, four visual profiles, adaptive performance/high/ultra
quality, combined solid and wireframe output, drag navigation, wheel zoom, Space to pause, and
R to reset. WebGPU uses normal lighting and a validated packed-uniform contract, with Canvas as
the deterministic fallback. Native dispatch supports the resident Sovereign X Vulkan daemon
and governed `AbortSignal` cancellation.

For live native presentation, set `SOVEREIGNX_SHARED_FRAME_PATH` to the worker's
`sharedFramePath`, start the browser host, and open `/?nativePreview=1`. The server publishes
only the active slot of the double-buffered SXFR ring; the browser validates its header and
sequence before presenting RGBA pixels.

## Architecture

```
ISL intent → CKL/GK decision → TimelinePlayer → Frame provenance → CSSV ledger
                     ↓
            Conformance profile (16 checks per host)
```

- **Engine SoT:** `engine/` — governance, DTOs, runtime, CSSV, conformance
- **Browser glue:** `js/` — CSE, boot, renderer
- **CSSV ledger:** `cssv/` — artifacts.json + transitions.ndjson + frames.ndjson
- **Unity / Unreal:** skeleton hosts until Play-in-Editor verified

## Scripts

| Command | Description |
|---------|-------------|
| `npm test` | Full smoke test suite |
| `npm run test:conformance` | 16-check browser conformance profile |
| `npm run test:cql` | CQL parser + interpreter |
| `npm run test:ckl` | Mythar Ascension CKL policies |
| `npm run init:cssv` | Initialize empty ledger files |
| `npm run serve` | Static browser host only |
| `npm run cssv:server` | CSSV dashboard + API only |
| `npm start` | Both servers |

## Conformance

Every runtime must satisfy the canonical profile in `engine/conformance/default.conformance-profile.json`:

- Provenance, Replay, Binding, Timeline, Evidence, CKL (16 checks)

Browser: **verified** via `npm run test:conformance`.  
Unity / Unreal: adapters **planned** — hosts remain **skeleton**.

## CSSV + CQL

Constitutional State Space Visualization stores artifacts, transitions, and frame provenance in a host-agnostic ledger. Query with CQL:

```sql
SELECT frames
FROM frame
WHERE frame.timeline = "mythar_ascension"
ORDER BY frame.timestamp ASC
LIMIT 1000
```

See `constitution/CSSV.md` and `constitution/CRA_CSSV.md`.

## Unreal (UE 5.8)

Detected at `C:\Program Files\Epic Games\UE_5.8`. C++ compile requires **Visual Studio 2022** with Desktop C++ workload and **Windows 10 SDK**.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-unreal.ps1
powershell -ExecutionPolicy Bypass -File scripts/build-unreal.ps1   # after SDK installed
```

Open `unreal/GovernedUnrealProject/GovernedUnrealProject.uproject` — see `unreal/GovernedUnrealProject/README.md` for PIE setup.

## Deployment notes

- Serve the repo root over HTTP (ES modules require it — do not open `index.html` as `file://`).
- Run CSSV server alongside the browser host for ingest + dashboard.
- Unity/Unreal: copy `engine/` shared sources into host projects; verify namespace references.

## Evidence map

Full artifact index: `constitution/CHARTER.md` § Evidence map.
