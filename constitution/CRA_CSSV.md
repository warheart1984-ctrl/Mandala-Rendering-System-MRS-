# CRA Section: CSSV Storage & Query Layer

> Part of the Constitutional Reference Architecture. See `constitution/CSSV.md` for the mathematical model.

## Role

CSSV is the CRA's canonical ledger of constitutional computation. It stores:

- **Artifacts** — worlds, timelines, intents, policies
- **Transitions** — governed state changes (edges in CSSV)
- **Frames** — runtime provenance samples (trajectory points)

## Storage format

```
cssv/
  artifacts.json      # static artifacts (JSON array)
  transitions.ndjson  # append-only governed transitions
  frames.ndjson       # append-only frame provenance
```

### Artifact record

```json
{
  "type": "artifact",
  "id": "world-mythar-plains",
  "artifactType": "world",
  "host": "browser",
  "timestamp": 1721580000.123,
  "payload": { "dto": "GovernedWorldDto", "data": {} }
}
```

### Transition record

```json
{
  "type": "transition",
  "id": "tx-0001",
  "from": "state-001",
  "to": "state-002",
  "intent": { "id": "intent-ascension", "timeline": "mythar_ascension" },
  "authority": "runtime.browser",
  "evidence": ["ev-ascension-001", "ev-ascension-002"],
  "decision": { "allowed": true, "reasons": [] },
  "host": "browser",
  "timestamp": 1721580001.551
}
```

### Frame record

```json
{
  "type": "frame",
  "intent": "intent-ascension",
  "timeline": "mythar_ascension",
  "world": "world-mythar-plains",
  "host": "browser",
  "timestamp": 1721580001.6,
  "params": { "speed": 1.0, "d4": 4.0, "d3": 4.0 },
  "frameIndex": 120
}
```

## Constitutional Query Language (CQL)

CQL slices CSSV trajectories for audit, drift analysis, and host comparison.

```
SELECT <fields>
FROM <artifact|transition|frame>
WHERE <predicate>
ORDER BY <field> ASC|DESC
LIMIT <n>
```

### Example queries

**Mythar Ascension frames:**
```
SELECT frames FROM frame
WHERE frame.timeline = "mythar_ascension"
ORDER BY frame.timestamp ASC
```

**Denied transitions (missing evidence):**
```
SELECT transitions FROM transition
WHERE decision.allowed = false
```

**Unity vs Unreal host comparison:**
```
SELECT frames FROM frame
WHERE frame.timeline = "mythar_ascension" AND host = "unity"
```

## CRA layer mapping

| CRA Layer | CSSV responsibility |
| --- | --- |
| Constitution | Defines topology and invariants of \(S\) and \(\mathcal{T}\) |
| Specification | DTOs, transition algebra, projection operators |
| Conformance | Host projections must respect CSSV + CKL before admitting \(\tau\) |
| Implementation | Loaders, schedulers, registry, CQL, dashboard |
| Deployment | Which CSSV views are active per environment |
| Stewardship | Archival, replay, policy evolution affecting \(\mathcal{T}\) |

## API namespace

All hosts register into CSSV via:

- **C#:** `SovereignX.CIEMS.Engine.CSSV.ICssvRegistry`
- **C++:** `SovereignX::CIEMS::Engine::CSSV::ICssvRegistry`
- **JS:** `engine/cssv/CssvRegistry.js`

## Evidence-bound claims

Claims such as *"Mythar Ascension obeyed dual-evidence and drift-throttle policies"* are statements about a trajectory \(\gamma\) in CSSV, backed by CKL evaluation and recorded provenance, verifiable across hosts sharing the same ledger representation.
