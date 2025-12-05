# COFF Operations - Common Object File Format

## Overview

COFF (Common Object File Format) is a portable object file format that Crystal Palace uses extensively for building modular capabilities. Understanding COFF operations is key to mastering Tradecraft Garden techniques.

## What is COFF?

COFF is:
- An **object file format** produced by compilers
- A **container** for code, data, symbols, and relocations
- **Smaller** than full PE/DLL files (no PE headers, import tables, etc.)
- **Linkable** - multiple COFFs can be merged
- **Mutable** - easier to transform than DLLs

## Why COFF for Tradecraft?

### Advantages
1. **Size** - No PE overhead (~1KB saved per file)
2. **Flexibility** - Easy to merge multiple objects
3. **Mutability** - Simpler structure for transformations
4. **Modularity** - Build capabilities from components
5. **Compatibility** - Standard format across toolchains

### COFF vs DLL
| Feature | COFF | DLL |
|---------|------|-----|
| PE Headers | ❌ No | ✅ Yes |
| Import Table | ❌ No | ✅ Yes |
| Export Table | ❌ No (symbols only) | ✅ Yes |
| Size Overhead | ~0KB | ~1-2KB |
| Directly Executable | ❌ Needs loader | ✅ LoadLibrary |
| Linkable | ✅ Easy | ❌ Difficult |

## COFF Structure

```
┌──────────────────────────────┐
│       COFF Header            │  File metadata
├──────────────────────────────┤
│    Section Table             │  .text, .data, .rdata, .bss
├──────────────────────────────┤
│    .text (code)              │  Executable instructions
├──────────────────────────────┤
│    .data (initialized)       │  Global variables with values
├──────────────────────────────┤
│    .rdata (read-only)        │  Constants, strings
├──────────────────────────────┤
│    Symbol Table              │  Function/variable names
├──────────────────────────────┤
│    Relocation Table          │  Addresses to fix during linking
├──────────────────────────────┤
│    String Table              │  Symbol name strings
└──────────────────────────────┘
```

## Crystal Palace COFF Operations

### 1. COFF Normalization

**What**: Collapses multiple related sections into standard sections

**Why**: Compilers generate .text$mn, .text$x, etc. Crystal Palace normalizes to .text

**When**: Automatically when loading COFF files

```
load "capability.x64.o"  # Normalization happens here
```

**Result**:
```
Before: .text$mn, .text$x, .text$zz, .data$r, .data$w
After:  .text, .data, .rdata
```

### 2. COFF Merge

**What**: Combines multiple COFF files into one

**Syntax**:
```
load "module1.x64.o"
  merge
load "module2.x64.o"
  merge
load "module3.x64.o"
  merge
```

**Process**:
1. Load first COFF
2. Load second COFF
3. Merge second into first (combine sections, symbols, relocations)
4. Repeat for each additional COFF

**Result**: Single COFF containing all modules

**Example**:
```
load "core.x64.o"
  merge
load "http.x64.o"
  merge
load "crypto.x64.o"
  merge
  make pico
link "full_capability.pico"
```

### 3. COFF Export

**What**: Generate normalized COFF output

**Syntax**:
```
load "capability.x64.o"
  make coff
link "output.o"
```

**Use Cases**:
- Build PICO from multiple modules
- Create linkable objects for later use
- Normalize compiler output

**Benefits**:
- Standard section layout
- Clean symbol table
- Ready for PICO runner

### 4. COFF as Capability

**What**: Use COFF files directly as capabilities instead of DLLs

**Advantages**:
- Smaller size
- Easier to mutate
- Simpler structure
- Faster to process

**Workflow**:
```
# Build capability as COFF
load "capability.x64.o"
  make coff +optimize +mutate
link "capability.o"

# Load in PICO runner
load "pico_runner.x64.o"
  make pic
load "capability.o"
  append $PIC
```

## COFF File Operations

### Inspecting COFF

**Using objdump**:
```bash
# View sections
objdump -h capability.o

# View symbols
objdump -t capability.o

# Disassemble
objdump -d capability.o

# View relocations
objdump -r capability.o
```

**Using dumpbin (Windows)**:
```cmd
dumpbin /headers capability.obj
dumpbin /symbols capability.obj
dumpbin /disasm capability.obj
dumpbin /relocations capability.obj
```

### Creating COFF

**GCC/Clang**:
```bash
# Compile to COFF object
gcc -c capability.c -o capability.o

# x64
x86_64-w64-mingw32-gcc -c capability.c -o capability.x64.o

# x86
i686-w64-mingw32-gcc -c capability.c -o capability.x86.o
```

**MSVC**:
```cmd
cl /c capability.c /Focapability.obj
```

### Linking Multiple COFFs

**Traditional Linker**:
```bash
# Link multiple objects
ld module1.o module2.o module3.o -o output.o
```

