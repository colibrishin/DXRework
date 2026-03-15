# Header-parser: location and role

## Location
- **Source:** `Programs/header-parser/`
- **Build:** CMake (e.g. VS 2022/18, x64) produces `header-parser.vcxproj`; Sharpmake references it when present. Exe: `Programs/header-parser/Release/header-parser.exe`.
- **References:** Build/ThirdPartyPrograms (HeaderParserProject), Build/CommonProject.build.cs (prebuild dependency), balius (spawns the exe).

## Role
1. **Input:** Invoked by **balius** with a **target file** (first CLI arg): a list of header paths (one per line, relative to `Intermediate/HeaderParser/<ProjectName>`). Also reads `<firstDir>/<Conf>.dep` for module dependencies.
2. **Parse:** For each header path, reads the file, runs a C++-oriented **parser** (tokenizer + handler) that recognizes macros (`-c` class, `-e` enum, `-p` property, `-f` function, `-m` custom e.g. GENERATE_BODY). Outputs a **JSON** description of classes, namespaces, members, and meta (ECLASS/ECLASS(component=client), etc.). Does not expand includes or full macro substitution; assumes compilable code.[^1]
3. **Generate:** Uses **bodygeneration_macro.h** and **dependency_template.h** to turn that JSON into:
   - **`.generated.h`:** `GENERATE_BODY` / `GENERATE_BODY_HEAD`, `polymorphic_type_hash<Type>`, static type names/hash, clone declarations, and for ECLASS(resource) the getter/create templates and BOOST serialization declarations.
   - **`.generated.cpp`:** Definitions (e.g. `StaticIsDerivedOf`, GetTypeName/GetTypeHash/IsDerivedOf/GetAllocationContext), clone impl, BOOST export/archive, and for resources a `static_assert(std::is_base_of_v<Resource, Type>)`.
4. **Output layout:** Writes under `HeaderGenerated/<firstDir>/` (e.g. `HeaderGenerated/Public/Foo.generated.h` and `Foo.generated.cpp`). Paths are derived from the source path via `GetDestinationPath` in main.cc.

## Integration in this project
- **Balius** (prebuild per project): hashes headers; if changed, copies headers to `Intermediate/HeaderParser/<ProjectName>`, writes the target file of changed headers, then runs **header-parser.exe** with that file and `-b <configuration>`.
- **CommonProject:** Adds `Intermediate/HeaderParser/<Name>/HeaderGenerated/Public` and `.../Private` to include paths so compiled code finds `#include "X.generated.h"`.
- **Monolith:** Adds the same HeaderGenerated paths for **every** dependency project so the unity build sees the same generated headers.

## References
- Original: [header-parser (baszalmstra)](https://github.com/baszalmstra/header-parser) — extracts RTTI from annotated C++ into JSON for code generation.
- This repo: `Programs/header-parser/` (fork/customization with bodygeneration_macro.h, ECLASS/ECLASS(component=client), resource/component/object tracking).
- Pipeline: memory-bank/tasks.md § Implementation plan (prebuild, header-parser, FastBuild).

[^1]: README: "assumes the passed in code will compile … does not perform name lookups … doesn't substitute macros or includes."
