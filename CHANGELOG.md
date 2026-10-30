# Changelog

## [Unreleased]

Logic fixes from a line-by-line comparison with upstream Perfare/Il2CppDumper.

### Search / initialization
- **Pattern search and symbol search now initialize the engine** — both reported success without ever calling `init()`, so dumps ran on empty type tables
- **Pattern search no longer discards its result** — it re-ran PlusSearch instead of using the registrations it found; the ARM LDR check also tested the wrong bit
- **Dump files work** — the dump address never reached the ELF parser (no segment/dynamic fix-up, wrong RVAs); it is now applied before search and honors *No Redirected Pointer*
- **Force Il2Cpp Version no longer corrupts metadata** — it overwrote the metadata parser's version after parsing; it now only applies to the binary, like upstream
- **Version 27.1 → 27.2 detection implemented** (was a stub); RGCTX entries are read with the correct pointer-sized layout for 27.2+ and on 32-bit
- **Generic method pointers keyed correctly** — the XOR hash collided once method indices exceeded 65535, attaching wrong addresses to generic instances
- **mapVATR throws for unmapped addresses** (as upstream) instead of silently reading file offset 0

### ELF parsing
- **DT_GNU_HASH parsing fixed** — wrong bloom word size on 64-bit and bucket count read from the wrong offset, losing the symbol table on most arm64 binaries
- **R_AARCH64_ABS64 / R_X86_64_64 relocations applied**; 32-bit relocation loop no longer drifts after each write
- **Protection check** now also detects `JNI_OnLoad` and `SHT_LOUSER` sections
- Wildcard Boyer-Moore search uses the correct skip table

### dump.cs
- **Field and parameter default values** (constants, enum values) are printed — previously a stub that always failed
- **v29+ custom attributes decoded** — the reader parsed the wrong blob layout and printed `[CustomAttribute]` placeholders
- `out` / `ref` / `in` / `[In]` / `[Out]` parameter modifiers, setter-only properties, `[Serializable]`, and `GenericInstMethod` blocks
- Custom attribute lookup used `assemblyIndex` instead of the image index
- Output format (spacing, uppercase hex, no BOM) matches upstream; a broken type no longer leaves a half-written declaration

### script.json / il2cpp.h / stringliteral.json
- `struct_generator.cpp` re-ported from upstream: generic class structs, `_array` structs, generic-parameter/pointer types, `MethodInfo_*` structs with RGCTX, type signatures, metadata usages for v27+ (previously none) and FieldInfo/MethodAddress entries for v17–26
- `header_constants.h` restored from upstream (was a placeholder); the il2cpp.h header is chosen with upstream's exact version list
- JSON output matches System.Text.Json (indentation, escaping, `null` signatures); stringliteral.json no longer lists every literal at `0x0`

### Robustness
- Array reads reject impossible counts instead of attempting multi-GB allocations; metadata/type indices are bounds-checked so corrupt input fails with an error instead of crashing
- Native calls are serialized so cleanup cannot free state under a running dump
- Dump addresses ≥ 0x8000000000000000 are parsed correctly; the job history records the detected Il2Cpp version

## [1.1.0] - 2026-06-04

### Critical Fixes
- **Fixed use-after-free in native dump** — JNI string was released before being used, causing undefined behavior/crashes
- **Fixed metadata usage processing** — `processingMetadataUsage()` was empty, breaking struct generation and string literal resolution for IL2CPP versions 16–26
- **Fixed 32-bit ELF pointer reading** — pointer arrays were always read as 64-bit, producing garbage output for 32-bit (armeabi-v7a) binaries
- **Fixed genericInsts reading** — structs were read sequentially instead of resolving each pointer via mapVATR

### Bug Fixes
- **Force Il2Cpp Version now works** — added `nativeSetConfig` JNI call applied before search; previously the config option had no effect
- **Fixed method modifier cache** — was keyed by `nameIndex` causing wrong modifiers for methods sharing the same name (e.g., all `.ctor` methods)
- **Fixed elfParser memory leak** — added proper destructor to `ElfIl2Cpp`
- **Fixed hex address input** — now accepts `0x` prefix (strips it before parsing)
- **Force Dump mode wired up** — config option now sets `isDumped=true` in native engine
- **No Redirected Pointer exposed in UI** — full end-to-end wiring from config panel to native

### il2cpp.h Generation
- **Full struct output matching original** — now generates `_Fields` (with parent inheritance), `_c` (class metadata with VTable/RGCTX/StaticFields), `_o` (object instance with typed klass pointer), `_StaticFields`, `_VTable`, `_RGCTXs`
- **Implemented `addParents`** — resolves parent type inheritance chain
- **Implemented `addVTableMethod`** — populates virtual method dispatch table from metadata
- **Implemented `recursionStructInfo`** — properly resolves struct dependencies in declaration order
- **Full method signatures** — includes `__this` parameter with correct struct type and all method parameters

### stringliteral.json
- **Actual RVA addresses** — resolves real addresses from metadata usages (v16–26) instead of always outputting `0x0`
- **Proper JSON escaping** — handles `\b`, `\f`, and `\u00xx` for control characters
- **Correct key casing** — uses `"value"` / `"address"` matching original format

### Performance
- **O(n²) → O(1) struct name lookup** — `getIl2CppStructName` was doing linear scan through all typeDefs per field; now uses hash map
- **Eliminated string concatenation in struct output** — `recursionStructInfo` writes directly to output stream instead of building/copying massive intermediate strings
- **64KB I/O buffering** — all output files (`dump.cs`, `script.json`, `stringliteral.json`, `il2cpp.h`) use buffered writes
- **Pre-allocated vectors** — `scriptMethod` and `structInfoList` reserved to avoid reallocations
- **Pointer-based struct lookup** — `structInfoWithStructName` stores pointers instead of copying entire StructInfo objects

### Code Quality
- **Eliminated duplicated metadata constructor** — extracted shared init logic into `Metadata::initialize()`, removing ~130 lines of duplicated code
- **Added `readPointerArray` helper** — BinaryStream method that correctly reads uint32/uint64 based on `is32Bit`
- **Temp file cleanup** — cached input files are deleted after dump completes (success or failure)

### UI/UX
- **Rich log output** — native engine now reports metadata stats, search strategy results, version detection, and per-phase progress to the app log viewer
- **File size display** — shows file sizes on load
- **Config panel** — added "No Redirected Pointer" toggle

## [1.0.0] - Initial Release

- On-device IL2CPP dumping with Kotlin/Jetpack Compose UI
- C++20 native core with ELF parsing
- Multiple search strategies (PlusSearch, pattern, symbol)
- Job history with Room database
- Export/share via ZIP or individual files
- Material 3 dynamic theming
