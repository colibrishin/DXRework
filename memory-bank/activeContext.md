# Active Context

## Current Work Focus
- **Compile-time hash map (constexpr)**: Add constexpr hash set/map from type list with compile-time lookup. See tasks.md § Plan: Compile-time hash map (constexpr). (Directory.Build.targets migration completed.)
- **Migrate Directory.Build.targets to Sharpmake** (completed): Goal is for Sharpmake to generate or set the custom events in the solution so that (1) generated-header cleanup on Rebuild and (2) discovery of *.generated.cpp are both driven by Sharpmake, not by repo-root Directory.Build.targets. See memory-bank/tasks.md § “Plan: Migrate Directory.Build.targets to Sharpmake” for options (recommended: extend NMakeCleanCommandLine in Sharpmake so CommonProject can append removal of HeaderGenerated).
- **Build pipeline (documented)**: Solution from GenerateSolution.bat; balius → header-parser; FastBuild compile. See tasks.md implementation plan. **Balius prebuild removed**—build balius manually (e.g. `cargo build --release` in balius/) before building the solution. FastBuild PATH and LIB (Utils + MasterBff) set so link.exe finds ntdll.lib when needed. Header-parser: correct tracking include path (GetIncludePathForTracking); optional **HEADER_PARSER_MAX_JOBS=1** or **--max-jobs 1** to limit thread count. See techContext.md.

## Next Steps
- Implement constexpr_hash_set / constexpr_hash_map per plan (CoreType.h: sorted array + binary search; contains(cityhash256) / contains(HashType)).
- Update this file when switching tasks or after significant changes.

## Code style
- **Always use braces** for `if`, `for`, `while`, `else` (and similar) — do not skip brackets even for one-line bodies.
- **Parentheses**: space after `(` and before `)` in conditionals and in function/macro argument lists — e.g. `if ( expr )`, `foo( a, b )`, `for ( auto it = ... )`. (Some older code omits these spaces; prefer the spaced form for new/edited code.)
- **Braces**: opening `{` and closing `}` on their own line (same indent as the controlling keyword or block). No space required before/after the `{` or `}` when they are on a separate line.
- **Subscripts**: optional space inside `[ ]` — e.g. `arr[ i ]` is used in parts of the codebase; `arr[i]` also appears; prefer `[ i ]` when touching that code for consistency with Renderer/CoreModule style.
- **Indentation**: tabs for block indentation (e.g. RenderPipeline, CoreModule).

## Include guideline
- Prefer forward declarations; include only when full type is needed (calls, size, layout). Include in .cpp when possible; limit scope to avoid pulling extra dependencies.
- **When the project has a dependency on another project** (via build.cs AddPublicDependency / AddPrivateDependency), that dependency’s **Public** folder is on the include path—**use short includes** (e.g. `"IRaytracingExtension.h"`), not full path. Add the dependency in build.cs if needed, then regenerate the solution.

## Active Decisions
- Core does not depend on other runtime modules; IGraphicAPI uses Abstracts::Resource*; implementations cast at boundary.
- Generated definitions (s_dependencies_, cloneImpl, BOOST) live in .generated.cpp; resource type enforced via static_assert there, not is_base_of in header.
