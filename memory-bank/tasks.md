# Tasks

## Source of truth for task tracking
Updated by VAN/plan workflows. Complexity and next steps derived from here and activeContext.

## Current task
- **Task**: Compile-time hash map: constexpr hash generation and comparison (populate from types; lookup by hash).
- **Complexity**: Level 3 (design + implementation; constexpr hash map).
- **Status**: **Planned.** See "Plan: Compile-time hash map (constexpr)" below. (Previous: Directory.Build.targets migration — implemented.)

### Plan: Compile-time hash map (constexpr)

**Goal**  
Support compile-time hash generation and comparison by populating a constexpr “hash map”: a fixed set of (hash, value) pairs built at compile time from a type list or hash list, with constexpr lookup (e.g. “is this HashType in the set?” or “index of this hash”).

**Current state**  
- **Hash generation**: Already compile-time. `type_hash<T>::value` is a constexpr `HashTypeT<T>`; hash is computed via `cityhash::CityHashCrc256_s(static_type_name<T>::full_name()...)` in `HashTypeT` constructor.  
- **Comparison**: `HashType` is pointer-to-`HashTypeImpl`; equality is pointer comparison. `polymorphic_type_hash<T>::is_derived_of(base)` does linear search over `upcast_array` (HashArray from typelist) using `std::ranges::find_if`.  
- **Gap**: No general constexpr map from hash value (e.g. `cityhash256` or `HashType`) to an index or “present” flag for an arbitrary set of types. Adding one allows “is hash in allowed set?” or “type index for hash” at compile time without runtime tables.

**Technology**  
- **C++ standard**: Project uses **C++20** (CommonProject.build.cs, Monolith.build.cs).  
- **Options for constexpr map**:  
  - **A. Constexpr sorted array + binary search**: Store `std::array<std::pair<cityhash256, size_t>, N>` (or `HashType` as key if we keep pointer identity). Sort at compile time; lookup via constexpr binary search. Keys must be comparable: `cityhash256` already has `operator<`.  
  - **B. Typelist linear search (existing pattern)**: For a `type_list<T...>`, “contains hash” is a constexpr loop over `type_hash<T>::value` (or over `HashArray`). No new container; reuse `chain_to_upcast_array`-style array + find. Good for small N.  
  - **C. External library**: e.g. Boost.Hana (`hana::map`) for type-level keys. Adds dependency; may not map directly to `HashType`/`cityhash256` without adapters.

**Recommended approach**  
- **Implement a minimal constexpr hash set/map in Engine (no new library).**  
  1. **Key type**: Use `cityhash::cityhash256` for the key so the map is value-based and can be built from type names (via `type_hash<T>::value.v`).  
  2. **Storage**: `std::array<std::pair<cityhash256, size_t>, N>` (or pair of arrays for keys and values). Populate from a typelist: `type_list<T...>` → array of `(type_hash<T>::value.v, index)`.  
  3. **Sort**: Constexpr sort the array by `cityhash256::operator<` so lookup is binary search.  
  4. **Lookup**: `constexpr bool contains(cityhash256)` and/or `constexpr std::optional<size_t> find_index(cityhash256)` using constexpr binary search.  
  5. **API**: e.g. `constexpr_hash_map<type_list<A,B,C>>` or `constexpr_hash_set<HashArray<N>>` with `contains(HashType h)` that uses `h->v` for the key.

**Implementation steps**  
1. **CoreType.h (or Memory public header)**  
   - Add `constexpr_hash_set` / `constexpr_hash_map`: template on a typelist or on `HashArray<N>`.  
   - Helper: build sorted `std::array<std::pair<cityhash256, size_t>, N>` from `type_list<T...>` (each element `type_hash<T>::value.v`, index).  
   - Constexpr sort (e.g. insertion sort or small constexpr sort compatible with C++20).  
   - `contains(cityhash256 key)` and optionally `find_index(cityhash256 key)` via binary search.  
   - If the API takes `HashType`, use `h->v` (cityhash256) for the key so the same map works for “set of types” and “set of hashes”.  
2. **Use cases**  
   - Allowed-type validation: e.g. “only these types may be used in this API”; build map from `type_list<A,B,C>`, then `if (!map.contains(ptr->GetTypeHash()->v)) return error;`.  
   - Compile-time dispatch: map hash to index into a switch or table.  
3. **Testing**  
   - Unit test or small example: build map from `type_list<int, float, MyClass>`, call `contains(type_hash<int>::value.v)` (true), `contains(some_other_hash)` (false).  
4. **No library add**  
   - Implement sort and binary search as constexpr functions in the same header; no dependency on Boost.Hana or other constexpr map libraries.

**Creative / design decisions**  
- **Key: cityhash256 vs HashType**: Using `cityhash256` as key allows value semantics and building the map from any list of hashes (not only types). Using `HashType` (pointer) would limit to types that have `type_hash<T>::value` and would not allow “set of hashes” from e.g. string names. Recommendation: key = `cityhash256`; provide overloads that take `HashType` and use `h->v`.  
- **Value type**: For “set” use cases, value can be `size_t` index or nothing (bool). For “map” use cases, value can be a second template parameter (e.g. index or type tag). Start with set (contains + optional index); extend to map if needed.

**References**  
- Hash generation: `Engine/Source/Runtime/Memory/Public/CoreType.h` — `type_hash`, `HashTypeT`, `cityhash::CityHashCrc256_s`, `static_type_name`, `chain_to_upcast_array`, `HashArray`.  
- Build: `Build/CommonProject.build.cs` (CppLanguageStandard.CPP20).

### Plan: Graceful module cleanup (DLL order, dependency, type resolution)

**Goal**  
Make modules clean up gracefully: fix incorrect shutdown/unload order, ensure subsystems clear module-scoped state before DLL unload, and avoid crashes during memory cleanup when types or code from an unloaded DLL are still referenced.

**Problems (existing)**  
1. **Shutdown order**: ~~`ModuleManager::Shutdown()` iterates in load order~~ **Fixed**: `ModuleManager::Shutdown()` now iterates `m_module_load_order_` in **reverse** order (dependents before dependencies).  
2. **DLL unload order**: `Destroy()` correctly uses **reverse** load order for `FreeLibrary`. Shutdown order is still wrong, so a module’s `ShutdownImpl()` may run after a dependency was already shut down and may touch that dependency.  
3. **No subsystem cleanup before Shutdown**: `RegisterOnModuleShutdown` exists and is documented in `ModuleRegistration.h`, but **no subsystem currently registers** a callback. Any subsystem that stores state by module name or holds `HashType`/vtables/pointers into a module never gets a chance to clear that state before `IModule::Shutdown()` and before `FreeLibrary`. After unload, those pointers are dangling; use (or static destructors) → crash.  
4. **Memory cleanup vs. unload order (non‑monolith)**: In `Launch.cpp`, after `ModuleManager::Shutdown()`, the code runs `g_static_alloc` release/purge (rebind_releases/rebind_purge), then `ModuleManager::Destroy()` (FreeLibrary). The rebind callbacks may invoke code in module DLLs; that is safe only if DLLs are still loaded. After `Destroy()`, no code should hold pointers into unloaded DLLs (e.g. `type_hash<T>` in Memory.dll). Static destructors in the exe or in remaining DLLs must not touch type info from already-unloaded DLLs.

**Mitigations**

| # | Mitigation | Description |
|---|------------|-------------|
| 1 | **Reverse shutdown order** | In `ModuleManager::Shutdown()`, iterate `m_module_load_order_` in **reverse** order (reverse iteration or build a reverse-order list) so that for each module, all dependents are shut down before it. This matches dependency semantics: last loaded (top of dependency DAG) shuts down first. |
| 2 | **Adopt OnModuleShutdown** | Every subsystem that holds module-scoped state (registrations by module name, caches keyed by `HashType` or types from modules, resolvers that store pointers into module code) must call `ModuleManager::GetInstance().RegisterOnModuleShutdown(...)` and in the callback implement `UnregisterModule(std::wstring_view name)`: remove all registrations for that module and clear any cached pointers/handles for that module. Document which subsystems must register (e.g. Renderer, ResourceManager, any factory or type-resolver). |
| 3 | **Clear type/allocator state before FreeLibrary** | Ensure `g_allocator_storage` and `g_static_alloc` (and any other global state that holds `type_hash<T>` or function pointers into modules) are fully released and cleared **before** calling `FreeLibrary`. In non‑monolith `Launch.cpp`, the current order (alloc cleanup → `Destroy()`) is correct; ensure no late use of allocators keyed by types from unloaded DLLs. Optionally: before `Destroy()`, explicitly clear or null out any global caches that might hold HashType/type_hash from modules. |
| 4 | **Single, consistent teardown sequence** | Define and document a single teardown sequence for both monolith and non‑monolith: (1) EngineEntryPoint::Destroy(), (2) ModuleManager::Shutdown() (in reverse load order), (3) all global allocator/memory cleanup (g_static_alloc, g_allocator_storage, etc.), (4) ModuleManager::Destroy() (FreeLibrary in reverse order), (5) core/graphic/OS cleanup if applicable. Ensure Launch.cpp (non‑monolith) and MonolithicLaunch follow the same logical order. |
| 5 | **No type resolution after unload** | After `ModuleManager::Destroy()`, no code path (including static destructors) should dereference `HashType` or `type_hash<T>` that lived in an unloaded DLL. If the Memory module is unloaded last (as in Launch.cpp), type_hash from other DLLs are already invalid when Memory’s destructors run; prefer unloading Memory only after all other modules are destroyed and no remaining code holds type_hash pointers from other DLLs. |

