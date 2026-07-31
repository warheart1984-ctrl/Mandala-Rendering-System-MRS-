# Constitutional Anime Rendering — CECP Trail

| Field | Value |
|-------|-------|
| `trailId` | `constitutional-anime-rendering-2026-07` |
| `feature` | Product entry-point: governed anime / cel stylization |
| `started` | 2026-07-31 |
| `overallStatus` | **partial** (anime look lane) + **skeleton** (AnimeWorldProfile) + **declared** (shot enforcement) |
| `mode` | Sage + Visionary |
| `softwareCreationMode` | Schema-Artist + Pipeline-Conductor |
| `lineage` | Architecture → Build → Implementation → Review → Inspection → ESFR |

## Mission lock (entry-point thesis)

**Constitutional Anime Rendering** is the correct product entry point — not a
photorealism apology. Hardware limits become design decisions; the engine
deliberately produces **governed stylization**. Studios understand: same character,
palette, lighting, line, and continuity constraints.

Hybrid of: cinematic cel shading · volumetric mist/light · strong silhouettes ·
controlled line weight · 3D environments · 2D-inspired characters · governed style
profiles · deterministic shot replay · 4D geometry for portals/spirits/impossible
architecture.

## Stage files

| Stage | File | Status |
|-------|------|--------|
| 01 Architect | [01-architect-adr.md](./01-architect-adr.md) | complete |
| 02 Builder | [02-builder-scaffold-manifest.md](./02-builder-scaffold-manifest.md) | complete |
| 03 Implementor | [03-implementor-notes.md](./03-implementor-notes.md) | complete (thin scaffold) |
| 04 Reviewer | [04-reviewer-conformance.md](./04-reviewer-conformance.md) | complete |
| 05 Inspector | [05-inspector-acceptance.md](./05-inspector-acceptance.md) | complete |
| 06 ESFR | [06-engineer-standards.md](./06-engineer-standards.md) | **PASS_WITH_GAPS** / **PROMOTE_WITH_GAPS** |

## Design + schema

- [design/ANIME_WORLD_PROFILE.md](./design/ANIME_WORLD_PROFILE.md) — field contract + binding map
- Schema: `schemas/anime/AnimeWorldProfile.v1.schema.json` (**skeleton**)
- Example: `schemas/anime/examples/mandala-cel-v1.example.json` (**skeleton**)
- Validator: `mrs/apps/genblaze-media/app/anime_world_profile.py` (**skeleton**)

## Related trails (do not invent parallel systems)

| Trail / module | Role | Status |
|----------------|------|--------|
| `ink-cel-render-lane-2026-07` | Engine3D soft-raster cel + ink AOV design | **partial** (design) |
| Genblaze `style_steer.py` (`fa33fac` lineage) | Diffusion/polish anime prompt steer | **partial** |
| `world-engine-probe-2026-07` | Amendment VII soft gates; Amendment VIII world profiles | VII **partial**; world engine HOLD |
| Photoreal / Cycles / CPCS trails | Optional side path | optional — not entry point |

## Honest non-claims

- Not Full Photoreal
- Not Digital Printer beauty SoT (NIM/FLUX anime = creative assist)
- Not CKL-enforced shot gate (enforcement **declared**)
- Lemonade SD / `sd-server` held on this AMD host when `pixelsProduced` is false
- Unity / Unreal host consumption **declared** / skeleton
- Photoreal Cycles remains available; do not delete that path

## Invoke current anime lane

```bash
# Env default
set GENBLAZE_STYLE=anime

# API
curl -s -X POST http://127.0.0.1:8787/api/generate \
  -H "content-type: application/json" \
  -d '{"prompt":"oracle mask grown from metallic mandala petals","style":"anime"}'

# Profile field validation (offline)
cd mrs/apps/genblaze-media && python -m pytest tests/test_anime_world_profile.py tests/test_style_steer.py -q
```
