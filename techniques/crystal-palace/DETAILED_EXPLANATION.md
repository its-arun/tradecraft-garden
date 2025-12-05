# Crystal Palace - Comprehensive Technical Explanation

## Table of Contents
1. [Introduction](#introduction)
2. [Architecture](#architecture)
3. [Linker Script Language](#linker-script-language)
4. [Binary Transformation Framework](#binary-transformation-framework)
5. [Resource Management](#resource-management)
6. [Dynamic Function Resolution](#dynamic-function-resolution)
7. [PIC Support Features](#pic-support-features)
8. [COFF Operations](#coff-operations)
9. [Aspect-Oriented Programming](#aspect-oriented-programming)
10. [Shared Libraries](#shared-libraries)
11. [Advanced Techniques](#advanced-techniques)

---

## Introduction

Crystal Palace is a specialized linker designed by Raphael Mudge to solve fundamental challenges in building position-independent capability loaders for offensive security operations. Unlike traditional linkers (ld, link.exe), Crystal Palace is purpose-built for red team tradecraft.

### The Problem Space

Traditional approaches to in-memory evasion face several challenges:

1. **Multi-Resource Packaging**: How do you package a loader with embedded payloads, configurations, and resources while maintaining position independence?

2. **Global Variable Access**: Position-independent code traditionally can't use global variables (especially .bss uninitialized variables) without complex workarounds.

3. **API Resolution**: Loaders must resolve Windows APIs without import tables, requiring custom resolution mechanisms.

4. **Signature Evasion**: Static signatures can detect common loader patterns; code must be polymorphic.

5. **Development Ergonomics**: Building PIC loaders with traditional toolchains involves extensive manual work, #ifdef preprocessor hacks, and fragile build processes.

Crystal Palace addresses all of these through binary transformation, intelligent linking, and a domain-specific language.

### Design Philosophy

Crystal Palace embodies the "leverage" principle - giving individual researchers the productivity of entire teams through intelligent automation. Instead of manual offset calculations, symbol manipulation, and custom build scripts, researchers write clean C code and let Crystal Palace handle the complexity.

---

## Architecture

### Stack-Based Composition Model

Crystal Palace uses a stack-based architecture for assembling capabilities:

```
┌─────────────────┐
│  Specification  │  ← Input: Linker script
│     Parser      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Stack Engine   │  ← Executes commands, maintains stack
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Binary        │  ← Disassemble, transform, reassemble
│  Transformer    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Resource      │  ← Append resources, link symbols
│    Linker       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    Output       │  ← Final capability (PIC/PICO/DLL/COFF)
│  Generator      │
└─────────────────┘
```

### Processing Flow

1. **Parse Specification**: Read linker script, validate syntax
2. **Load Objects**: Load COFF files, DLLs, binaries onto stack
3. **Merge/Combine**: Merge multiple objects using COFF normalization
4. **Transform**: Apply binary transformations (+optimize, +mutate, +disco)
5. **Fix PIC Issues**: Apply DFR, fixptrs, fixbss as needed
6. **Link Resources**: Append resources and create symbol linkages
7. **Generate Output**: Create final PIC/PICO/DLL/COFF with appended data

### Internal Representation

Crystal Palace maintains an internal normalized COFF representation:

- **Symbol Table**: All functions, variables, imports, exports
- **Section Table**: .text (code), .data (initialized data), .rdata (read-only), .bss (uninitialized)
- **Relocation Table**: References that need fixing during linking
- **Instruction Stream**: Disassembled x86/x64 instructions for transformation

This normalization allows uniform transformation regardless of input format.

---

## Linker Script Language

### Specification File Structure

Crystal Palace specification files use a simple, indentation-based syntax:

```
# Comments start with #

# Architecture-specific blocks
x64:
  command "args"
    subcommand "args"
    subcommand "args"

x86:
  command "args"
    subcommand "args"
```

### Core Commands

#### load

Load a file onto the stack:

```
load "bin/capability.x64.o"
load "bin/loader.dll"
load "shellcode.bin"
```

Supports:
- COFF object files (.o, .obj)
- DLL files (.dll)
- Raw binaries (.bin, .raw, etc.)

#### make

Generate output format:

```
make pic          # x86 position-independent code
make pic64        # x64 position-independent code
make coff         # Normalized COFF export
make pico         # Position-independent capability object
make dll          # DLL format (if applicable)
```

The `make` command can be followed by `+options` for transformations:

```
make pic +optimize +mutate +disco
```

#### link

Create the final output file:

```
link "output.x64.bin"
```

This command pops the top of the stack and writes it to disk.

#### merge

Merge the top stack item into the item below it:

```
load "module1.x64.o"
  merge
load "module2.x64.o"
  merge
# Stack now contains module1+module2 combined
```

#### append

Append the top stack item to the previous item:

```
load "loader.x64.o"
  make pic
load "payload.bin"
  append $PIC      # Append to PIC
```

Append targets:
- `$PIC` / `$PIC64` - Append to PIC/PIC64
- `$PICO` - Append to PICO
- `$DLL` - Append to DLL
- `$OBJECT` - Append to current object

### Indentation and Flow Control

Indented commands apply to the previously loaded item:

```
load "file.o"
  dfr "resolve" "ror13"    # Applies to file.o
  fixptrs "_caller"         # Applies to file.o
  make pic                  # Applies to file.o
```

This creates a natural flow for composing transformations.

---

## Binary Transformation Framework

Crystal Palace's killer feature is its ability to disassemble programs, modify them at the instruction level, and reassemble them. This enables transformations impossible with source code alone.

### +optimize: Link-Time Optimization

Removes unused functions and data:

```
make pic +optimize
```

**How it works:**

1. **Call Graph Construction**: Starting from entry point `go()`, build a graph of all function calls
2. **Reachability Analysis**: Mark all functions reachable from entry point
3. **Dead Code Elimination**: Remove unreachable functions and their data
4. **Section Compaction**: Remove unused .data/.rdata/.bss sections

**Use case**: When using shared libraries, only include actually-used functionality.

**Example**: Shared library has 50 functions, but loader only uses 5. Without optimization: 500KB. With optimization: 50KB.

### +disco: Function Randomization

Randomizes function order within sections:

```
make pic +disco
```

**How it works:**

1. **Function Boundary Detection**: Identify start/end of each function in .text
2. **Entry Point Preservation**: Keep first function in place (often entry point)
3. **Randomization**: Shuffle remaining functions randomly
4. **Relocation Update**: Fix all call instructions and relocations

**Use case**: Break up known function ordering patterns that signatures rely on.

**Security note**: Combined with +mutate, creates unique binary instances.

### +mutate: Code Mutation

Mutates code for signature evasion:

```
make pic +mutate
```

**How it works:**

1. **Constant Breaking**: Replace `mov eax, 0x12345678` with equivalent multi-instruction sequences
2. **Stack Strings**: Identify string constants, push them to stack byte-by-byte at runtime
3. **Noise Injection**: Insert benign instructions that don't affect logic (nops, redundant moves, etc.)
4. **Instruction Substitution**: Replace instructions with functional equivalents

**Example transformations:**

```assembly
; Before mutation
mov eax, 0x1234
call SomeFunc

; After mutation
xor eax, eax
add eax, 0x1000
add eax, 0x234
push ebx
pop ebx           ; noise
call SomeFunc
```

**Important**: This is NOT obfuscation. Goal is "content signature resilience" - breaking up known byte patterns while maintaining readability.

### +gofirst: Entry Point Positioning

Moves the `go()` entry point to the first function:

```
make pic +gofirst
```

**How it works:**

1. **Locate go() Symbol**: Find the entry point function
2. **Move to Position 0**: Reorder .text section to place go() first
3. **Update Relocations**: Fix all internal references

**Use case**: Align with PICO convention where entry point must be at offset 0.

**Alternative to**: Wrapping go() in a trampoline function just for positioning.

### Transformation Chaining

Transformations are applied in a specific order:

```
make pic +optimize +disco +mutate +gofirst
```

Processing order:
1. Normalize COFF representation
2. Apply DFR, fixptrs, fixbss (if specified)
3. +optimize (must happen before disco to avoid dead code shuffling)
4. +disco (randomize before mutation for better entropy)
5. +mutate (mutate the randomized layout)
6. +gofirst (final positioning)
7. Generate output format

---

## Resource Management

### Appending Resources

Crystal Palace allows appending arbitrary data to capabilities:

```
load "loader.x64.o"
  make pic
load "shellcode.bin"
  append $PIC
load "config.dat"
  append $PIC
```

Result:
```
[PIC Loader Code]
[Shellcode Data]
[Config Data]
```

### Symbol-Based Resource Access

Crystal Palace creates symbols for appended resources:

```c
// In your loader C code
extern unsigned char _binary_shellcode_bin_start[];
extern unsigned char _binary_shellcode_bin_end[];
extern unsigned int  _binary_shellcode_bin_size;

void execute_payload(void) {
    // Access the appended shellcode
    void (*payload)() = (void(*)())_binary_shellcode_bin_start;
    payload();
}
```

**Symbol naming convention**:
- `_binary_<filename>_<ext>_start` - Pointer to start
- `_binary_<filename>_<ext>_end` - Pointer to end
- `_binary_<filename>_<ext>_size` - Size in bytes

### Multiple Resource Pattern

Common pattern for multi-stage loaders:

```
# Specification
load "stage1.x64.o"
  make pic +mutate
load "stage2.bin"
  append $PIC
load "stage3.bin"
  append $PIC
load "config.bin"
  append $PIC
link "final.bin"
```

```c
// In stage1.x64.c
extern unsigned char _binary_stage2_bin_start[];
extern unsigned int  _binary_stage2_bin_size;

extern unsigned char _binary_stage3_bin_start[];
extern unsigned int  _binary_stage3_bin_size;

extern unsigned char _binary_config_bin_start[];
extern unsigned int  _binary_config_bin_size;

void go(void) {
    // Parse config
    parse_config(_binary_config_bin_start, _binary_config_bin_size);

    // Execute stage 2
    execute(_binary_stage2_bin_start);

    // Execute stage 3
    execute(_binary_stage3_bin_start);
}
```

### Resource Overlay Pattern

Resources can overlay each other for decryption/decompression:

```c
extern unsigned char _binary_encrypted_bin_start[];
extern unsigned int  _binary_encrypted_bin_size;

void go(void) {
    // Decrypt in-place
    decrypt(_binary_encrypted_bin_start, _binary_encrypted_bin_size);

    // Now execute the decrypted payload at same location
    void (*payload)() = (void(*)())_binary_encrypted_bin_start;
    payload();
}
```

---

## Dynamic Function Resolution

### The DFR Problem

Position-independent code can't use import tables - they contain absolute addresses. Crystal Palace solves this with Dynamic Function Resolution.

### DFR Declaration

```
dfr "resolver_function" "method"
```

**Methods:**
- `ror13` - Hash-based resolution (uses ROR13 hashing algorithm)
- `strings` - String-based resolution (passes string pointers)

### How DFR Works

Crystal Palace:

1. **Scans Code**: Identifies all `__imp_MODULE$Function` references
2. **Generates Resolver Calls**: Inserts call to resolver before each API use
3. **Passes Arguments**: Depending on method, passes hashes or strings
4. **Patches Instructions**: Replaces import references with resolver results

### ror13 Method

```
dfr "resolve" "ror13"
```

Crystal Palace converts:
```c
KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
```

Into equivalent of:
```c
void* (*pVirtualAlloc)() = resolve(0x91AFCA54, 0x3F9287AE);
pVirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
```

Where:
- `0x91AFCA54` = ROR13 hash of "KERNEL32"
- `0x3F9287AE` = ROR13 hash of "VirtualAlloc"

**Your resolver implementation:**
```c
void* resolve(DWORD moduleHash, DWORD functionHash) {
    // Walk PEB to find loaded modules
    // Compare ROR13 hashes
    // Walk EAT to find function
    // Compare ROR13 hashes
    // Return function pointer
}
```

### strings Method

```
dfr "resolve" "strings"
```

Crystal Palace converts:
```c
KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
```

Into equivalent of:
```c
void* (*pVirtualAlloc)() = resolve("KERNEL32", "VirtualAlloc");
pVirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
```

**Your resolver implementation:**
```c
void* resolve(char* module, char* function) {
    HANDLE hModule = LoadLibraryA(module);
    return GetProcAddress(hModule, function);
}
```

### Multi-Resolver Pattern

For advanced scenarios, declare multiple resolvers:

```
dfr "resolve" "ror13" "KERNEL32, NTDLL"
dfr "resolve_ext" "strings"
```

**How it works:**
- First resolver handles KERNEL32 and NTDLL APIs (hash-based, fast)
- Second resolver handles all other modules (string-based, can load new DLLs)

**Why this matters**: The `resolve_ext` function can safely use KERNEL32/NTDLL APIs (like LoadLibraryA, GetProcAddress) because those are resolved by the first resolver:

```c
void* resolve(DWORD moduleHash, DWORD functionHash) {
    // Basic resolver for KERNEL32/NTDLL only
    // Walk PEB, find module, find function
    return function_pointer;
}

void* resolve_ext(char* module, char* function) {
    // Can use APIs because KERNEL32/NTDLL already resolved
    HANDLE hModule = KERNEL32$GetModuleHandleA(module);
    if (hModule == NULL)
        hModule = KERNEL32$LoadLibraryA(module);
    return (void*)KERNEL32$GetProcAddress(hModule, function);
}
```

### DFR and Code Mutation

When combining DFR with +mutate:

```
dfr "resolve" "ror13"
make pic +mutate
```

Crystal Palace mutates the hash values and resolver calls, creating unique instances while maintaining functionality.

---

## PIC Support Features

### fixptrs: x86 Direct Addressing Fix

**The Problem**: x86 doesn't have RIP-relative addressing like x64. Accessing data requires knowing absolute addresses:

```c
// This works fine in x64 PIC
const char* str = "Hello";

// In x86 PIC, this generates:
mov eax, offset str    ; Absolute address - breaks PIC!
```

**The Solution**: fixptrs transforms the code to calculate addresses dynamically:

```
fixptrs "_caller"
```

**Requirements**: You must provide a caller function that returns its own return address:

```c
void* _caller(void) {
    return __builtin_return_address(0);
}
```

**How it works:**

1. Crystal Palace disassembles x86 PIC code
2. Identifies instructions referencing .data/.rdata sections
3. Calculates offset from instruction to data
4. Rewrites instruction to:
   - Call _caller() to get current EIP
   - Add offset to get data address
5. Replaces direct reference with computed address

**Before transformation:**
```assembly
mov eax, offset str    ; Direct reference
```

**After transformation:**
```assembly
call _caller
add eax, offset_to_str  ; EIP + offset = absolute address
mov eax, [eax]
```

**Impact**: x86 PIC can now access string constants and read-only data without manual offset calculations or preprocessor hacks.

### fixbss: Global Variables in PIC

**The Problem**: Position-independent code traditionally can't use uninitialized global variables (.bss section) because they need a writable memory location at a known offset.

```c
// This breaks traditional PIC
int global_counter = 0;  // .bss section

void increment(void) {
    global_counter++;  // Where is this in memory?
}
```

**The Solution**: fixbss provides stable read/write memory through binary transformation:

```
fixbss "_bss_fix"
```

**Requirements**: You must provide a fixing function that returns a read/write memory pointer:

```c
// Returns pointer to read/write memory region
void* _bss_fix(void) {
    // Strategy: Find slack space in loaded DLL sections
    // Or: Allocate VirtualAlloc'd memory
    // Or: Use TLS slots
    // Must return same address on subsequent calls!
    return get_stable_rw_memory();
}
```

**How it works:**

1. Crystal Palace identifies all .bss references in the code
2. Injects calls to _bss_fix() before first use
3. Rewrites instructions to use returned pointer + offset
4. Ensures all references to same variable use same offset

**Common Implementation** (from Simple PIC example):

```c
void* _bss_fix(void) {
    static void* bss_base = NULL;

    if (bss_base != NULL)
        return bss_base;

    // Find slack space in NTDLL or KERNEL32
    // This is the padding between actual section size
    // and virtual size (page-aligned)

    HMODULE hNtdll = get_ntdll();
    bss_base = find_slack_space(hNtdll);

    return bss_base;
}

void* find_slack_space(HMODULE hModule) {
    // Walk PE sections
    // Find one with SizeOfRawData < VirtualSize
    // Return pointer to: base + SizeOfRawData
    // This is already RW memory, zero-initialized!
}
```

**Constraints**:
- fixbss only transforms common instruction forms (mov, lea, etc.)
- Aggressive compiler optimizations may generate exotic instructions that Crystal Palace can't transform
- Size of .bss must fit in available slack space (or your allocation strategy)

**Security Benefits**:
- No need to allocate new RW memory (VirtualAlloc is loud)
- Reuses existing RW pages in loaded DLLs
- Maintains position independence

### Symbol Remapping

The `remap` command renames symbols in the COFF symbol table:

```
remap "old_symbol" "new_symbol"
```

**Use Case 1: API Convention Conversion**

Different resolvers expect different symbol formats:

```
# Convert standard imports to MODULE$Function format
remap "__imp_VirtualAlloc" "__imp_KERNEL32$VirtualAlloc"
remap "__imp_GetProcAddress" "__imp_KERNEL32$GetProcAddress"
```

**Use Case 2: Dual-Purpose Loaders**

Build one codebase that supports multiple execution modes:

```c
// In your code, have multiple entry points
void go_dll(void) {
    // DLL-specific initialization
    execute_capability();
}

void go_object(void) {
    // COFF-specific initialization
    execute_capability();
}
```

In spec file:
```
# For DLL target
x64:
  remap "go_dll" "go"
  load "loader.x64.o"
    make pic +optimize
  link "loader.dll"

# For COFF target
x64:
  remap "go_object" "go"
  load "loader.x64.o"
    make coff +optimize
  link "loader.x64.o"
```

Link-time optimization (+optimize) will eliminate the unused entry point's code automatically.

---

## COFF Operations

### COFF Normalization

Crystal Palace internally normalizes COFF files:

**What it does:**
- Collapses multiple related sections (e.g., .text$mn, .text$x, etc. → .text)
- Standardizes symbol table format
- Merges relocation tables
- Resolves internal linkages

**Why it matters:**
- Enables uniform transformation regardless of compiler
- Allows merging COFFs from different toolchains
- Prepares files for binary transformation

**Automatically applied** when you load a COFF file.

### COFF Merge

Combine multiple COFF files into one:

```
load "module1.x64.o"
  merge
load "module2.x64.o"
  merge
load "module3.x64.o"
  merge
  make coff
```

**How it works:**

1. **Symbol Table Merge**: Combine all symbols, resolve conflicts
2. **Section Merge**: Combine .text, .data, .rdata, .bss sections
3. **Relocation Merge**: Update relocations with new offsets
4. **Reference Resolution**: Link cross-module function calls

**Result**: Single COFF containing all modules' functionality.

**Use Case**: Build modular capabilities, assemble at link-time:

```
core.o     - Core capability logic
http.o     - HTTP communication
crypto.o   - Encryption functions
utils.o    - Helper utilities

→ merge all → capability.o
```

### COFF Export

Create normalized COFF output:

```
make coff
```

**Output format:**
- Standard COFF object file
- Normalized section layout
- Clean symbol table
- Ready for PICO runner or further linking

**Why export COFF?**
- Smaller than DLL (no PE overhead)
- Compatible with PICO convention
- Can be loaded by lightweight loaders
- Easier to mutate and transform

### ./piclink Command

Special command for PIC-only projects:

```bash
./piclink capability.spec.txt
```

Enables position-independent code projects to use Crystal Palace's mutation, optimization, and resource linking WITHOUT requiring a target capability.

**Use case**: You have shellcode/PIC but want to:
- Apply +mutate for signature evasion
- Use +optimize to remove unused code
- Link resources via symbols

---

## Aspect-Oriented Programming

Crystal Palace implements AOP principles for separating tradecraft from capability logic.

### AOP Terminology

- **Join Point**: A point in program execution where instrumentation can occur (e.g., API call, function call)
- **Advice**: The code that implements cross-cutting functionality (e.g., logging, encryption, anti-analysis)
- **Pointcut**: The specification of which Join Points receive which Advice
- **Weaving**: The process of adding Aspects to the program (Crystal Palace does this via binary transformation)

### attach: Hooking Win32 APIs

```
attach "MODULE$Function" "_HookFunction"
```

**How it works:**

1. Crystal Palace disassembles the program
2. Finds all references to MODULE$Function
3. Rewrites them to call _HookFunction instead
4. Excludes hooks' own internal calls (prevents recursion)

**Example:**

```
attach "KERNEL32$CreateFileA" "_LogCreateFile"
```

```c
// Your hook implementation
HANDLE WINAPI _LogCreateFile(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
) {
    // Log the file access
    log("CreateFile: %s", lpFileName);

    // Call the real API
    return KERNEL32$CreateFileA(
        lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile
    );
}
```

**Hook Stacking**: Multiple hooks stack in first-attached-first-executed order:

```
attach "KERNEL32$VirtualAlloc" "_Hook1"
attach "KERNEL32$VirtualAlloc" "_Hook2"
attach "KERNEL32$VirtualAlloc" "_Hook3"
```

Execution order: Capability → Hook1 → Hook2 → Hook3 → Real API

This enables Chain of Responsibility pattern.

### redirect: Hooking Local Functions

```
redirect "function_name" "_hook_function"
```

Same as attach, but for local functions instead of Win32 APIs.

**Example:**

```
redirect "parse_command" "_audit_command"
```

```c
void parse_command(char* cmd) {
    // Original implementation
}

void _audit_command(char* cmd) {
    // Audit the command
    log_command(cmd);

    // Call original (need to handle this carefully)
    parse_command(cmd);  // This would recurse!
}
```

**Note**: Careful with recursion. Often better to use AOP for observation rather than replacement of local functions.

### Hook Protection Mechanisms

#### protect: Isolate Functions from Hooks

```
protect "function1, function2, function3"
```

Prevents ALL hooks from applying within the listed functions.

**Use case**: Performance-critical code or functions that must call real APIs directly:

```c
void performance_critical_loop(void) {
    // Don't want hook overhead here
    for (int i = 0; i < 1000000; i++) {
        KERNEL32$VirtualAlloc(...);
    }
}
```

```
protect "performance_critical_loop"
```

#### preserve: Protect Specific Targets

```
preserve "target|MODULE$Function" "function1, function2"
```

Within function1 and function2, calls to MODULE$Function won't be hooked.

**Use case**: Hook uses the API being hooked:

```c
HANDLE _LogCreateFile(...) {
    // This call should NOT be hooked (infinite recursion!)
    return KERNEL32$CreateFileA(...);
}
```

```
attach "KERNEL32$CreateFileA" "_LogCreateFile"
preserve "KERNEL32$CreateFileA" "_LogCreateFile"
```

#### optout: Exclude Specific Hooks

```
optout "function" "hook1, hook2"
```

Within function, hook1 and hook2 don't apply (but other hooks do).

**Use case**: Fine-grained control over hook application.

### IAT Hooking

For capabilities with import tables:

```
addhook "MODULE$Function" "hook_function"
```

**Difference from attach**:
- `attach` rewrites call sites in code
- `addhook` patches Import Address Table

**Runtime Resolution**:

```c
FARPROC __resolve_hook(DWORD functionHash);
```

Linker intrinsic that converts ROR13 hash to hook pointer.

**Hook Filtering**:

```
filterhooks $DLL
```

Removes hooks not needed for the current capability, reducing size.

### AOP Benefits for Red Teams

1. **Instrumentation Without Code Changes**: Add logging, encryption, anti-analysis without modifying capability code

2. **Modular Tradecraft**: Separate evasion logic from capability logic

3. **Reusable Hooks**: Build library of hooks (encrypt network traffic, evade EDR, etc.) and apply to any capability

4. **Rapid Iteration**: Test different evasion approaches by swapping hooks without recompiling capabilities

**Example - Anti-Debugging Hook**:

```
attach "KERNEL32$IsDebuggerPresent" "_FakeDebugCheck"
```

```c
BOOL WINAPI _FakeDebugCheck(void) {
    // Always return FALSE, even if debugger attached
    return FALSE;
}
```

---

## Shared Libraries

### LibTCG: Tradecraft Garden Library

Crystal Palace supports ZIP-based shared libraries. LibTCG is the standard library for Tradecraft Garden.

**What it provides:**
- DLL parsing utilities
- PICO execution functions
- EAT (Export Address Table) resolution
- Printf-style debugging
- Common helpers

**Including LibTCG**:

```c
#include "tcg.h"
```

Specification file:
```
mergelib "lib/libtcg.x64.zip"
```

### Shared Library Format

ZIP archive containing:
- `*.x86.o` - x86 compiled objects
- `*.x64.o` - x64 compiled objects
- `tcg.h` - Header file for declarations

### Shared Library Conventions

**Critical Constraint**: Shared library code MUST avoid read/write global variables for PIC compatibility.

**Good**:
```c
const char* get_error_string(int code) {
    const char* messages[] = {  // Read-only data
        "Success",
        "Failure"
    };
    return messages[code];
}
```

**Bad**:
```c
int error_count = 0;  // Read/write global - breaks PIC!

void log_error(void) {
    error_count++;  // Not PIC compatible
}
```

**Workaround for State**: Pass pointers to state:

```c
// Library function
void increment_counter(int* counter) {
    (*counter)++;
}

// User code with fixbss
int global_counter = 0;  // OK because fixbss handles it

void use_library(void) {
    increment_counter(&global_counter);
}
```

### Link-Time Optimization with Libraries

```
load "capability.x64.o"
  mergelib "libtcg.x64.zip"
  make pic +optimize
```

Without +optimize: Entire library included (500KB+)
With +optimize: Only used functions (50KB)

**How it works:**
1. Start from capability's go() entry point
2. Build call graph of all reachable functions
3. Eliminate unreachable library functions
4. Remove unused data sections

### Function Disco with Libraries

```
make pic +optimize +disco
```

Interweaves library code with capability code:

**Without disco**:
```
[All capability functions]
[All library functions]
```

**With disco**:
```
[cap_func1]
[lib_func3]
[cap_func2]
[lib_func1]
[cap_func3]
[lib_func8]
```

Breaks up predictable library patterns for signature evasion.

---

## Advanced Techniques

### Dual-Architecture Builds

Single specification file for both x86 and x64:

```
x64:
  load "capability.x64.o"
    dfr "resolve" "ror13"
    fixbss "_bss_fix"
    make pic +optimize +mutate
  load "payload.bin"
    append $PIC
  link "output.x64.bin"

x86:
  load "capability.x86.o"
    dfr "resolve" "ror13"
    fixptrs "_caller"
    fixbss "_bss_fix"
    make pic +optimize +mutate
  load "payload.bin"
    append $PIC
  link "output.x86.bin"
```

### Polymorphic Capabilities

Using +mutate for unique instances:

```bash
# Generate 100 unique instances
for i in {1..100}; do
    ./link capability.spec.txt
    mv output.bin polymorphic_$i.bin
done
```

Each iteration produces different:
- Constant values
- Stack string implementations
- Noise instruction patterns
- Function ordering (if +disco used)

### Multi-Stage Loaders

Stage 1: Minimal loader
Stage 2: Decryption routine
Stage 3: Full capability

```
# Stage 1 spec
load "stage1.x64.o"
  make pic +mutate
load "encrypted_stage2.bin"
  append $PIC
load "encrypted_stage3.bin"
  append $PIC
link "dropper.bin"
```

```c
// stage1.x64.c
extern unsigned char _binary_encrypted_stage2_bin_start[];
extern unsigned char _binary_encrypted_stage3_bin_start[];

void go(void) {
    // Decrypt stage 2
    decrypt(_binary_encrypted_stage2_bin_start, key);

    // Execute stage 2, which decrypts and executes stage 3
    void (*stage2)() = (void(*)())_binary_encrypted_stage2_bin_start;
    stage2();
}
```

### PICO Export Functions

For modular capabilities:

```
exportfunc "capability_init" "__tag_init"
exportfunc "capability_exec" "__tag_exec"
exportfunc "capability_cleanup" "__tag_cleanup"
```

```c
// In calling code
PICOMAIN_FUNC pInit = PicoGetExport(pico, pico, __tag_init());
PICOMAIN_FUNC pExec = PicoGetExport(pico, pico, __tag_exec());
PICOMAIN_FUNC pCleanup = PicoGetExport(pico, pico, __tag_cleanup());

pInit();
pExec();
pCleanup();
```

Tags prevent synchronization errors between builder and runner.

### Conditional Capability Assembly

Use remap for conditional compilation:

```c
// In code
#ifdef FEATURE_HTTP
void go_http(void) { ... }
#endif

#ifdef FEATURE_SMB
void go_smb(void) { ... }
#endif
```

Spec file:
```
# HTTP build
x64:
  remap "go_http" "go"
  load "capability.x64.o"
    make pic +optimize
  link "http_capability.bin"

# SMB build
x64:
  remap "go_smb" "go"
  load "capability.x64.o"
    make pic +optimize
  link "smb_capability.bin"
```

Link-time optimization removes unused code path.

---

## Conclusion

Crystal Palace represents a paradigm shift in offensive capability development. By providing:

1. **Domain-Specific Abstractions**: Linker script language tailored for tradecraft
2. **Binary Transformation**: Automated optimization, mutation, and instrumentation
3. **PIC Ergonomics**: fixptrs and fixbss eliminate manual PIC complexity
4. **Aspect-Oriented Design**: Separate tradecraft from capability logic
5. **Modular Composition**: Build capabilities from reusable components

Crystal Palace gives individual researchers the productivity advantages of entire teams, embodying the "leverage" principle central to Tradecraft Garden's philosophy.

The framework enables rapid hypothesis testing, polymorphic capability generation, and sophisticated in-memory evasion techniques - all while maintaining clean, readable C code and automated build processes.

---

## References

- [Planting a Tradecraft Garden](https://aff-wg.org/2025/06/04/planting-a-tradecraft-garden/)
- [Tilling the Soil - Binary Transform](https://aff-wg.org/2025/07/09/tradecraft-garden-tilling-the-soil/)
- [COFFing Out the Night Soil](https://aff-wg.org/2025/09/10/coffing-out-the-night-soil)
- [Weeding the Garden - PIC Ergonomics](https://aff-wg.org/2025/10/13/weeding-the-tradecraft-garden/)
- [PIC Parterre - DFR/fixbss/remap](https://aff-wg.org/2025/10/27/tradecraft-gardens-pic-parterre/)
- [Aspect-Oriented Programming](https://aff-wg.org/2025/11/10/tradecraft-engineering-with-aspect-oriented-programming/)