**Implementation steps**  
1. **ModuleManager::Shutdown() — reverse order**  
   In `ModuleManager.cpp`, change the shutdown loop to iterate `m_module_load_order_` in reverse (e.g. `for (auto it = m_module_load_order_.rbegin(); it != m_module_load_order_.rend(); ++it) ShutdownModule(*it);`). Keep `Destroy()` as-is (already reverse).  
2. **Audit subsystems for module-scoped state**  
   Identify every subsystem that (a) registers something per-module (e.g. render passes, resources, factories), or (b) caches pointers/HashType/type_hash that could point into a module. For each, add `RegisterOnModuleShutdown` and implement `UnregisterModule(module_name)` that removes all entries for that module and clears related caches.  
3. **Document and enforce teardown order**  
   In `ModuleRegistration.h` or a short “Module lifecycle” note: state the exact order (Shutdown in reverse load order → alloc/memory cleanup → Destroy/FreeLibrary) and that subsystems must not hold references into a module after its callback has run.  
4. **Launch.cpp (non‑monolith)**  
   Align cleanup sequence with the defined order: ensure ModuleManager::Shutdown() runs before any cleanup that might call into modules; ensure ModuleManager::Destroy() runs only after allocator/memory cleanup; ensure core/graphic/OS DLLs are unloaded in an order that avoids type resolution from unloaded DLLs (e.g. unload Memory last).  
5. **Optional: dependency-aware shutdown**  
   If load order does not strictly reflect dependency DAG (e.g. lazy loading), consider computing a proper shutdown order from `GetDependencies()`/`LoadAfter()` so that dependents are always shut down before their dependencies, instead of relying only on load order.  
6. **Core module init/deinit: single source of order**  
   Implement Option A (CoreModuleBootstrap) or Option B (documented table). Prefer Option A: add `CoreModuleBootstrap::InitializeCoreModules()` and `DeinitializeCoreModules()` that perform the exact init and deinit sequences above; call them from `Launch.cpp` so the order is not duplicated and cannot drift. Document the dependency DAG and the required order in code comments or in `ModuleRegistration.h` / a short module-lifecycle doc.

**Graceful module cleanup — implementation status**  
- **Done**: (1) ModuleManager::Shutdown() uses reverse load order. (2) Renderer and ResourceManager register OnModuleShutdown and implement UnregisterModule; render/resource modules pass ENGINE_MODULE_SCOPE. (3) Teardown sequence documented in ModuleRegistration.h (steps 1–9). (4) Launch.cpp deinit order aligned: Destroy → Shutdown → Core → Graphic → OS → alloc → Destroy → CoreModule → Memory.  
- **Optional (not done)**: Dependency-aware shutdown from DAG; CoreModuleBootstrap to centralize core init/deinit.

**Core modules: manual load/unload, order, and dependency**

The following applies to the **non‑monolith** path. Core modules (Memory, CoreModule, Core, WinAPIWrapper, GraphicInterface) are **manually** loaded and unloaded in `Launch.cpp` via `load_seq` / `cleanup_seq` and globals (`g_core_mem`, `g_module_api`, `g_core_api`, `g_os_api`, `g_graphic_api`). They are **not** managed by `ModuleManager` (they are removed from `m_module_loaded_` in `Shutdown()` via `CheckNoInit`). Their init/deinit order and dependencies must be defined explicitly and implemented in one place.

**Dependency DAG (core only)**  
- **Memory**: no engine dependency (provides allocator, type_hash, CoreType). Must be loaded first so type_hash and allocators exist before any other module uses them.  
- **CoreModule**: depends on Memory (ModuleManager, ModuleInfo, IModule). Must be loaded after Memory.  
- **Core**: depends on Memory (CoreType, allocators), CoreModule (optional: may use ModuleManager). Load after CoreModule.  
- **WinAPIWrapper**: OS layer; no engine module dependency. Can load after Core so that Core can assume OS is ready if needed.  
- **GraphicInterface**: depends on Memory, Core; may depend on WinAPI (window/device). Load after Core and WinAPIWrapper.

**Required initialization order (concrete)**  
1. **Memory** — `load_seq(g_core_mem, "Memory.dll", ...)`  
2. **CoreModule** — `load_seq(g_module_api, "CoreModule.dll", ...)`  
3. **Core** — `load_seq(g_core_api, "Core.dll", ...)`  
4. **WinAPIWrapper** — `load_seq(g_os_api, "WinAPIWrapper.dll", ...)` then `WinAPI::WinAPIWrapper::Initialize(hInstance)`  
5. **GraphicInterface** — `load_seq(g_graphic_api, "<GraphicInterface>.dll", ...)`  
6. **EngineEntryPoint::Initialize()** (starts app loop / loads modules via ModuleManager)  
7. **WinAPIWrapper::Update()** (or equivalent first frame)

**Required deinitialization order (concrete)**  
Deinit must be the **reverse** of dependency order so that no code runs in a module after its dependencies are unloaded. Order:  
1. **EngineEntryPoint::Destroy()** — stop app loop, release high-level systems.  
2. **ModuleManager::Shutdown()** — shut down all **managed** modules (reverse load order); core modules are skipped (CheckNoInit).  
3. **Core** — `cleanup_seq(g_core_api)` (Core’s ShutdownImpl; no use of Graphic/OS beyond what’s already torn down).  
4. **GraphicInterface** — `cleanup_seq(g_graphic_api)`.  
5. **WinAPIWrapper** — `cleanup_seq(g_os_api)`.  
6. **Global allocator / memory cleanup** — `g_static_alloc` release/purge, `g_allocator_storage.cleanup()`, `PoolAllocatorStorage::report_leakage()`. No use of type_hash or allocators from modules that are already unloaded.  
7. **ModuleManager::Destroy()** — FreeLibrary for all **managed** DLLs (reverse order).  
8. **CoreModule** — `cleanup_seq(g_module_api)` (ModuleManager itself is in CoreModule; destroy it after managed modules are unloaded).  
9. **Memory** — `cleanup_seq(g_core_mem)` last, so no code still holds `type_hash<T>` or allocator state from other DLLs when Memory is unloaded.

**Management and single source of truth**  
- **Option A (recommended): centralize in a bootstrap type**  
  Introduce a small `CoreModuleBootstrap` (or equivalent) in the engine (e.g. under Launch or CoreModule) that: (1) holds the ordered list of core module names and paths, (2) exposes `InitializeCoreModules()` and `DeinitializeCoreModules()` that perform load_seq/cleanup_seq in the documented order, (3) is the only place that knows the dependency order. `Launch.cpp` (and any other non‑monolith entry point) calls these two functions instead of inlining load_seq/cleanup_seq. Same order is used for monolith if/when core is ever split into DLLs.  
- **Option B: document only**  
  Keep init/deinit in `Launch.cpp` but add a single comment or small table (e.g. in `ModuleRegistration.h` or a “Module lifecycle” doc) that states the exact init and deinit order and the dependency DAG. Any new entry point or platform must follow the same table.  
- **ModuleManager’s role**  
  ModuleManager **does not** load or unload core modules; it only skips them in `Shutdown()`/`Destroy()` via `CheckNoInit`. Managed modules (everything else) are loaded via `LoadModule` / `LoadModuleAll` and unloaded via `Shutdown()` (reverse order) then `Destroy()`. Core modules remain the responsibility of the entry point (or CoreModuleBootstrap).

**Current vs required (Launch.cpp)**  
- **Init**: Memory → CoreModule → Core → WinAPIWrapper → GraphicInterface → EntryPoint → Update. Matches required order.  
- **Deinit**: **Aligned.** Order is EntryPoint::Destroy() → ModuleManager::Shutdown() → Core → Graphic → OS → alloc cleanup → ModuleManager::Destroy() → CoreModule → Memory. Documented in `ModuleRegistration.h` (teardown sequence). Optional: centralize in CoreModuleBootstrap later.

**References**  
- `Engine/Source/Runtime/CoreModule/Public/ModuleManager.h` — Shutdown, Destroy, RegisterOnModuleShutdown, m_module_load_order_.  
- `Engine/Source/Runtime/CoreModule/Private/ModuleManager.cpp` — Shutdown (reverse iteration), ShutdownModule (callbacks then IModule::Shutdown), Destroy (reverse, FreeLibrary), CheckNoInit (core modules excluded).  
- `Engine/Source/Runtime/CoreModule/Public/ModuleRegistration.h` — pattern for UnregisterModule and RegisterOnModuleShutdown.  
- `Engine/Launch/Private/Launch.cpp` — non‑monolith: load_seq (Memory, CoreModule, Core, WinAPIWrapper, GraphicInterface); cleanup_seq order per ModuleRegistration.h: Destroy → Shutdown → Core → Graphic → OS → alloc → Destroy → CoreModule → Memory.  
- `Engine/Source/Runtime/Memory/Public/Allocator.h` — g_static_alloc, rebind_release.  
- `Engine/Source/Runtime/Memory/Public/CoreType.h` — g_allocator_storage, type_hash, is_allocator_live.

