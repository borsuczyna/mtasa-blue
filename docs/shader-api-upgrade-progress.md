# Shader API Upgrade — Progress Tracker

Status legend: `pending` | `in-progress` | `done` | `blocked`

This is a living current-state snapshot, updated in place as work lands. It has no historical log (see commit history for that). Companion document: [shader-api-upgrade-plan.md](shader-api-upgrade-plan.md).

## Architecture prerequisites

- [x] CRenderStateScope (done — `Client/core/Graphics/CRenderStateScope.h/.cpp`)
- [x] Device-loss/resource-stop integration (done — new item types plug into the existing `CRenderItemManager::OnLostDevice`/`OnResetDevice` fan-out and `CClientRenderElement`/`SetParent` ownership; no parallel mechanism added)
- [ ] Recursive-render guard (pending — not needed yet, nothing manually re-enters `Render3DStuff` until item 13)

## Stage 1 — Capability reporting, diagnostics, accounting, MRT, custom depth, render passes, one scene view

- [x] 5. Capability reporting (`SDxCapabilities`, `dxGetRenderCapabilities`) (done)
- [x] 6. Shader compile diagnostics (done — `dxGetShaderDiagnostics`, an optional third `dxCreateShader` result, technique validation, parameter reflection, profiles, source identifier and retained compile log)
- [ ] 6A. Extended shader language/compiler path (pending — opt-in final options table planned; legacy D3DX Effect compilation remains the default. Research must distinguish parser limitations, SM2 static-flow limits and SM3 dynamic-loop support before implementation)
- [x] 7. Render statistics / resource accounting (done for durable counters and GPU-object/memory snapshots via `dxGetRenderStatistics`; asynchronous GPU timings and per-frame switch counters remain a later profiling milestone)
- [ ] 8. Automatic shader value bindings (pending)
- [x] 9. Typed render targets (format validation) (done — `CheckDeviceFormat` guard in `CRenderItemManager::CreateRenderTarget`)
- [x] 10. Custom depth-stencil targets (done — `CDepthStencilTargetItem`/`CClientDepthStencilTarget`/`dxCreateDepthStencilTarget`; **sampleable path explicitly not implemented**, rejected with a clear error)
- [x] 11. MRT binding + validation (done — `CMrtSetItem`/`CClientMrtSet`/`dxCreateMrtSet`)
- [x] 12. Safe scoped render-pass infrastructure (done — `dxBeginRenderPass`/`dxEndRenderPass`, backed by `CRenderStateScope`; defensively force-closed in `CDirect3DEvents9::OnPresent` and `CRenderItemManager::OnLostDevice`, matching the existing `RestoreDefaultRenderTarget()` "in case script forgets" pattern. Known limitation: the open-pass stack is global, not per-resource, so one resource could in principle call `dxEndRenderPass` while another resource's pass is open — same trust model as the existing single-slot `dxSetRenderTarget`, not a new class of risk, but not yet hardened further)
- [ ] 13. One independent scene view (pending — **deliberately not attempted this session**: requires manually invoking `Render3DStuff` outside its hook and converting the `CClouds::RenderSkyPolys`-adjacent hook chain; this is native engine-hooking work that needs to be validated against a running game, which this environment cannot build or launch. Implementing it blind was judged too risky)
- [ ] 14. Camera/state restoration validation (pending — blocked on 13)

### Verification
The Debug Win32 client builds successfully in Visual Studio. The `dx9_foundation_test` resource has also passed an in-game MRT/custom-depth/render-pass smoke test on hardware reporting four simultaneous render targets. The second target remains black with ordinary `dxDraw*` calls by design; a shader writing distinct `COLOR0`/`COLOR1` outputs is still needed for the deterministic MRT-output test.

## Sky shaders

- [ ] Conditional sky-render trampoline (pending)
- [ ] `engineApplyShaderToSky` / `engineRemoveShaderFromSky` (pending)

## Stage 2 — Multiple scheduled scene views

- [ ] Raise scene-view cap + update modes (pending)
- [ ] Per-view GPU-time accounting (`CGpuQueryManager`) (pending)
- [ ] Budget-exceeded rejection (pending)

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
- [ ] Single/multi scene-view demo (pending)
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
