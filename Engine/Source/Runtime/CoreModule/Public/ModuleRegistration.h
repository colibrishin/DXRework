#pragma once

#include <array>
#include <string_view>

#include "IModule.h"

/**
 * --- Load phase order (ELoadPhase) ---
 * Phase order: Core (0) < Graphic (1) < UI (2) < Internal (3) < Application (4).
 * Managed modules are loaded by ModuleManager in this phase order; within each phase,
 * order is given by GetDependencies() (topological sort).
 * Core modules are loaded manually in Launch.cpp; their load order follows
 * kCoreModuleLoadOrder below (ELoadPhase::Core).
 * See memory-bank/tasks.md "Loading phase order for every module" for the full table.
 */

/// Manual (core) module load order; corresponds to ELoadPhase::Core. Launch.cpp loads and
/// unloads in this order (cleanup in reverse). Last entry is the graphic interface module.
inline constexpr std::array<std::wstring_view, 5> kCoreModuleLoadOrder = {
    L"Memory", L"CoreModule", L"Core", L"WinAPIWrapper", L"GraphicInterface" };

/**
 * --- Teardown sequence (single source of order) ---
 * Non‑monolith (e.g. Launch.cpp) MUST follow this order so dependents shut down before
 * dependencies and no code holds pointers into unloaded DLLs.
 *
 * 1. EngineEntryPoint::Destroy()  — stop app loop, release high-level systems
 * 2. ModuleManager::Shutdown()    — managed modules only, in REVERSE load order (dependents first).
 *    Each module: OnModuleShutdown callbacks run, then IModule::Shutdown(). Core modules are skipped.
 * 3. Core cleanup                — cleanup_seq(g_core_api). No use of Graphic/OS beyond teardown.
 * 4. GraphicInterface cleanup   — cleanup_seq(g_graphic_api)
 * 5. WinAPIWrapper cleanup       — cleanup_seq(g_os_api)
 * 6. Global allocator cleanup    — g_static_alloc release/purge, g_allocator_storage.cleanup()
 * 7. ModuleManager::Destroy()   — FreeLibrary for managed DLLs (reverse order)
 * 8. CoreModule cleanup          — cleanup_seq(g_module_api). ModuleManager lives here; destroy after managed unload.
 * 9. Memory cleanup              — cleanup_seq(g_core_mem) last so no code holds type_hash/allocators from other DLLs.
 *
 * Subsystems must not hold references into a module after its OnModuleShutdown callback has run.
 */

/**
 * Generic pattern for module-scoped registration and full cleanup on DLL unload.
 *
 * --- For subsystems (Renderer, etc.) ---
 * 1. Add an optional final parameter (e.g. std::wstring_view module_name = {}) to each
 *    Register* API. When IS_DLL and module_name non-empty, store the registration under
 *    that module name.
 * 2. Implement void UnregisterModule(std::wstring_view module_name) that removes every
 *    registration recorded for that module and clears any cached state (e.g. resolver
 *    caches that hold pointers into unloaded code).
 * 3. Register with ModuleManager so cleanup runs before the DLL's Shutdown():
 *      ModuleManager::GetInstance().RegisterOnModuleShutdown(
 *          []( std::wstring_view name ) { YourSubsystem::GetInstance().UnregisterModule( name ); } );
 *
 * --- For modules (DLLs) ---
 * 1. When calling a subsystem's Register* from InitializeImpl(), pass the current module
 *    name so the subsystem can track by module. Use ENGINE_MODULE_SCOPE at the end of
 *    the argument list if the API accepts an optional module_name as last parameter.
 * 2. ShutdownImpl() can still call Unregister* for symmetry; the engine will have
 *    already run UnregisterModule(GetModuleName()) before Shutdown(), so those calls
 *    are no-ops. Alternatively, rely entirely on the engine callback for cleanup.
 *
 * --- Declaration macro (header parser) ---
 * You can declare a module's registrations in one place and use a header parser or
 * codegen to generate InitializeImpl/ShutdownImpl or to drive UnregisterModule. For
 * example:
 *   MODULE_REGISTRATIONS(DeferredRenderPassTask,
 *       (RenderPass, "DeferredRenderPassTask")
 *       (RenderPassWith, "DeferredRenderPassTask", SHADER_DOMAIN_OPAQUE))
 * The parser can emit a table or macro expansion so every module follows the same pattern.
 */

/// Appends ", GetModuleName()" for use as the final argument to subsystem Register* APIs
/// that accept an optional module name. Use only inside IModule::InitializeImpl() (or
/// other code that has GetModuleName() in scope and is the current module instance).
#define ENGINE_MODULE_SCOPE , GetModuleName()