### Plan: Load timing enum (load phases)

**Goal**  
Introduce explicit **load timing phases** (e.g. Core, UI, Graphic, Internal) so module load order is driven by phase first, then by existing dependency/LoadAfter within each phase. This replaces the need for duplicated manual dependency lists when the real intent is “load me after Core” or “load me in the Graphic phase”.

**Current pain**  
- Many modules override `LoadAfter()` (and sometimes `GetDependencies()`) with long, duplicated lists of module names (e.g. “Core”, “Memory”, “RenderPipeline”, …) just to express “I load after foundational / graphic / UI” subsystems.  
- Order is emergent from registration order and lazy resolution; there is no single place that expresses “Core time”, “UI time”, “Graphic time”, “Internal (scene manager, input, etc.)”.

**Proposed design**

1. **Load phase enum**  
   - Add an enum (e.g. `ELoadPhase` or `LoadPhase`) in a CoreModule (or Core) public header, used by both static and DLL builds.  
   - Suggested values (expand as needed):  
     - **Core** — Memory, CoreModule, Core (and optionally WinAPIWrapper if kept in manager).  
     - **Graphic** — D3D12GraphicInterface, RenderPipeline, Shader, Texture, Mesh, etc.  
     - **UI** — ImGuiManager, any UI subsystem.  
     - **Internal** — SceneManager, InputManager, CameraManager, SoundManager, PhysicsManager, and other “internal” engine systems.  
     - **Application** (optional) — game/editor modules that depend on everything above.  
   - Naming can follow existing style (e.g. `EENUM()` / `ENGINE_COREMODULE_API` if the enum lives in CoreModule).

2. **IModule API**  
   - Add `virtual ELoadPhase GetLoadPhase() const` to `IModule`, with a default implementation returning e.g. `Internal` (or the phase that matches current “no LoadAfter” behavior).  
   - Modules that today only use `LoadAfter()` to mean “after Core” or “after Graphic” can instead (or additionally) return the appropriate phase; phase then drives coarse order without listing every module by name.

3. **ModuleManager load order**  
   - **Option A (two-phase load)**  
     - **Create**: For each module, load DLL / get `IModule*` (create only; do not call `Initialize()`).  
     - **Sort**: Sort modules by `GetLoadPhase()` (enum order), then within same phase by dependency (resolve `GetDependencies()` / `LoadAfter()` and topological order).  
     - **Initialize**: Call `Initialize()` in that sorted order and append to `m_module_load_order_`.  
     - Requires separating “create IModule” from “initialize IModule” in `LoadModule` (or adding a create-only path used by `LoadModuleAll`).  
   - **Option B (metadata table)**  
     - Maintain a table (name → phase, optional name → dependencies) so the manager can sort **before** loading any module. Then call existing `LoadModule(name)` in the sorted order.  
     - Table can be hand-maintained or generated (e.g. from a macro or header parser). Modules still implement `GetLoadPhase()` for consistency; the table is used only for the initial sort in `LoadModuleAll`.  
   - Recommendation: **Option A** if we are willing to refactor `LoadModule`/`LoadModuleAll` to support create-then-initialize; **Option B** if we want minimal change to current load path and are okay maintaining a table.

4. **Shutdown order**  
   - Shutdown and Destroy already use reverse load order. Once load order is phase-aware, reverse order remains “dependents before dependencies” as long as within-phase order is dependency-correct.

5. **Migration**  
   - Implement enum and `GetLoadPhase()`; default to `Internal`.  
   - Assign phases to core/graphic/UI modules (e.g. Core, Memory → Core; D3D12, RenderPipeline, Shader → Graphic; ImGui → UI; SceneManager, InputManager, … → Internal).  
   - Optionally trim `LoadAfter()` lists where they only repeated “everything in earlier phases”; keep `LoadAfter()` for fine-grained order within the same phase (e.g. Shader after Texture).

**Implementation steps**  
1. **Add enum and IModule::GetLoadPhase()**  
   - Define `ELoadPhase` in `IModule.h` (or `ModuleManager.h` / dedicated `LoadPhase.h`).  
   - Add `virtual ELoadPhase GetLoadPhase() const` with default `return ELoadPhase::Internal;`.  
2. **Implement phase-aware sort in ModuleManager**  
   - Option A: Add a path that creates all modules without initializing; sort by phase then dependency; then initialize in order and build `m_module_load_order_`.  
   - Option B: Add a name→phase (and optionally name→dependencies) table; in `LoadModuleAll`, sort the list of module names by phase then dependency, then call `LoadModule(name)` in that order.  
3. **Assign phases per module**  
   - Override `GetLoadPhase()` in each module (or populate the table in Option B) for Core, Graphic, UI, Internal, Application.  
4. **Document**  
   - In `ModuleRegistration.h` or a short “Module lifecycle” note: document the phase order and that within-phase order is still given by `GetDependencies()` / `LoadAfter()`.

**References**  
- `Engine/Source/Runtime/CoreModule/Public/IModule.h` — GetDependencies(), LoadAfter().  
- `Engine/Source/Runtime/CoreModule/Private/ModuleManager.cpp` — LoadModule (static/DLL), LoadModuleAll, m_module_load_order_, TryResolveLazyness.  
- Various `*Module.cpp` — LoadAfter() overrides (RenderPipeline, D3D12GraphicInterface, Shader, Texture, ImGuiManager, CameraManager, etc.).

**Implementation status (explicit load phase)**  
- **Done**: (1) `ELoadPhase` enum and `IModule::GetLoadPhase()` in `IModule.h`. (2) Option A in ModuleManager: `CreateModuleOnly(name)`, `InitializeModuleInOrder(name)`, `SortByPhaseAndDependency(names)`; `LoadModuleAll()` now creates all modules (static: from `m_module_initializer_`; DLL: directory scan + AddModule), sorts by phase then topological order (GetDependencies() only), then initializes in that order. Core/special modules remain excluded via `CheckNoInit`. (3) **GetLoadPhase() overrides** added to all runtime modules: CoreModule → Core; IClientModule → Application; RenderPipeline, Shader, Texture, Mesh, ComputeShader, RaytracingExtension, RaytracingShader, Material, Deferred/Forward/RaytracingRenderPassTask, ShadowTexture, ShadowManager, Font, RenderComponent, ModelRenderer, TextRenderer, Shape, ShapeRenderComponent, ParticleRenderer, ParticleRendererExtension, ParticleRendererRenderInstanceTask, AnimationTexture, AtlasAnimationTexture, Bone, BaseAnimation, BoneAnimation, AtlasAnimation → Graphic; ImGuiManager → UI; SoundManager, Sound, SoundPlayer, FMODSoundInterface, InputManager, DirectInputInterface, CameraManager, PhysicsManager, BoostSocketWrapper, ReflectionEvaluator, Animator, ProjectionFrustum → Internal. (4) Phase order documented in `ModuleRegistration.h`. (5) Core load order: `kCoreModuleLoadOrder` in ModuleRegistration.h; Launch.cpp follows it. (6) LoadAfter() removed from IModule and all modules; within-phase order uses GetDependencies() only.  
- **Pending**: None for load phases.

**Loading phase order for every module**

Phase enum order: **Core (0) → Graphic (1) → UI (2) → Internal (3) → Application (4)**. Within each phase, order is determined by topological sort of `GetDependencies()`.

| Phase | Module name | Notes |
|-------|-------------|--------|
| **Core (manual)** | Memory | Not in ModuleManager; loaded first in Launch. |
| | CoreModule | Not in ModuleManager; after Memory. |
| | Core | Not in ModuleManager; after CoreModule. |
| | WinAPIWrapper | Not in ModuleManager; after Core. |
| | GraphicInterface (e.g. D3D12GraphicInterface) | Not in ModuleManager; after Core/WinAPI. |
| **Graphic (1)** | RenderPipeline | No LoadAfter; base of graphic stack. |
| | RaytracingExtension | LoadAfter: RenderPipeline. |
| | Shader, Texture, Mesh, ComputeShader, DeferredRenderPassTask, ForwardRenderPassTask, RaytracingRenderPassTask | LoadAfter: RenderPipeline (and RaytracingExtension for RaytracingShader). |
| | RaytracingShader | LoadAfter: RenderPipeline, RaytracingExtension. |
| | Material, ShadowTexture, ShadowManager, Font | Graphic resources/passes. |
| | RenderComponent, ModelRenderer, TextRenderer, Shape, ShapeRenderComponent | Render components. |
| | ParticleRenderer, ParticleRendererExtension, ParticleRendererRenderInstanceTask | Particle rendering. |
| | AnimationTexture, AtlasAnimationTexture, Bone, BaseAnimation, BoneAnimation, AtlasAnimation | Animation/rendering assets. |
| **UI (2)** | ImGuiManager | LoadAfter: RenderPipeline. |
| **Internal (3)** | FMODSoundInterface | Sound backend; no LoadAfter. |
| | SoundManager | LoadAfter: FMODSoundInterface. |
| | Sound, SoundPlayer | LoadAfter: SoundManager (and FMODSoundInterface). |
| | InputManager, DirectInputInterface | Input. |
| | CameraManager | LoadAfter: RenderPipeline. |
| | PhysicsManager | Physics. |
| | BoostSocketWrapper | Networking. |
| | ReflectionEvaluator | LoadAfter: RenderPipeline. |
| | Animator | Gameplay animator. |
| | ProjectionFrustum | Camera/math. |
| **Application (4)** | (Client/game/editor modules) | Override `GetLoadPhase()` to return `Application` when added. |

