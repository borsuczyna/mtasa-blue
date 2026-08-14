# Shader API Upgrade — Progress Tracker

Status legend: `pending` | `in-progress` | `done` | `blocked`

This is a living current-state snapshot, updated in place as work lands. It has no historical log (see commit history for that). Companion document: [shader-api-upgrade-plan.md](shader-api-upgrade-plan.md).

## Architecture prerequisites

- [x] CRenderStateScope (done — `Client/core/Graphics/CRenderStateScope.h/.cpp`)
- [x] Device-loss/resource-stop integration (done — new item types plug into the existing `CRenderItemManager::OnLostDevice`/`OnResetDevice` fan-out and `CClientRenderElement`/`SetParent` ownership; no parallel mechanism added)
- [x] Recursive-render guard (done — secondary world rendering rejects re-entry and consumes the queued flag before entering GTA rendering)

## Stage 1 — Capability reporting, diagnostics, accounting, MRT, custom depth, render passes, one scene view

- [x] 5. Capability reporting (`SDxCapabilities`, `dxGetRenderCapabilities`) (done)
- [x] 6. Shader compile diagnostics (done — `dxGetShaderDiagnostics`, an optional third `dxCreateShader` result, technique validation, parameter reflection, profiles, source identifier and retained compile log)
- [ ] 6A. Extended shader language/compiler path (pending — opt-in final options table planned; legacy D3DX Effect compilation remains the default. Research must distinguish parser limitations, SM2 static-flow limits and SM3 dynamic-loop support before implementation)
- [x] 7. Render statistics / resource accounting (done for durable counters and GPU-object/memory snapshots via `dxGetRenderStatistics`; asynchronous GPU timings and per-frame switch counters remain a later profiling milestone)
- [ ] 8. Automatic shader value bindings (in-progress — annotation-only `mtaSemantic` bindings now cover inverse/transpose matrices, camera right/up, viewport size and render-target size plus inverse dimensions; legacy implicit names remain unchanged. Near/far clip, frame timing and environment values remain pending)
- [x] 9. Typed render targets (format validation) (done — `CheckDeviceFormat` guard in `CRenderItemManager::CreateRenderTarget`)
- [x] 10. Custom depth-stencil targets (done — `CDepthStencilTargetItem`/`CClientDepthStencilTarget`/`dxCreateDepthStencilTarget`; **sampleable path explicitly not implemented**, rejected with a clear error)
- [x] 11. MRT binding + validation (done — `CMrtSetItem`/`CClientMrtSet`/`dxCreateMrtSet`)
- [x] 12. Safe scoped render-pass infrastructure (done — `dxBeginRenderPass`/`dxEndRenderPass`, backed by `CRenderStateScope`; defensively force-closed in `CDirect3DEvents9::OnPresent` and `CRenderItemManager::OnLostDevice`, matching the existing `RestoreDefaultRenderTarget()` "in case script forgets" pattern. Known limitation: the open-pass stack is global, not per-resource, so one resource could in principle call `dxEndRenderPass` while another resource's pass is open — same trust model as the existing single-slot `dxSetRenderTarget`, not a new class of risk, but not yet hardened further)
- [x] 13. One independent scene view proof of concept (done — `DxSceneView` provides camera setup, queued rendering and a one-view Stage-1 cap. The secondary GTA world is rendered through private RenderWare color/depth rasters with a full RW camera update lifecycle, then copied into the script-visible texture. In-game testing confirmed independent static-world, vehicle and ped rendering without framebuffer leakage. Commit: `cee4fdc5c`. Follow-up fidelity fix: SceneViews render through `RenderScene()` only (deliberately, to avoid GTA's monolithic `RenderEffects()` and its shared particle/corona queues), which also skips `RenderEffects()`'s final `CPostEffects::ColourFilter()` step - a real per-frame full-screen tint driven by `CTimeCycle`'s current PostFx colours that the primary view always receives. This left every SceneView consistently duller than the primary camera. `ApplySecondarySceneColourFilter` reproduces the same additive blend against a private per-view scratch texture (not GTA's shared, primary-screen-sized `pRasterFrontBuffer`), scoped with a D3D9 state block so RenderWare's fixed-function state cache is never left stale. Commit: `37d122ebe`)
- [x] 14. Camera/state and RenderWare-resource validation (done for the Stage-1 scope — persistent RW rasters now have explicit `CMultiplayerSA` ownership, are released when the final SceneView disappears and before D3D9 reset, and are recreated lazily. Scoped camera-raster cleanup closes an active RW update and restores original camera rasters on every normal exit. Debug Win32 compilation and in-game resource restart, stop/start and repeated alt-tab tests passed. Runtime resolution switching is intentionally not handled because MTA does not expose it in game; target dimensions are handled when views are recreated.)

### Verification
Debug Win32 compilation succeeds. The `dx9_foundation_test` resource passed in-game capability, custom-depth, render-pass and true MRT tests on hardware reporting four simultaneous render targets; a shader produced visibly distinct `COLOR0` and `COLOR1` outputs. The same resource proves a genuinely independent RenderWare-backed world view while leaving the primary camera output intact. Resource restart, stop/start and repeated alt-tab/device-reset tests also pass after explicit RW raster lifecycle integration.

## Sky shaders

- [ ] Conditional sky-render trampoline (pending)
- [ ] `engineApplyShaderToSky` / `engineRemoveShaderFromSky` (pending)

## Stage 2 — Multiple scheduled scene views

- [x] Raise scene-view cap + update modes (done for the current two-view scope — the shared create/render/capability limit is two and both views drain sequentially at the safe pre-`ConstructRenderList` point. Debug Win32 builds, two-camera isolation and throttled-mode tests pass. `dxSetSceneViewUpdateMode` supports `manual`, `once`, `always`, `every_n_frames` and millisecond `interval`. The public output retains its last complete frame until replacement copying finishes. Resolution and pixel count do not affect scheduling.)
- [x] Isolated per-SceneView world-material shader assignments that ignore primary/other-view shader maps (done — scoped alternate matching, resource-owned shader references and apply/remove Lua entry points passed Debug builds and two-view visual isolation, restart and device-reset tests. Commit: `bda1eac90`. Target-element filtering (the plan's deferred follow-up, mirroring `engineApplyShaderToWorldTexture`'s own `targetElement`/`appendLayers`) has since been added: `SSceneViewShaderAssignment` carries an optional target entity resolved against a new `CRenderWare::GetRenderingClientEntity()` getter, with the same appendLayers exclusivity semantics as the world-texture path. The `dxApplyShaderToSceneViewWorldTexture`/`dxRemoveShaderFromSceneViewWorldTexture` development aliases mentioned in the plan have been removed now that the signatures reached parity - only the `engine*` names exist. Commit: `dd7038bee`)
- [x] SceneView output post-process chain using safe render passes and ping-pong targets; `dxGetSceneViewTexture` remains a plain getter (done for the single-effect path — a private intermediate target and explicit input binding, runtime-confirmed via `sceneview_world_shader_test`'s output-shader test (grayscale pass, one view affected, one left as a color reference). Multi-effect ordering/ping-pong chaining remains deferred, unneeded until a demo actually requires stacking more than one output shader)
- [x] Matrix-based SceneView camera setter in addition to the existing position/target setter (done — `dxSetSceneViewMatrix` and `DxSceneView:setMatrix` validate finite, non-degenerate basis vectors, normalize the camera basis and reuse the existing internal `CMatrix` path; also accepts the same 4x4 table format as `setElementMatrix` (verified to already share `ReadMatrix`, no code change needed there). Runtime-confirmed via `sceneview_world_shader_test`'s matrix-camera test: a hand-built orbiting table matrix on one view, `getElementMatrix(localPlayer)` reused directly on the other)
- [x] Orthographic SceneView projection (done — `dxSetSceneViewOrthographicProjection(sceneView, width, height, nearClip, farClip)` / `dxSetSceneViewPerspectiveProjection(sceneView)`, validated (finite, positive width/height/near/far, far > near) in the Lua binding, stored on `CClientSceneView` and consumed by a new `CMultiplayer::SetSceneViewProjection(...)` call issued immediately before each `RenderSecondaryScene()`, mirroring the existing `SetSceneViewShaderContext` scoped-per-render-context pattern. Since GTA's own camera model has no orthographic concept, `CMultiplayerSA::RenderSecondaryScene` overrides the RenderWare camera's projection directly via the real `RwCameraSetProjection`/`RwCameraSetViewWindow`/`RwCameraSetNearClipPlane`/`RwCameraSetFarClipPlane` entry points, applied unconditionally (perspective included) after GTA's own `CopyCameraMatrixToRWCam`/`CalculateDerivedValues` position the view matrix - required because the up-to-`MAX_SCENE_VIEWS_PER_FRAME` views share one physical RenderWare camera object sequentially each frame. `dxGetSceneViewInfo` reports the current mode via a new `orthographic` field. Known, documented limitation: GTA's own coarse `ConstructRenderList` visibility selection still culls against the perspective frustum (no orthographic awareness in GTA's own camera model), so an orthographic view's candidate entity list can be a superset of a tightly-fit one - RenderWare's own per-object frustum test during `RenderScene` does use the corrected projection, so this affects draw-call count, not correctness of what is drawn. Runtime-confirmed via `sceneview_world_shader_test`'s orthographic-projection test: View 1 perspective top-down for reference, View 2 orthographic from an elevated 3/4 angle (a straight top-down angle made the no-foreshortening effect too subtle to judge - an angled view where receding geometry visibly does not shrink is the actual confirming test). Debug Win32 builds pass for Game SA, Client Core, Multiplayer SA and Client Deathmatch. **Stage 2 is now closed.**)

## Stage 3 — Cubemap render targets

- [ ] `CCubemapRenderTargetItem` (pending)
- [ ] `dxCreateCubemapRenderTarget` / camera / face-render Lua API (pending)

## Stage 4 — Shadow-mapping primitives

- [ ] 4.1 Shadow-map camera (depth-only, orthographic) (pending)
- [ ] 4.2 Sun-direction shadow example (pending)
- [ ] 4.3 Cascaded shadow prototype (optional) (pending)

## Stage 5 — Debug/demo Lua resources

- [ ] Capabilities viewer (pending)
- [ ] Shader-diagnostics viewer (pending)
- [ ] Render-stats HUD (pending)
- [ ] MRT demo (pending)
- [ ] Custom-depth demo (pending)
- [ ] Render-pass demo (pending)
- [x] Single/multi scene-view demo (done — local-only `sceneview_world_shader_test` resource, same untracked-under-`Bin/` convention as `dx9_foundation_test`. A CEGUI gridlist picker (`client/ui.lua`) switches between five independent tests, each its own file under `client/tests/`: camera parity baseline, two views with genuinely distinct camera angles, per-element world-texture shader targeting a single entity, an output post-process shader, and `dxSetSceneViewMatrix`'s table-matrix input. In-game confirmed working)
- [ ] Cubemap reflection demo (pending)
- [ ] Sky-shader demo (pending)
- [ ] Sun-shadow demo (pending)
- [ ] Cascaded-shadow demo (if built) (pending)

## Stage 6 — Regression, performance, documentation

- [ ] Regression suite (existing `dx*` resources unmodified, `dxGetStatus` shape diff) (pending)
- [ ] Stress tests (views × MRT × nesting × churn, device-loss/resource-stop injection) (pending)
- [ ] Zero-cost-when-idle evidence (pending)
- [ ] Full API documentation + DX9-limitations appendix (pending)
- [ ] Extended-language regression suite: legacy bytecode/technique behavior, SM2 constant-loop unrolling, SM3 dynamic loops, profile fallback and compiler-budget rejection (pending)
