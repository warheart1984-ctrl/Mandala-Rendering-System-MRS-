# Constitutional State Space Visualization (CSSV)

> **Drive-G-1:** Claims below are labeled by evidence. Implementation status: **partial** (browser in-memory registry + Node CQL server).

## Definition

CSSV is a four-dimensional spatiotemporal visualization of constitutional artifacts, authority, evidence, provenance, state transitions, and runtime execution within the Constitutional Reference Architecture (CRA).

It is **host-agnostic**: Unity, Unreal, browser, and VR are projections of the same constitutional state space—not owners of it.

## State space

Base space: let \(X \subset \mathbb{R}^3\) be spatial coordinates and \(T \subset \mathbb{R}\) be time.

\[
S = X \times T
\]

A point \(s = (x, y, z, t) \in S\) is a **constitutional state**.

### Constitutional fields over \(S\)

| Field | Map | Codomain |
| --- | --- | --- |
| Artifact | \(A : S \to \mathcal{A}\) | constitutions, policies, intents, timelines, CKL, ISL, CSR |
| Authority | \(\alpha : S \to \mathcal{U}\) | principals, roles, engines, hosts |
| Evidence | \(E : S \to \mathcal{E}\) | evidence bundles |
| Provenance | \(P : S \to \mathcal{P}\) | lineage, decisions, executions |
| Runtime | \(R : S \to \mathcal{R}\) | host configurations (browser, Unity, Unreal, VR) |

Fields may be **sparse**: many points carry no artifact, evidence, or runtime attachment.

## Transitions

A constitutional transition is a governed change:

\[
\tau : S \to S,\quad \tau(s) = s'
\]

with transition record:

\[
\Theta = (\text{intent}, \text{authority}, \text{evidence}, \text{decision}, \text{provenance})
\]

Only transitions satisfying CRA constraints are admitted into transition set \(\mathcal{T}\):

- **Intent-driven** — every \(\tau\) justified by intent \(I \in \mathcal{A}\)
- **Authority-bounded** — \(\alpha(s)\) may enact \(\tau\)
- **Evidence-supported** — \(E(s)\) meets CKL requirements
- **Provenance-recorded** — \(P(s')\) includes \(\Theta\)

## Transition algebra

- **Composition:** \(\tau_2 \circ \tau_1 \in \mathcal{T}\) when output of \(\tau_1\) is valid input to \(\tau_2\)
- **Identity:** \(\mathrm{id}_s(s) = s\) (observation without mutation)
- **Guarded:** \(\tau \in \mathcal{T} \iff \forall p \in \mathrm{CKL},\, p(\Theta)\) satisfied
- **Trajectory equivalence:** paths \(\gamma_1, \gamma_2\) equivalent if intents, decisions, and provenance match up to isomorphism across hosts

## Projection operators

| Host | Projection |
| --- | --- |
| Unity | \(\pi_{\mathrm{Unity}} : S \times \mathcal{A} \times \mathcal{P} \to U_{\mathrm{Scene}}\) |
| Unreal | \(\pi_{\mathrm{Unreal}} : S \times \mathcal{A} \times \mathcal{P} \to U_{\mathrm{Level}}\) |
| Web | \(\pi_{\mathrm{Web}} : S \times \mathcal{A} \times \mathcal{P} \to \mathrm{BrowserCanvas}\) |
| VR | \(\pi_{\mathrm{VR}} : S \times \mathcal{A} \times \mathcal{P} \to \mathrm{VRWorld}\) |

### 4D cinematic projection

\[
\Pi_{4D} : \Gamma \to \mathcal{C}
\]

Maps constitutional trajectories \(\gamma\) to cinematic configurations (camera, 4D rotation, projection params). The browser `TimelinePlayer` and Unity/Unreal schedulers are numerical implementations of \(\Pi_{4D}\).

## Implementation evidence

| Artifact | Path | Status |
| --- | --- | --- |
| CSSV registry (JS) | `engine/cssv/CssvRegistry.js` | **partial** — browser in-memory |
| Ledger loader | `engine/cssv/ledger.js` | **enforced** (Node) |
| CQL parser/interpreter | `engine/cssv/cqlParser.js`, `cqlInterpreter.js` | **partial** |
| C# mirror | `engine/CSSV/CssvLedger.cs` | **skeleton** |
| C++ mirror | `engine/CSSV/CssvLedger.h` | **skeleton** |
| Ledger files | `cssv/artifacts.json`, `transitions.ndjson`, `frames.ndjson` | **declared** |
| Dashboard | `cssv/server.js`, `cssv/public/` | **partial** |
| Browser wiring | `js/engine/boot.js`, `js/app.js` | **partial** |