**Source of names**: `MODULE_IMPL(ModuleClass, Name)` in each `*Module.cpp`; core names from `CheckNoInit` and Launch load sequence.  
**Within-phase order**: Resolved by `SortByPhaseAndDependency()` from `GetDependencies()` only (e.g. FMODSoundInterface before SoundManager before Sound if GetDependencies() returns them).

### Plan: Detach managers from Core and late-phase UI registration

**Goal**  
Detach more managers from the core so that CoreModule does not own their lifecycle; and ensure UI-related registration (ResourceManager::RegisterLoadResource / RegisterNewResource, SceneManager menu registration) happens in a late phase, after or with UIManager (ImGuiManager) so that UI is ready when registrations are used.

**Current state**  
- **CoreModule** (Core phase) calls `CoreLoop::AddManager(LOOP_TYPE_LOGIC, ResourceManager::GetInstance, SceneManager::GetInstance, TaskScheduler::GetInstance)` and optionally `AddManager(LOOP_TYPE_RENDER, Debugger::GetInstance)`. So ResourceManager, SceneManager, and TaskScheduler are created and started as soon as Core loads.  
- **ImGuiManager** loads in **UI** phase (after Graphic, before Internal). It registers with CoreLoop and provides the actual UI (ImGui).  
- **UI registration** (RegisterLoadResource, RegisterNewResource, RegisterNewMenuItem, RegisterLoadMenuItem) is done in **Graphic**-phase modules (Texture, Shader, Material, Shape, Font, ComputeShader, AtlasAnimationTexture, Sound) inside their `InitializeImpl()`. So registration runs before ImGuiManager is in the loop; ResourceManager just stores callbacks. At runtime, when the user opens a dialog, ResourceManager uses `g_ui_accessor` (set by ImGuiManager). So functionally registration can stay where it is, but the user wants “UI registration in late phase” for clarity and so that UI is initialized before any registration runs.

**Proposed direction**

1. **Detach manager registration from Core**  
   - Remove from **CoreModule::InitializeImpl()** the `AddManager` calls for ResourceManager, SceneManager, and TaskScheduler (and optionally Debugger).  
   - Introduce a **later-phase module** that owns adding these managers to CoreLoop. Options:  
     - **A. New module** (e.g. `EngineLoop` or `CoreServices`): loads in **Graphic** or **Internal** phase, depends on Core; in `InitializeImpl()` calls `CoreLoop::AddManager(LOOP_TYPE_LOGIC, ResourceManager::GetInstance, SceneManager::GetInstance, TaskScheduler::GetInstance)` and, if WITH_DEBUG, `AddManager(LOOP_TYPE_RENDER, Debugger::GetInstance)`. In `ShutdownImpl()` calls the matching `RemoveManager`.  
     - **B. Reuse an existing early module**: e.g. RenderPipeline (Graphic) or a first Internal module could add these managers. That couples “loop bootstrap” to that feature; a dedicated small module is cleaner.  
   - **Recommendation**: Option A — new module (e.g. `EngineLoop`), phase **Graphic** (so ResourceManager/SceneManager exist before any Graphic module that calls RegisterLoadResource in its InitializeImpl).  
   - **CoreModule** after change: either no-op (or only minimal core wiring) or still add something that must run at Core time; document that “loop managers” are owned by EngineLoop.

2. **UI registration in late phase**  
   - **Option A (phase order only)**  
     - Keep current pattern: modules call `ResourceManager::RegisterLoadResource` / `RegisterNewResource` (and SceneManager menu registration) in their own `InitializeImpl()`.  
     - Ensure **ImGuiManager** loads **before** any module that performs UI registration: move ImGuiManager to load **before** Graphic modules that register (e.g. UI phase before Graphic), or keep phase order Graphic → UI and **move** all UI-registering modules to **Internal** (or a new “PostUI” phase) so they load after ImGuiManager.  
     - Simplest: change **phase order** so **UI (2)** comes **before** **Internal (3)** but **after** **Graphic (1)** is insufficient if Graphic modules register; so either (i) **UI before Graphic** (big change: ImGuiManager before RenderPipeline), or (ii) **UI registration in Internal/Application**: have only modules in Internal (or later) call Register*; Graphic modules that currently register would defer registration (see Option B).  
   - **Option B (deferred registration)**  
     - Provide a “deferred UI registration” API: e.g. `RegisterLoadResourceDeferred(type_name, callback)` that stores the intent; a **flush** runs once ImGuiManager has been initialized (e.g. from EngineEntryPoint after first frame or from a late-phase module). Then Graphic modules can call the deferred API in InitializeImpl; flush happens in late phase.  
   - **Option C (registration pass after UI)**  
     - Keep phase order Graphic → UI → Internal. Add an explicit “UI registration pass” that runs **after** ImGuiManager::Initialize: e.g. EngineEntryPoint or a small subsystem calls “register all” on ResourceManager/SceneManager, and each module (or a registry) provides its Register* in that pass. Modules would not call Register* in InitializeImpl; they would register their intent with a central list, and the pass would run after UI is ready.  
   - **Recommendation**: Option A with (ii) — **move UI registration to late phase** by having modules that currently register in Graphic phase instead **declare** their registration (e.g. via a macro or a small per-module function) and have a **single late-phase module** (e.g. “UIRegistration” or “EditorUI” in **Internal** or **Application**, loading after ImGuiManager) that calls ResourceManager::RegisterLoadResource etc. for all known types. That requires centralizing the list of “who registers what” (e.g. a table or generated code). Alternatively, minimal change: **keep registration in each module’s InitializeImpl** but **ensure those modules load in Internal phase** (after UI): move Texture, Shader, Material, etc. to Internal, or introduce a sub-phase “InternalAfterUI” so they load after ImGuiManager. Easiest: **phase order UI then Internal** and **move only the “registration” side** so that any module that does Register* loads in **Internal** (or after ImGuiManager). So: modules that only need to “register UI” and don’t need to be Graphic for other reasons could be Internal; or add a **new phase** “PostUI” = 4, Application = 5, and put “UI-registering” modules in PostUI so they load after ImGuiManager.

3. **Concrete implementation steps (detach managers)**  
   - Add new module **EngineLoop** (or **CoreServices**): depends on Core, phase **Graphic** (or Internal).  
   - In EngineLoopModule::InitializeImpl: `CoreLoop::AddManager(LOOP_TYPE_LOGIC, ResourceManager::GetInstance, SceneManager::GetInstance, TaskScheduler::GetInstance);` and, if WITH_DEBUG, `AddManager(LOOP_TYPE_RENDER, Debugger::GetInstance);`  
   - In EngineLoopModule::ShutdownImpl: matching `RemoveManager` in reverse order.  
   - In CoreModule::InitializeImpl / ShutdownImpl: remove the AddManager / RemoveManager calls for ResourceManager, SceneManager, TaskScheduler (and Debugger if moved).  
   - Update **Loading phase order** table: add EngineLoop in Graphic (early) or Internal; document that Core no longer adds these managers.

4. **Concrete implementation steps (UI registration late phase)**  
   - **Minimal**: Document that “UI registration should occur from modules that load after ImGuiManager”; optionally move one or two modules that only do registration to Internal so they load after UI.  
   - **Full**: Introduce a “UI registration pass” or “PostUI” phase: either (a) a new phase value (e.g. PostUI = 4, Application = 5) and put UI-registering modules there, or (b) a dedicated UIRegistration module (Internal or Application) that runs after ImGuiManager and invokes all Register* for known types (centralized list).  
   - Ensure **g_ui_accessor** (or equivalent) is set before any code path that opens a UI dialog; today ImGuiManager sets this when it initializes, so any registration that might trigger a dialog before first frame should be avoided or deferred.

**Dependencies and risks**  
- CoreModule must not depend on ResourceManager/SceneManager/TaskScheduler for its own init (only for AddManager). Removing AddManager from CoreModule is safe.  
- EngineLoop module must link Core (where ResourceManager, SceneManager, TaskScheduler, CoreLoop live).  
- If UI registration is moved to a later phase, any code that assumes “all Register* are done” by end of Graphic phase must be updated (e.g. editor menus built from ResourceManager).

**References**  
- `Engine/Source/Runtime/Core/Private/CoreModule.cpp` — current AddManager/RemoveManager.  
- `Engine/Source/Runtime/Core/Public/EngineEntryPoint.h` — CoreLoop::AddManager.  
- `Engine/Source/Runtime/ImGuiManager` — UI phase; ImGuiManagerModule adds manager in InitializeImpl.  
- Texture, Shader, Material, Shape, Font, ComputeShader, Sound, AtlasAnimationTexture — RegisterLoadResource/RegisterNewResource in InitializeImpl.

### Plan: ModuleManager — enhanced dependency management, refactorization and validation