**Crystal Palace**:
```
load "module1.o"
  merge
load "module2.o"
  merge
load "module3.o"
  merge
  make coff
```

## COFF Transformation Pipeline

```
Source Files (C/C++)
    ↓
Compile to COFF
    ↓
Load into Crystal Palace
    ↓
Normalize Sections
    ↓
Merge (if multiple)
    ↓
Transform (optimize, mutate, disco)
    ↓
Export as COFF or convert to PICO/PIC
    ↓
Append resources (optional)
    ↓
Link final capability
```

## Modular Capability Assembly

### Pattern: Core + Extensions

```
# Core functionality
load "core.x64.o"
  merge

# HTTP extension
load "ext_http.x64.o"
  merge

# SMB extension
load "ext_smb.x64.o"
  merge

# Finalize
  make coff +optimize
link "full_capability.o"
```

### Pattern: Shared Components

```
# Shared utilities
load "utils.x64.o"
  merge

# Capability 1
load "screenshot.x64.o"
  merge
  make coff
link "screenshot_capability.o"

# Capability 2 (reuses utils)
load "utils.x64.o"
  merge
load "keylog.x64.o"
  merge
  make coff
link "keylog_capability.o"
```

## COFF Optimization

### Link-Time Optimization

```
load "capability.x64.o"
  make coff +optimize
```

**Benefits**:
- Removes unused functions
- Eliminates dead code
- Reduces size by 30-70%

**How it works**:
1. Start from entry point `go()`
2. Build call graph
3. Mark reachable functions
4. Remove unreachable functions and data

### Function Randomization

```
load "capability.x64.o"
  make coff +disco
```

**Benefits**:
- Randomizes function order
- Breaks signature patterns
- Maintains entry point at position 0

### Code Mutation

```
load "capability.x64.o"
  make coff +mutate
```

**Benefits**:
- Mutates constants
- Creates stack strings
- Adds noise instructions
- Unique signature per build

## COFF Symbol Operations

### Symbol Remapping

```
# Rename symbols
remap "old_name" "new_name"
```

**Use Case**: API convention conversion

```
x64:
  remap "__imp_VirtualAlloc" "__imp_KERNEL32$VirtualAlloc"
  remap "__imp_CreateThread" "__imp_KERNEL32$CreateThread"
  load "capability.x64.o"
    make coff
```

### Function Exports

```
exportfunc "screenshot" "__tag_screenshot"
exportfunc "keylog" "__tag_keylog"
```

**Use Case**: PICO capabilities with multiple entry points

## COFF Loader Implementation

### Minimal COFF Loader

```c
typedef void (*COFF_ENTRY)(void);

void execute_coff(void* coff_data, size_t size) {
    // 1. Parse COFF headers
    PIMAGE_FILE_HEADER header = (PIMAGE_FILE_HEADER)coff_data;

    // 2. Calculate image size
    size_t image_size = calculate_image_size(header);

    // 3. Allocate memory
    void* base = VirtualAlloc(NULL, image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // 4. Load sections
    load_coff_sections(coff_data, base);

    // 5. Process relocations
    process_coff_relocations(coff_data, base);

    // 6. Resolve symbols
    resolve_coff_symbols(coff_data, base);

    // 7. Execute entry point
    COFF_ENTRY entry = (COFF_ENTRY)base;
    entry();
}
```

### Section Loading

```c
void load_coff_sections(void* coff, void* base) {
    PIMAGE_FILE_HEADER fh = (PIMAGE_FILE_HEADER)coff;
    PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)(fh + 1);

    for (int i = 0; i < fh->NumberOfSections; i++) {
        void* dest = (BYTE*)base + sections[i].VirtualAddress;
        void* src = (BYTE*)coff + sections[i].PointerToRawData;

        memcpy(dest, src, sections[i].SizeOfRawData);
    }
}
```

### Relocation Processing

```c
void process_coff_relocations(void* coff, void* base) {
    // Walk relocation table
    // Fix addresses based on load address
    // Apply relocations to loaded sections
}
```

## COFF Best Practices

### 1. Modular Design
```c
// core.c - Core functionality
void init_capability(void);
void execute_capability(void);

// ext_network.c - Network extension
void network_send(void* data, size_t size);
void network_recv(void* buffer, size_t size);

// Merge at build time
```

### 2. Symbol Visibility
```c
// Use static for internal functions
static void internal_helper(void) { }

// Public functions for exports
void public_api(void) { }
```

### 3. Section Organization
```c
// Use section attributes for organization
__attribute__((section(".text"))) void code_func(void) { }
__attribute__((section(".data"))) int data_var = 42;
__attribute__((section(".rdata"))) const char* str = "const";
```

## Related Techniques

- [PICO](../pico/) - COFF-based capabilities
- [Crystal Palace](../crystal-palace/) - COFF operations
- [Binary Transformation](../binary-transformation/) - COFF transformations

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Explore [POC](./POC/) for code examples
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational use
