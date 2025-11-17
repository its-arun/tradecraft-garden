# PICO - Position Independent Capability Objects

## Overview

PICO (Position Independent Capability Object) is a convention for executing one-time or persistent COFF files from position-independent code. Think of it as "Beacon Object Files (BOFs) for any PIC loader."

## What is PICO?

PICO is:
- A **convention** for packaging capabilities as COFF files
- A **standard** for PIC loaders to execute COFFs
- A **format** similar to BOFs but vendor-neutral
- A **framework** for modular post-exploitation

## PICO vs BOF

| Feature | BOF | PICO |
|---------|-----|------|
| Format | COFF | COFF |
| Loader | Cobalt Strike Beacon | Any PIC loader |
| Scope | Cobalt Strike ecosystem | Universal |
| Entry Point | Named function | Tag-based |
| Persistence | One-shot execution | Can be persistent |
| Exports | Limited | Full export support |

## PICO Architecture

```
┌─────────────────────────────────┐
│     PIC Loader / Runner         │
├─────────────────────────────────┤
│  • PICO Execution Engine        │
│  • Memory Management            │
│  • Symbol Resolution            │
│  • Export Discovery             │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│     PICO Capability (COFF)      │
├─────────────────────────────────┤
│  • Entry Point: go()            │
│  • Exported Functions (tagged)  │
│  • Resources                    │
│  • Relocations                  │
└─────────────────────────────────┘
```

## Building PICOs

### Simple PICO
```c
// capability.c
#include <windows.h>

void go(void) {
    // PICO entry point
    MessageBoxA(NULL, "Hello from PICO", "PICO", MB_OK);
}
```

Compile:
```bash
gcc -c capability.c -o capability.o
```

Crystal Palace:
```
load "capability.x64.o"
  make pico
link "capability.pico"
```

### PICO with Exports

```c
// multi_capability.c

// Entry point
void go(void) {
    // Initialization
}

// Exported functions
void screenshot(void) {
    // Take screenshot
}

void keylog_start(void) {
    // Start keylogger
}

void keylog_stop(void) {
    // Stop keylogger
}
```

Crystal Palace:
```
load "multi_capability.x64.o"
  exportfunc "screenshot" "__tag_screenshot"
  exportfunc "keylog_start" "__tag_keylog_start"
  exportfunc "keylog_stop" "__tag_keylog_stop"
  make pico
link "multi_capability.pico"
```

## Executing PICOs

### Basic Execution
```c
// In your PIC loader
typedef void (*PICOMAIN_FUNC)(void);

void execute_pico(void* pico_data, size_t pico_size) {
    // 1. Allocate memory
    void* pico_mem = VirtualAlloc(NULL, pico_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // 2. Copy PICO
    memcpy(pico_mem, pico_data, pico_size);

    // 3. Process relocations
    process_pico_relocations(pico_mem);

    // 4. Resolve imports (if any)
    resolve_pico_imports(pico_mem);

    // 5. Execute entry point (offset 0)
    PICOMAIN_FUNC pico_entry = (PICOMAIN_FUNC)pico_mem;
    pico_entry();
}
```

### Accessing Exports
```c
// Get exported function by tag
PICOMAIN_FUNC pScreenshot = PicoGetExport(pico, pico, __tag_screenshot());

if (pScreenshot) {
    pScreenshot();  // Execute exported function
}
```

## PICO Conventions

### Entry Point
- Function named `go()`
- Signature: `void go(void)`
- Located at offset 0 (use `+gofirst`)

### Exports
- Use Crystal Palace `exportfunc` to tag functions
- Tags are random integers preventing sync errors
- Access via `PicoGetExport()`

### Global Variables
- PICOs **support** read/write globals (unlike PIC for shared libs)
- Each PICO instance has its own globals
- Use fixbss if needed

### API Resolution
- PICOs can use standard API calling convention
- Loader resolves imports OR
- PICO includes DFR resolver

## PICO Use Cases

### 1. Modular Post-Exploitation
```
Core Loader → Load PICOs as needed:
  - screenshot.pico
  - credentials.pico
  - lateral_movement.pico
  - exfil.pico
```