**Goal**  
Make ModuleManager’s dependency handling explicit, maintainable, and safe: refactor duplicated logic, introduce a clear dependency model (DAG), add validation (e.g. cycle detection), and derive load/shutdown order from that model instead of emergent behavior.

**Current state and pain points**  
1. **Two parallel concepts**: `GetDependencies()` and `LoadAfter()` are both “must be loaded before me”; the manager treats them identically but IModule exposes two APIs. Modules use one or both (e.g. IClientModule::LoadAfter() returns GetDependencies()). Semantics are unclear and code is duplicated (two loops in LoadModule for static and DLL paths).  
2. **No explicit DAG**: Load order is emergent from the order in which `LoadModule` / `LoadModuleAll` are called and from `TryResolveLazyness` (when a dependency appears, the dependent is retried). There is no single place that “knows” the full dependency graph or a canonical load order.  
3. **No validation**: Circular dependencies are not detected; they can cause infinite recursion or modules stuck in `m_lazy_modules_` forever. Missing dependencies are only discovered at load time by failing to find a module.  
4. **Lazy resolution is hard to reason about**: `m_lazy_modules_[A] = { B, C }` means “A is waiting for B and C”. When B loads, we erase B from A’s set and if the set becomes empty we call LoadModule(A) again. Order of resolution depends on which module gets loaded first; multiple passes may be needed. No guarantee that the final `m_module_load_order_` is a valid topological order if dependencies are declared inconsistently.  
5. **CheckNoInit duplicated and brittle**: Core/special modules are excluded by name (static) or by handle (DLL). Two separate implementations (CheckNoInit(module_name) vs CheckNoInit(ModuleInfo*)); adding a new core module requires touching both.  
6. **LoadModule is large and mixed**: Path discovery (DLL), dependency checking, lazy registration, and initialization are all in one function; static and DLL branches duplicate the dependency loops.

**Proposed direction**  
1. **Unify “depends on” into one concept**  
   - Either keep both `GetDependencies()` and `LoadAfter()` but document “union of both = all modules that must be loaded before this one” and handle them in one place, or deprecate `LoadAfter()` and use only `GetDependencies()` (with a single loop in the manager).  
2. **Introduce an explicit dependency graph**  
   - Build a directed graph: nodes = module names (known from AddModule / RegisterStaticModule / directory scan); edges = A → B if A depends on B (B must load before A).  
   - Source: for each module that has an IModule instance (or a way to query deps without full load), add edges from module to each of GetDependencies() and LoadAfter().  
   - Core/special modules are excluded from the managed graph (they are pre-loaded and not in ModuleManager’s loaded set).  
3. **Validation**  
   - **Cycle detection**: Before loading, or when building the graph, run a cycle check (e.g. DFS or Kahn). If a cycle exists, fail fast with a clear error listing the cycle.  
   - **Missing dependencies**: When building the graph, ensure every referenced dependency is a known module (or a known core/special name). If not, fail or warn with a clear message.  
4. **Explicit load and shutdown order**  
   - **Load order**: Topological sort of the dependency graph (dependencies first). Use this order when loading (e.g. LoadModuleAll iterates in this order instead of arbitrary directory/registration order).  
   - **Shutdown order**: Reverse of load order (already planned in “Graceful module cleanup”). Optionally store the resolved order so that after Shutdown() removes core entries from the map, we still have a consistent list of “what to shut down” and “what to Destroy”.  
5. **Refactor for clarity**  
   - **Extract dependency logic**: e.g. a small `ModuleDependencyGraph` or free functions that: (1) collect all module names and their dependency lists (from IModule or a registry), (2) build the graph, (3) validate (cycles, missing), (4) return a topological load order. ModuleManager::LoadModuleAll() and LoadModule() call into this instead of inlining lazy logic.  
   - **Separate path discovery from loading**: AddModule() / path resolution (DLL search) stays; the actual “ensure dependencies and then init” step uses the graph and order.  
   - **CheckNoInit**: Prefer a single list of “core/special module names” (or handles) in one place (e.g. config or a function that takes name/handle and returns bool). Reduce duplication between IS_DLL and !IS_DLL.

**Implementation steps**  
1. **Unify dependency collection**  
   In ModuleManager (or a helper), collect “all modules that must load before me” as the union of GetDependencies() and LoadAfter() in one place. Use this single list for both static and DLL branches; remove the duplicate loops. Optionally add IModule::GetRequiredModules() that returns that union and deprecate LoadAfter() if desired.  
2. **Add ModuleDependencyGraph (or equivalent)**  
   - Input: set of module names + for each name, optional list of required modules (from IModule when available; core modules have no IModule in the manager).  
   - Build directed edges: for each module M and each required R, edge M → R.  
   - Validate: (a) no cycle (report cycle if found), (b) every required name is either a known module or a known core/special name.  
   - Output: topological order (load order) and optionally reverse order (shutdown order).  
3. **Integrate with LoadModuleAll / LoadModule**  
   - LoadModuleAll: (1) discover all modules (directory scan or static registry), (2) for each, get dependency list (may require loading the DLL to get IModule, or maintain a separate manifest; see below), (3) build graph, validate, get order, (4) load in topological order.  
   - **Catch-22**: To get GetDependencies() we need IModule, which we get after LoadLibrary and GetProcAddress(InitializeModule). So the current “load, then check deps, then maybe defer” approach is natural for DLLs. Refactor can still: (a) build the graph incrementally as modules are loaded (first pass: load all that have no deps, then those whose deps are loaded, etc., using the same topological idea), or (b) use an external manifest (e.g. a per-DLL .json or build-time generated list) so we can compute order without loading. Option (a) keeps single source of truth in IModule; option (b) allows validation before any LoadLibrary.  
4. **Validate and fail fast**  
   - After building the graph (incrementally or from manifest), run cycle detection. On cycle: log or throw with the cycle listed.  
   - On missing dependency: fail or warn before loading.  
5. **Centralize CheckNoInit**  
   - Replace the two CheckNoInit implementations with one that takes (name, optional handle) and checks against a single list or predicate (e.g. “is core module” table or function).  
6. **Document and test**  
   - Document the dependency model in ModuleRegistration.h or a short “Module lifecycle / dependency” doc. Add a unit test or small test that builds a graph with a cycle and asserts validation fails.

**References**  
- `Engine/Source/Runtime/CoreModule/Private/ModuleManager.cpp` — LoadModule (LoadAfter + GetDependencies loops, TryResolveLazyness, m_lazy_modules_, m_module_load_order_), LoadModuleAll, CheckNoInit (static vs DLL).  
- `Engine/Source/Runtime/CoreModule/Public/ModuleManager.h` — m_module_loaded_, m_module_paths_, m_lazy_modules_, m_module_load_order_.  
- `Engine/Source/Runtime/CoreModule/Public/IModule.h` — GetDependencies(), LoadAfter().  
- Various modules (RenderPipeline, Shader, Texture, RaytracingShader, Sound, etc.) — override LoadAfter() / GetDependencies().

### Plan: Migrate Directory.Build.targets to Sharpmake

**Current state**
- **Directory.Build.targets** (repo root): Defines `_GeneratedCppDir` and target `CleanHeaderGenerated` (BeforeTargets="Clean") to remove `Intermediate\HeaderParser\$(MSBuildProjectName)\HeaderGenerated` on Clean/Rebuild. Auto-imported by MSBuild for all projects under the root.
- **Build/HeaderGenerated.targets**: Same logic + ClCompile discovery (`$(_GeneratedCppDir)\**\*.generated.cpp`). Imported **per configuration** via `conf.CustomTargetsFiles` and `conf.CustomProperties["HeaderGeneratedRoot"]` from CommonProject. Clean from this file was not reliable for FastBuild/NMake projects (Clean may run NMakeCleanCommandLine instead of the MSBuild Clean target).

**Goal**
- All behaviour (discovery of *.generated.cpp and cleanup on Rebuild) driven by Sharpmake-generated project/solution so that Directory.Build.targets can be removed or reduced to a comment.

**Options**

| Option | Description | Pros / cons |
|--------|-------------|-------------|
| **A. Append to NMakeCleanCommandLine** | Extend Sharpmake so FastBuild projects can append an optional clean command (e.g. `if exist "…\HeaderGenerated" rmdir /s /q "…"`). CommonProject would set this via a new conf option or a property the platform template reads. | Clean runs reliably for NMake/FastBuild because it’s part of the same command. Requires changing Sharpmake platform (BasePlatform.Vcxproj.Template.cs and the generator that fills `[options]` or NMakeCleanCommandLine). |
| **B. Project-level Import + Clean target** | Add Build/HeaderGenerated.targets to **project**-level CustomTargetsFiles (not per-config) so the Import has no Condition and the Clean target is always in scope. Rely on MSBuild running the Clean target (and thus BeforeTargets="Clean") before or as part of Clean. | No Sharpmake core change. May still not run if the build only runs NMakeCleanCommandLine for Clean. |
| **C. Keep Directory.Build.targets for Clean only** | Keep cleanup in Directory.Build.targets; keep discovery and HeaderGeneratedRoot in Sharpmake (HeaderGenerated.targets via CustomTargetsFiles). | No migration of cleanup; Directory.Build.targets stays as source of Clean behaviour. |

