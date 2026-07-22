# Mythar4DScene setup (Unity)

`.unity` scene binaries cannot be fabricated as text. Create the scene once in the Editor:

1. Open folder `unity/GovernedUnityProject` as a Unity project (2021.3+ LTS recommended).
2. Create scene `Assets/Demo/Scenes/Mythar4DScene.unity`.
3. Add empty GameObject **GovernedHost** with components:
   - `IslIntentBootstrap`
   - `ExecutionOrchestratorHost`
   - `TimelineExecutor`
   - `TimelineScheduler`
   - `GovernedTimelineStore`
   - `BindingResolver`
4. On `IslIntentBootstrap`: assign `Assets/Demo/Isl/Opening4DReveal.isl` to **Isl Script**; leave default `contextJson`.
5. Wire references: Bootstrap → Orchestrator; Orchestrator → TimelineExecutor; Executor → Store, Scheduler, Bindings.
6. Add GameObject **TesseractHero** with `FourDTesseractRenderer` (`bindingName` = `tesseract-hero`).
7. Ensure Main Camera exists; optionally add `GovernedBinding` with `bindingName` = `camera-main`.
8. Confirm StreamingAssets: `timelines/opening_4d_reveal.timeline.json`, `policies/default.policies.json`.
9. Enter Play Mode — Console should show ISL compile + CKL allow + TimelineExecutor Play.

**Status:** skeleton until Play Mode is verified on a local Unity install.
