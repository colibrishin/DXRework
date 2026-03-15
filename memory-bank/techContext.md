# Tech Context

## Technologies
- **Language**: C++20.
- **Graphics**: D3D12 (D3D12GraphicInterface).
- **Build**: Sharpmake (C#), FastBuild, CMake (header-parser).
- **Package manager**: vcpkg (vcpkg_installed); baseline and libs in build files.
- **Other**: Boost (serialization), TBB, FMOD, Rust (balius).

## Build pipeline (summary)
1. **Solution/projects**: From **GenerateSolution.bat**; projects come from **each build.cs**. **If build.cs is modified, re-run GenerateSolution.bat** (or build Launch/Monolith so their prebuild runs it) to regenerate the solution and vcxproj/BFF.
2. **Prebuild**: Balius (header modification detection) runs per project; writes **target file** (list of headers) and calls **header-parser**; header-parser parses those headers and **generates definitions/implementations** via **file macros** (bodygeneration_macro.h) and postGenerated → .generated.h and .generated.cpp.
3. **Compilation**: **FastBuild** does the actual build; layout aligned by Sharpmake.

## Development Setup
- **Windows**: Primary; VS 2022/2026 Build Tools or full IDE. vswhere for VS path.
- **Scripts**: Setup.bat (Setup.ps1); GenerateSolution.bat (Sharpmake, net8.0); DeveloperPrompt.bat (vcvars64, Rust on PATH, opens cmd).
- **Balius**: Rust. Built by Setup.ps1; build balius before building the solution (e.g. `cargo build --release` in balius/). Invoked per project: balius.exe <EngineDir> <ProjectName> <SourceRoot> <GitDir> <ConfName>. Detects header changes; passes headers to header-parser via target file. **FastBuild PATH (three-path, append only)**: BFF PATH = SDK (Sharpmake) ; Rust (Utils.GetRustPathForFastBuild) ; VC (Utils.GetVCToolsPathForFastBuild). CommonProject sets AdditionalGlobalEnvironmentVariables["PATH"] and ["LIB"]; MasterBff appends PATH and emits LIB (prepends Windows SDK Lib via GetWindowsSdkLibPathForBff when needed so link.exe finds ntdll.lib).
- **Header parser**: C++/CMake under Programs/header-parser. Reads target file (from balius); generates .generated.h and .generated.cpp using file macros. Output: Intermediate/HeaderParser/<ProjectName>/HeaderGenerated. To reduce thread count (e.g. avoid system hiccups), set **HEADER_PARSER_MAX_JOBS=1** in the environment or pass **--max-jobs 1**; then each instance runs sequentially instead of parallel over the file list.

## Technical Constraints
- Core must not depend on other runtime modules; use Abstracts::Resource* in Core APIs; implementations cast.
- Generated headers included after class definition so type is complete for statics.
- IModule base in generated polymorphic_type_hash emitted as Engine::IModule.
- Resource type constraint in .generated.cpp (static_assert), not is_base_of in header.

## Include path rule
- If a project **depends on another project** (build.cs), the dependency’s Public folder is in the include path—**use short includes** (e.g. `"IRaytracingExtension.h"`), not full path. Add the dependency in build.cs when you need headers from another module; then regenerate the solution.

## Dependencies
- Engine: Core, Memory, Boost, TBB, many runtime modules. Client: engine modules. Build: .NET (Sharpmake), CMake, vcpkg, VS.