**Recommended**
- **A** for full migration: add support in Sharpmake for an optional “additional NMake clean” fragment (e.g. `conf.AdditionalNMakeCleanCommands` or a CustomProperty that the FastBuild platform template appends to NMakeCleanCommandLine). CommonProject sets it to remove `$(HeaderGeneratedRoot)\Intermediate\HeaderParser\$(ProjectName)\HeaderGenerated` (using a path that resolves at build time). Then remove the Clean target from Directory.Build.targets and from Build/HeaderGenerated.targets (targets file can keep only ClCompile discovery and PropertyGroup).

**References**
- Directory.Build.targets: repo root; Build/HeaderGenerated.targets: Build/; CommonProject: Build/CommonProject.build.cs (CustomTargetsFiles, HeaderGeneratedRoot); Sharpmake FastBuild template: Programs/Sharpmake/Sharpmake.Platforms/Sharpmake.CommonPlatforms/BasePlatform.Vcxproj.Template.cs (NMakeCleanCommandLine).

### Plan: Git commits (split by features and fixes)

Commit in this order. Only includes build/solution/prebuild/header-parser scope; other working-tree changes (Texture consolidation, Core/RenderPipeline moves, Client, etc.) should be split into separate commits or a separate branch.

| # | Type    | Title | Files |
|---|--------|-------|-------|
| 1 | **Feature** | Merge HeaderGenerated.targets into Sharpmake (vcxproj discovery + stop importing targets) | `Build/CommonProject.build.cs` (remove CustomTargetsFiles, keep HeaderGeneratedRoot + AdditionalNMakeCleanCommands), `Build/HeaderGenerated.targets` (stub to comment), `Directory.Build.targets` (comment update), `Programs/Sharpmake/Sharpmake.Generators/VisualStudio/Vcxproj.Template.cs` (ProjectHeaderGeneratedDiscovery block), `Programs/Sharpmake/Sharpmake.Generators/VisualStudio/Vcxproj.cs` (emit discovery when HeaderGeneratedRoot set). |
| 2 | **Feature** | Generated tracking headers: use filename-only includes | `Programs/header-parser/main.cc` (emit `_component_tracking.generated.h` etc. without `Public/` prefix). |
| 3 | **Feature** | Replace SolutionPrebuild project with prebuild events | `Build/Utils.cs` (add `AddSolutionPrebuildSteps`: BuildBalius + optional GenerateSolution), `Engine/Launch/Launch.build.cs` (call AddSolutionPrebuildSteps, remove SolutionPrebuild dep/include), `Engine/Monolith/Monolith.build.cs` (same), `Engine/Kolibri.build.cs` (remove AddProject SolutionPrebuild, remove include). Delete `Build/SolutionPrebuild/` if still present. |
| 4 | **Fix** | Sharpmake build script: use Project.Configuration.BuildStepExecutable | `Build/Utils.cs` (replace `Configuration.BuildStepExecutable` with `Project.Configuration.BuildStepExecutable`). |
| 5 | **Fix** | FastBuild vcxproj: avoid orphan NMakeCleanCommandLine closing tag | `Programs/Sharpmake/Sharpmake.Platforms/Sharpmake.CommonPlatforms/BasePlatform.Vcxproj.Template.cs` (add `rem FastBuild clean` line so block is never empty). Optionally fix existing `Kolibri_All.vcxproj` in same commit. |
| 6 | **Fix** | FastBuild BuildBalius Exec: ExecOutput, args, cargo path | `Build/Utils.cs` (set output log path, input/args order; resolve cargo via CARGO_HOME / %USERPROFILE%\.cargo\bin\cargo.exe). |

**Notes**
- Commits 1–3 are features; 4–6 are fixes. Apply fixes after the feature commits they depend on (e.g. 4 and 6 touch Utils.cs after 3 adds it).
- Regenerate solution/BFF after commits 1, 3, 5, 6 so generated `.sln`/`.vcxproj`/`.bff` reflect changes.
- Memory-bank docs (`memory-bank/*.md`) can be updated in the same commit as the change they describe or in a single “docs: memory-bank” commit.

## Previous task (plan)
- **Task**: Plan — how Sharpmake includes headers/cpps and how the solution discovers generated output.
- **Status**: Plan complete. See “Plan: How Sharpmake includes headers/cpps and how the solution discovers generated output” below. Summary: headers via include paths (discovered at compile time); .cpp/.generated.cpp via BFF file list (must exist at solution generation time; generated .cpp only in BFF if folder exists when Sharpmake ran; see Mitigations.)

### Implementation plan: GetResourceByRawPath (ResourceManager)

**Context**
- `Resource.h` and header-parser generated code call `ResourceManager::GetInstance().GetResourceByRawPath<T>(path)`.
- ResourceManager currently has `GetResourceByMetadataPath` (search by `GetMetadataPath()`) but no `GetResourceByRawPath` (search by `GetPath()` = raw file path).
- Resource stores raw path in `m_path_` / `GetPath()`; Entity stores metadata path in `GetMetadataPath()`.

**Planned changes**
1. **ResourceManager.h**: Add public `GetResourceByRawPath<T>(const std::filesystem::path& path)` template and non-template `GetResourceByRawPath(path, type)`; add private `SearchResourceByRawPath(path, type)` and its const overload (mirror metadata API).
2. **ResourceManager.cpp**: Implement `GetResourceByRawPath(path, type)` (search by raw path; optionally load from path if not found, mirroring metadata path behavior if desired) and `SearchResourceByRawPath(path, type)` (find resource where `resource->GetPath() == path`).

**References**
- Existing: `GetResourceByMetadataPath`, `SearchResourceByMetadata` (ResourceManager.h/cpp).
- Callers: Resource.h `RESOURCE_SELF_INFER_GETTER`, bodygeneration_macro.h `bodyGenerationResourceGetter`.

**Implementation (done)**
- ResourceManager.h: Added `GetResourceByRawPath<T>(path)` and `GetResourceByRawPath(path, type)`; added `SearchResourceByRawPath(path, type)`.
- ResourceManager.cpp: Implemented both; search uses `resource->GetPath() == path`; getter ensures Load() if found.

### Plan: What happens if you remove GetByRawPath

**Current usage**
- **Definitions only**: The API is **declared** in (1) `Resource.h` macro `RESOURCE_SELF_INFER_GETTER`, (2) header-parser `bodygeneration_macro.h` (so every ECLASS resource gets `GetByRawPath(path)` in its generated header), and (3) `ResourceManager.h`/`.cpp`.
- **No call sites**: Grep shows **no code** calls `GetByRawPath(...)` or `GetResourceByRawPath(...)` anywhere. So removing it does not change current runtime behavior.

**If you remove it**
1. **You must remove it in three places** or the build breaks:
   - **ResourceManager.h**: Remove `GetResourceByRawPath<T>(path)`, `GetResourceByRawPath(path, type)`, and `SearchResourceByRawPath(path, type)`.
   - **ResourceManager.cpp**: Remove implementations of `GetResourceByRawPath(path, type)` and `SearchResourceByRawPath(path, type)`.
   - **Resource.h**: Remove the `GetByRawPath` line from `RESOURCE_SELF_INFER_GETTER`.
   - **Programs/header-parser/bodygeneration_macro.h**: Remove the two lines that generate `GetByRawPath` in `bodyGenerationResourceGetter`. Then **regenerate** all `.generated.h` (run header-parser for each project, or delete `Intermediate/HeaderParser/*/HeaderGenerated` and rebuild) so existing generated headers no longer declare `GetByRawPath`.
2. **Effect**: You lose the ability to look up a resource by its **raw file path** (`GetPath()`). Lookup by **name** (`Get`) and by **metadata path** (`GetByMetadataPath`) remain. If you later need “find resource by the path it was loaded from,” you would have to re-add this API or implement a different mechanism.

**Recommendation**
- If you do not need raw-path lookup: remove it in all four places and regenerate generated headers for a simpler API surface.
- If you might need it later (e.g. “get texture by path”) or want symmetry with metadata-path lookup: keep it as implemented; cost is small and there are no call sites yet.

## Plan: How Sharpmake includes headers/cpps and how the solution discovers generated output

**Scope:** Trace and document how headers and .cpp files are included, and how generated headers (and .generated.cpp) are discovered. No code changes unless the plan reveals a fix.

### 1. How Sharpmake sets include paths (headers)

**Where:** `Build/CommonProject.build.cs` → `ConfigureAll` → “Include” block (conf.IncludePaths, conf.IncludePrivatePaths).

**Per-project (CommonProject-derived, e.g. Client, AnimationTexture):**

| Path added | Purpose |
|------------|--------|
| `conf.IncludePrivatePaths.Add(conf.ProjectPath + "/Private")` | Project’s Private folder. |
| `HeaderParserTargetDir + "/Public"` | Generated headers under `Intermediate/HeaderParser/[project.Name]/HeaderGenerated/Public`. |
| `HeaderParserTargetDir + "/Private"` | Same for `.../Private`. |
| `[project.SourceRootPath]` | Project source root (e.g. Client/, or Engine module path). |
| `[project.SourceRootPath]/Public` | Project Public. |
| `SolutionDir + "/Engine"` | Engine tree. |

**HeaderParserTargetDir:**  
`headerGeneratedRoot + "/Intermediate/HeaderParser/[project.Name]/HeaderGenerated"`  
with `headerGeneratedRoot = EngineDir ?? SolutionDir`.

