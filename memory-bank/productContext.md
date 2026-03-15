# Product Context

## Why This Project Exists
Custom engine and client for control over rendering (D3D12), modules, and memory/type systems. Enables game features (render tasks, physics, sound) on a shared engine core.

## Problems It Solves
- **Module loading**: IModule + ModuleManager for init/shutdown and dependencies.
- **Reflection without RTTI**: type_hash / polymorphic_type_hash (CoreType.h) for serialization and safe_cast.
- **Generated code**: header-parser produces GENERATE_BODY, type_hash specializations, and overridable defs (header vs .generated.cpp).
- **Allocation**: Central allocator registry and pools with exception-safe paths.

## How It Should Work
- Build: Setup (Sharpmake, balius, header-parser, vcpkg) → GenerateSolution.bat → open solution. Per-project prebuild runs balius; header-parser fills Intermediate/HeaderParser/<ProjectName>/HeaderGenerated.
- Source headers include .generated.h after the class definition. Modules register and initialize in order.

## User Experience Goals
- Developers: Clear build steps, generated files in a known location, minimal boilerplate for ECLASS/GENERATE_BODY.
- Runtime: Stable module lifecycle and allocation.
