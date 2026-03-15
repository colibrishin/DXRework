# Project Brief: DXRework

## Purpose
C++ game/engine codebase: custom engine, D3D12 graphics, modular runtime (IModule, Resource, Component, Object).

## Core Goals
- **Engine runtime**: Modules (Core, Memory, RenderPipeline, PhysicsManager, etc.), render pipeline, physics, sound, core abstractions (Resource, Component, Object, Singleton).
- **Build & tooling**: Sharpmake (solution/vcxproj + FastBuild BFF); header-parser (.generated.h / .generated.cpp); balius (Rust, header copy + git diff + invokes header-parser).
- **Quality**: RTTI-free type hashing (CoreType.h), exception-safe allocators, Core module independence (no Core → other runtime modules).

## Scope
- **In scope**: Engine (Engine/), Client, Build (Sharpmake), Programs (header-parser), balius. vcpkg; Windows SDK / VS.
- **Out of scope**: Full product spec—this brief is the technical foundation for the memory bank.

## Source of Truth
This file is the foundation for productContext, systemPatterns, techContext, activeContext, and progress. Keep it short; expand in other memory-bank files.