So **generated headers** are discovered by the compiler because **include paths** point at `HeaderGenerated/Public` and `HeaderGenerated/Private`. Any `#include "X.generated.h"` resolves as long as that path is set and the file exists when the compile runs (created by balius/header-parser in prebuild).

**Monolith (static lib):**  
`Engine/Monolith/Monolith.build.cs` adds, for **each** dependency project,  
`Intermediate/HeaderParser/<DepName>/HeaderGenerated/Public` and `.../Private` to `conf.IncludePaths`. So Monolith sees every dependency’s generated headers.

### 2. How Sharpmake collects source files (.cpp and .generated.cpp)

**Where:** Project constructor and `ConfigureAll` do **not** explicitly list .cpp files. Sharpmake infers sources from:

- **SourceRootPath** (e.g. `Client/` or module path from `GetSharpmakeFilePath()`).
- **AdditionalSourceRootPaths** (e.g. `Intermediate/HeaderParser/<ProjectName>/HeaderGenerated`).

**When:** At **solution generation time** (when `GenerateSolution.bat` runs Sharpmake). Sharpmake scans those roots and emits the **current** set of files into the generated project (vcxproj) and **BFF** (FastBuild).

**Implication:**  
- **Headers** only need to exist at **compile time** (prebuild creates them before compile). Include paths are fixed in the generated project, so generated headers are “discovered” by the compiler via include path.  
- **.cpp / .generated.cpp** must exist **at solution generation time** to be in the BFF. If `HeaderGenerated` is empty when Sharpmake runs, no `.generated.cpp` are added → FastBuild never compiles them → linker errors for generated symbols.

**Why Client does not detect generated .cpp (re-evaluation):**  
Sharpmake collects sources at **solution generation time** by scanning directories. In `Project.cs` it iterates `AdditionalSourceRootPaths` and calls `GetDirectoryFiles(additionalSourceRootPath)` (→ `Util.DirectoryGetFiles`). In Sharpmake's `FakeFileTree.cs`, when the path does not exist, `Directory.Exists(path)` is false and the function returns `new string[] { }` (no exception). So when `Intermediate/HeaderParser/Client/HeaderGenerated` **does not exist or is empty** when GenerateSolution runs, zero files are added → BFF has no Client `.generated.cpp` → FastBuild never compiles them → linker errors. Solution Explorer shows no “intermediate” node. We do **not** run balius for Client from the solution-level prebuild, so that folder is typically missing when Sharpmake runs; Engine projects often have the folder from a prior build, so their generated files are included.

### 3. How the solution “discovers” generated headers (summary)

| Mechanism | Headers (.generated.h) | Sources (.generated.cpp) |
|-----------|------------------------|---------------------------|
| **Discovery** | Via **include path** (HeaderGenerated/Public, .../Private). File must exist at **compile** time. | Via **source list** in BFF/vcxproj. Files must exist at **solution generation** time to be in the list. |
| **When created** | Project prebuild (balius → header-parser) before compile. | Same prebuild. |
| **Who uses** | Compiler (include paths set by Sharpmake in generated project). | FastBuild (BFF generated by Sharpmake). |

So: **generated headers** are “discovered” by the **compiler** using **include paths** set by Sharpmake; **generated .cpp** are “discovered” by the **build** only if they were **present when Sharpmake ran** and thus included in the BFF.

### 4. Mitigations (no balius for Client from solution prebuild)

- **Directory.Build.targets** (repo root) adds `ClCompile` for `$(_GeneratedCppDir)/**/*.generated.cpp`. This affects **vcxproj** and can help IDE / MSBuild-driven builds; **FastBuild** uses the BFF file list, which is fixed at solution generation time.
- **Without** running balius for Client before GenerateSolution, options are: (1) Run header-parser/balius for Client **manually** before running GenerateSolution so the folder exists; (2) Build Client once (prebuild creates the folder and .generated.cpp), then run GenerateSolution again so the BFF is regenerated with those files, then build again; (3) Implement build-time source discovery in the BFF — non-trivial.  
### 5. References

- Include paths: `Build/CommonProject.build.cs` (Include block), `Engine/Monolith/Monolith.build.cs` (dependency HeaderGenerated paths).
- Source roots: `Build/CommonProject.build.cs` (SourceRootPath, AdditionalSourceRootPaths).
- Prebuild: `Build/CommonProject.build.cs` (EventCustomPrebuildExecute balius), `Utils.AddSolutionPrebuildSteps` from Launch/Monolith (BuildBalius + GenerateSolution; no RunBaliusClient).
- Generated output layout: header-parser writes to `HeaderGenerated/<firstDir>/` (e.g. Public/), see `Programs/header-parser/main.cc` (GetDestinationPath, cppStream).

---

## Implementation plan: Build pipeline (evaluated)

### 1. Solution and project generation
- **Solution file** is produced by **GenerateSolution.bat** (not checked in as source of truth). The batch file: builds Sharpmake (dotnet build Sharpmake.Application), optionally generates header-parser.vcxproj via CMake (VS 18 2026, x64), then runs Sharpmake with three sources: EngineMain.build.cs, ClientMain.build.cs, KolibriMain.build.cs.
- **Projects** (vcxproj, BFF) are generated from **each build.cs**: EngineSolution.build.cs uses `[module: Include("%EngineDir%/Engine/Source/Runtime/**/**.build.cs")]` and calls `conf.AddProject<Core>(target)` etc.; ClientSolution adds ClientProject; Kolibri.build.cs adds header-parser (when vcxproj exists), MonolithClient/MonolithServer, Launch. Solution prebuild (balius + GenerateSolution) runs as Launch/Monolith prebuild events.
- **Rule**: If any **build.cs** is modified (new project, dependency, path, option), the solution and project files must be **re-generated** by running **GenerateSolution.bat** (or by building Launch/Monolith, whose prebuild runs it). The build system does not auto-regenerate on build.cs changes.

### 2. Prebuild: Balius (header modification detection → header-parser)
- Before compiling a project, **balius** is invoked per project (CommonProject `EventCustomPrebuildExecute`): `balius.exe <EngineDir> <ProjectName> <SourceRoot> <GitDir> <ConfName>`.
- Balius: (1) Computes SHA256 hash of all .h/.hpp under project source root; if hash unchanged, skips (still ensures HeaderGenerated dirs exist). (2) Otherwise: ensures per-project git repo at `Intermediate/HeaderParser/<ProjectName>`, copies headers there, ensures `<Conf>.dep` for dependency info, runs `git status --porcelain` to detect changed files. (3) Commits in that repo; if there were changes, writes the **target file** (list of changed header paths, one per line) and runs **header-parser.exe** with that target file and tag options (-e EENUM, -c ECLASS, -p EPROPERTY, -f EFUNC, -m GENERATE_BODY, -b &lt;configuration&gt;). (4) Saves hash to `Intermediate/HeaderParser/Hash/<ProjectName>.hash`.
- So: **balius** = header modification detection + copy + git + **passes list of headers to header-parser** via the target file.

### 3. Header-parser (definitions and implementation from file macros)
- **header-parser** reads the **target file** (path to which is first CLI arg; produced by balius). Each line is a path to a header (relative to `Intermediate/HeaderParser/<ProjectName>`). It also reads `<firstDir>/<Conf>.dep` for module dependencies.
- For each header it: parses (JSON/C++ parsing), then generates output using **file macros** in **bodygeneration_macro.h**: `generatedHeaderFormat` (GENERATE_BODY_HEAD, GENERATE_BODY_STATIC, polymorphic_type_hash, etc.), and **postGenerated** (s_dependencies_, cloneImpl, BOOST serialization, resource static_assert) appended to **cppStream** so that **.generated.cpp** contains the definitions and implementations. Output paths: `.generated.h` and `.generated.cpp` under `HeaderGenerated/<firstDir>/` (e.g. Public/).
- So: **header-parser** parses headers from balius and **generates required definitions and implementations** by the **file macro** (bodygeneration_macro.h) and postGenerated stream to .generated.cpp.

### 4. Compilation: FastBuild (aligned by Sharpmake)
- Actual compile/link is done by **FastBuild** (FBuild.exe from `Programs/Sharpmake/tools/FastBuild/Windows-x64/`). Sharpmake generates the BFF and vcxproj that invoke FastBuild; Monolith (e.g. Kolibri_All) sets `conf.IsFastBuild = true` and `FastBuildSettings.FastBuildMakeCommand`. So the **build is started with FastBuild**, and its layout (targets, dependencies) is **aligned by Sharpmake** (which generated the BFF and project references).

### Order of operations (when user builds)
1. Launch/Monolith prebuild runs: build balius (cargo), then run GenerateSolution.bat → solution/vcxproj/BFF up to date.
2. For each project (e.g. when building Monolith): header-parser vcxproj is built first if present; then per-project prebuild runs balius → balius may run header-parser → generated files land in HeaderGenerated.
3. FastBuild compiles and links using the generated files and include paths set by Sharpmake.