### 2. Capability Updates
```
Update capabilities without replacing loader:
  - Upload new PICO
  - Execute updated functionality
  - No need to re-establish C2
```

### 3. Task-Specific Execution
```
C2 Server → Task → Load specific PICO → Execute → Unload
```

### 4. Plugin Architecture
```
Main Implant + Plugin System:
  - Core functionality in main implant
  - Extended capabilities as PICOs
  - Load/unload dynamically
```

## PICO Loader Implementation

### Minimal PICO Runner
```c
#include "pico.h"

void execute_pico_from_memory(void* pico_data, size_t size) {
    PICO_CONTEXT ctx = {0};

    // Parse PICO (COFF format)
    if (!pico_parse(pico_data, size, &ctx))
        return;

    // Allocate memory for PICO
    ctx.base = VirtualAlloc(NULL, ctx.image_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // Load sections
    pico_load_sections(&ctx);

    // Process relocations
    pico_process_relocations(&ctx);

    // Resolve imports
    pico_resolve_imports(&ctx);

    // Execute entry point
    PICOMAIN_FUNC entry = (PICOMAIN_FUNC)ctx.base;
    entry();

    // Optional: Keep loaded for exports
    // Or: Free memory after execution
}
```

### PICO Context Structure
```c
typedef struct {
    void* base;                  // Base address in memory
    size_t image_size;           // Total image size
    PIMAGE_SECTION_HEADER sections;
    DWORD section_count;
    PIMAGE_SYMBOL symbols;
    DWORD symbol_count;
    // ... other fields
} PICO_CONTEXT;
```

## Crystal Palace PICO Features

### Creating PICOs
```
load "capability.x64.o"
  make pico
```

### PICO with Transformations
```
load "capability.x64.o"
  make pico +optimize +mutate
```

### PICO Exports
```
exportfunc "function_name" "__tag_function_name"
```

### PICO with Resources
```
load "capability.x64.o"
  make pico
load "data.bin"
  append $PICO
```

## PICO Best Practices

### 1. Small and Focused
✅ One capability per PICO
❌ Monolithic multi-purpose PICOs

### 2. Clean Entry/Exit
```c
void go(void) {
    // Initialize
    setup();

    // Execute capability
    do_work();

    // Cleanup
    teardown();
}
```

### 3. Error Handling
```c
void go(void) {
    if (!initialize()) {
        return;  // Fail gracefully
    }

    execute_capability();
}
```

### 4. Minimize Dependencies
- Avoid C runtime functions
- Use Windows APIs directly
- Include minimal helper functions

### 5. Export Strategy
```c
// Clear exports for reusable capabilities
exportfunc "init" "__tag_init"
exportfunc "execute" "__tag_execute"
exportfunc "cleanup" "__tag_cleanup"

// Allows caller to control lifecycle
```

## PICO Security Considerations

### Memory Protection
```c
// After loading, change to RX
VirtualProtect(pico_base, pico_size, PAGE_EXECUTE_READ, &old_protect);
```

### Unload After Use
```c
// For one-shot PICOs
execute_pico(pico_data, pico_size);
VirtualFree(pico_base, 0, MEM_RELEASE);
```

### Encrypted PICOs
```
# Encrypt before appending
load "loader.x64.o"
  make pic
load "encrypted_capability.pico"  # Pre-encrypted
  append $PIC
```

## Related Techniques

- [COFF Operations](../coff-operations/) - Understanding COFF format
- [Position Independent Code](../position-independent-code/) - PIC fundamentals
- [Crystal Palace](../crystal-palace/) - PICO building
- [Binary Transformation](../binary-transformation/) - PICO transformations

## Resources

- [Tradecraft Garden Examples](https://tradecraftgarden.org/)
- [BOF Inspiration](https://hstechdocs.helpsystems.com/manuals/cobaltstrike/current/userguide/content/topics/beacon-object-files_main.htm)

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study POC examples in [POC/](./POC/)
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational use