## Build fix (implemented)
- **RaytracingShader.cpp**: Needed full type `IRaytracingExtension` for `Load_INTERNAL()`. Added dependency **RenderPipeline** in RaytracingShader.build.cs; include `"IRaytracingExtension.h"` (short form—dependency’s Public is on include path). No full path needed when project depends on the other module.
- **Guideline**: Prefer forward declarations; include only when full type required. When project **depends on another project** (build.cs), use **short includes**; dependency’s Public folder is already on the include path.

## Investigation: Why some generated headers are missing (plan)

**Build errors recalled**
- `ClientModule.generated.h`: No such file or directory (when building client unity, e.g. Kolibri_All).
- `ShadowIntersectionRenderTask.generated.h`: No such file or directory (same build).

**Root cause (investigation)**

1. **Who runs balius**
   - **CommonProject** (Build/CommonProject.build.cs) adds the balius prebuild and `Intermediate/HeaderParser/[project.Name]/HeaderGenerated` to **IncludePaths**.
   - **ClientProject** extends CommonProject → when the **Client** project is built, balius runs with `Name = "Client"` and writes to `Intermediate/HeaderParser/Client/HeaderGenerated/`.
   - **Monolith** (MonolithClient / Kolibri_All) extends **Project**, not CommonProject → it does **not** add the balius prebuild and does **not** add any `HeaderParser/.../HeaderGenerated` include paths.

2. **How the Monolith is built**
   - Monolith uses `AdditionalSourceRootPaths` from Client + Engine solution projects (so it compiles Client + Engine sources).
   - Monolith does **not** add `Intermediate/HeaderParser/Client/HeaderGenerated` (or any dependency’s HeaderGenerated) to its include path.
   - So when the Monolith compiles `Client/ClientModule/Public/ClientModule.h`, the include `"ClientModule.generated.h"` is resolved from the **source** roots (e.g. `Client/`, `Client/ClientModule/Public/`). The file produced by balius lives under `Intermediate/HeaderParser/Client/HeaderGenerated/Public/`, which is **not** on the Monolith’s include path.

3. **Why Client generated headers are “not generated” from the Monolith’s point of view**
   - If the **Client** project is built first (as a dependency), balius runs for Client and **does** generate files under `Intermediate/HeaderParser/Client/HeaderGenerated/`.
   - Those files exist, but the **Monolith** never looks there because it never adds that path.
   - So from the Monolith build: “some headers are not generated” = **generated headers are produced in a location the Monolith does not search**.

**Fix options**

| Option | Description | Pros / cons |
|--------|-------------|-------------|
| **A. Monolith include path** | In Monolith.build.cs, add `Intermediate/HeaderParser/<DepName>/HeaderGenerated/Public` (and `/Private`) for every dependency project (Client + Engine projects that use ECLASS). Ensure Client (and others) are built before Monolith so balius has run. | Correct long-term; Monolith sees the same generated headers as each project. Requires iterating dependencies and possibly filtering which projects have HeaderGenerated. |
| **B. Client-only path** | Add only `Intermediate/HeaderParser/Client/HeaderGenerated/Public` and `.../Private` to Monolith’s include path, and ensure Client project build (and thus Client balius) runs before Monolith. | Fixes Client missing headers with minimal change; other modules already work if Monolith gets their sources via a different mechanism or doesn’t compile their .h directly. |
| **C. Stub in source tree** | Keep hand-written or script-generated `.generated.h` (and optionally `.generated.cpp`) next to Client ECLASS headers (e.g. `Client/ClientModule/Public/ClientModule.generated.h`, `Client/RenderTasks/Public/ShadowIntersectionRenderTask.generated.h`) so the Monolith finds them via existing source-root include path. | No build.cs changes; works immediately. Downside: duplicates logic of header-parser and can drift (manual maintenance or custom script). |

**Recommended next steps**

1. ~~**Verify** that building the **Client** project alone runs balius and produces `Intermediate/HeaderParser/Client/HeaderGenerated/Public/ClientModule.generated.h` (and similar).~~
2. ~~**Implement Option B** (or A): add HeaderGenerated include paths in Monolith.build.cs.~~ **Done:** Monolith now adds `Intermediate/HeaderParser/<DepName>/HeaderGenerated/Public` and `.../Private` for **every** dependency project (Client + all Engine projects in the solution). This matches Option A; no balius/header-parser code changes required.
3. **Regenerate solution** after the Monolith.build.cs change (run GenerateSolution.bat or build Launch/Monolith).
4. Build order already ensures dependencies (Client, Engine projects) build before Monolith, so their balius prebuilds run and generated headers exist before the Monolith compiles. Optional: remove manual stubs `Client/ClientModule/Public/ClientModule.generated.h` and `Client/RenderTasks/Public/ShadowIntersectionRenderTask.generated.h` once the pipeline is verified so the single source of truth is header-parser output.

**References**
- Build pipeline: tasks.md § Implementation plan (solution generation, balius, header-parser, FastBuild).
- CommonProject: `Build/CommonProject.build.cs` (balius prebuild, HeaderParser include paths).
- Monolith: `Engine/Monolith/Monolith.build.cs` (no HeaderParser paths, uses AdditionalSourceRootPaths only).

## Backlog
- **Load timing enum (load phases)**: Implement Plan: Load timing enum (load phases) — add ELoadPhase (Core, UI, Graphic, Internal, etc.), IModule::GetLoadPhase(), and phase-aware sort in ModuleManager so duplicated manual LoadAfter lists can be replaced by phase.
- **ModuleManager dependency refactor**: Implement Plan: ModuleManager — enhanced dependency management (unify GetDependencies/LoadAfter, explicit DAG, cycle detection, topological order, refactor LoadModule, centralize CheckNoInit).
- ~~**Graceful module cleanup**~~ **Done**: Reverse shutdown order (ModuleManager), OnModuleShutdown adopted (Renderer, ResourceManager), teardown sequence documented (ModuleRegistration.h), Launch.cpp deinit order aligned.
- Re-run build to confirm (previous run hit EXEC error 0xc0000409 on RaytracingShader.obj—possible runtime/crash; fix applied).
- Run header parser for all modules using .generated.h as needed.
- Implement Monolith include path for Client (or all deps) HeaderGenerated per investigation above.

## Plan: Git commit chunks

**Status**: Executed. Fixes 1–2 and features 3–6 were committed (submodule commits in Programs/Sharpmake and Programs/header-parser; root commits for submodule refs, build files, docs). Items 2+5 and 3+4 were combined in single commits to avoid splitting main.cc and CommonProject.build.cs. **Graceful module cleanup** (separate chunk): fix(Launch) align deinit order, docs(module) teardown sequence and status (ModuleRegistration.h, tasks.md, progress.md).

Split and chunk commits into **fixes** (small, targeted) then **features** (by area). Order so the tree keeps building.

### Fixes (first)

| # | Commit title | Scope | Description |
|---|----------------|--------|-------------|
| 1 | **fix(build): FastBuild LIB so link.exe finds ntdll.lib** | MasterBff.cs | Add `GetWindowsSdkLibPathForBff()`, prepend Windows SDK Lib path to LIB in `envAdditionalVariables` when LIB does not already contain "Windows Kits". Fixes LNK1181 when building e.g. balius under FastBuild. |
| 2 | **fix(header-parser): correct include path in tracking .generated.h** | main.cc | Use `GetIncludePathForTracking(path)` (path as-is) instead of `DropFirstDirectory` when writing `#include` lines in component/object/resource tracking headers. Aligns with project includes (e.g. `Components/Public/CubifyComponent.h`). |

### Features (by area)

| # | Commit title | Scope | Description |
|---|----------------|--------|-------------|
| 3 | **build: remove balius prebuild stage** | CommonProject.build.cs, Launch.build.cs, Monolith.build.cs | Remove calls to `AddBuildBaliusPrebuildStepFirst` and `AddSolutionPrebuildSteps`. Balius is built manually (Setup.ps1 / cargo) before building the solution. |
| 4 | **build: FastBuild PATH and LIB from Utils** | Build/Utils.cs, Build/CommonProject.build.cs | Add PATH/LIB helpers in Utils; set `AdditionalGlobalEnvironmentVariables["PATH"]` and `["LIB"]` for win64 so FastBuild Execs see Rust/VC and SDK/VC libs. |
| 5 | **build(header-parser): add --max-jobs and HEADER_PARSER_MAX_JOBS** | Programs/header-parser/main.cc | Add `-j` / `--max-jobs`. Env `HEADER_PARSER_MAX_JOBS` overrides. When 1, use `std::execution::seq` to reduce thread count. |
| 6 | **docs: techContext Balius, FastBuild PATH/LIB, header-parser max-jobs** | memory-bank/techContext.md | Document balius build-before-solution, three-path and LIB in BFF, and HEADER_PARSER_MAX_JOBS / --max-jobs. |

### Other branch changes (chunk by theme)

If the working tree also contains unrelated refactors (texture consolidation, render pipeline moves, Core changes), chunk them in separate commits by theme, e.g.:

- **refactor(Texture): consolidate Texture1D/2D/3D into Texture module**
- **refactor(RenderPipeline): move RenderPassTask, RenderInstanceTask, RenderType to RenderPipeline**
- **refactor(Core): hash-compare header and related API changes**
- **chore: solution/scripts and regeneration** (GenerateSolution.bat, Setup, submodule if needed)

Order: fixes 1–2 → features 3–6 → other refactor commits so the solution keeps building.
