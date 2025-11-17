# Services PICO Pattern - Quick Notes

## Core Concept
Centralize shared functionality (resolvers, utilities, globals) in a "services" PICO that capabilities merge.

## The Problem
```c
// Traditional: Every PICO duplicates code
pico1.c: resolver + capability1
pico2.c: resolver + capability2 (DUPLICATE resolver)
pico3.c: resolver + capability3 (DUPLICATE resolver)
```

## The Solution
```c
// Services pattern: Shared code
services.c: resolver + utilities + globals
pico1.c: capability1 (no resolver)
pico2.c: capability2 (no resolver)
pico3.c: capability3 (no resolver)

// Build: merge services into each
services + pico1 → final_pico1
services + pico2 → final_pico2
```

## Services Structure
```c
// services.c
void* resolve_explicit(DWORD module_hash, DWORD func_hash);
void* resolve_default(const char* module, const char* func);
void* services_malloc(size_t size);
void services_free(void* ptr);

DWORD g_last_error;
void* g_heap_base;
```

## Capability Using Services
```c
// capability.c
extern void* resolve_explicit(...);
extern void* resolve_default(...);
extern void* services_malloc(size_t);

void go(void) {
    // Use shared functions
    void* mem = KERNEL32$VirtualAlloc(...);  // Uses resolve_explicit
    char* buf = services_malloc(256);        // Uses shared allocator
}
```

## Dual-Resolver Pattern

### Why Two Resolvers?
```
resolve_explicit: KERNEL32, NTDLL (guaranteed loaded)
resolve_default:  All others (can load new modules)
```

### Key Insight
```c
// Explicit resolver handles KERNEL32
void* resolve_explicit(DWORD hash) {
    // No LoadLibrary, just EAT walking
}

// Default resolver CAN USE KERNEL32 APIs
// (because explicit already resolved them!)
void* resolve_default(const char* module, const char* func) {
    HMODULE h = KERNEL32$LoadLibraryA(module);  // SAFE!
    return KERNEL32$GetProcAddress(h, func);
}
```

## Crystal Palace Config
```
x64:
  # Build with services
  load "services.x64.o"
    merge
  load "capability.x64.o"
    merge

  # Configure dual resolvers
  dfr "resolve_explicit" "ror13" "KERNEL32, NTDLL"
  dfr "resolve_default" "strings"

  # Enable globals
  fixbss "_bss_fix"

  make pico +optimize
```

## Swappable Evasion

### Change Evasion Without Touching Capabilities
```c
// services_v1.c
void* resolve_default(const char* mod, const char* func) {
    return LoadLibraryA + GetProcAddress;  // Vanilla
}

// services_v2.c
void* resolve_default(const char* mod, const char* func) {
    return tp_ldr_load_dll + tp_get_proc;  // Thread Pool evasion
}

// Rebuild capabilities with v2:
// All capabilities now use TP evasion, no code changes!
```

## Benefits

### Size Reduction
```
3 capabilities, each 50KB with embedded resolver:
Without services: 150KB total

3 capabilities, each 30KB + shared services:
With services: 90KB total (40% smaller)
```

### Consistency
- All use same resolver
- Same behavior across PICOs
- Single source of truth

### Maintainability
- Update resolver once
- Rebuild all capabilities
- Instant propagation

### Development Speed
- Capability devs don't write resolvers
- Focus on capability logic
- Less code, fewer bugs

## Common Patterns

### Pattern 1: Minimal
```c
void* resolve(DWORD hash);
```

### Pattern 2: Standard
```c
void* resolve_explicit(...);
void* resolve_default(...);
void* malloc(size_t);
void free(void*);
```

### Pattern 3: Full
```c
// Above + globals + more utilities
DWORD g_last_error;
void* g_heap;
int strlen(const char*);
void* memcpy(void*, const void*, size_t);
```

## Build Flow
```
1. Compile services.c → services.x64.o
2. Compile capability.c → capability.x64.o
3. Crystal Palace: load services, merge, load capability, merge
4. Configure DFR for combined object
5. make pico +optimize (removes unused services functions)
6. link final.pico
```

## Link-Time Optimization
```
+optimize removes unused services:

services.c has 20 functions
capability.c uses 5 of them

make pico +optimize:
→ Final PICO contains only 5 used functions
→ Automatic dead code elimination
```

## Version Management
```c
// services.h
#define SERVICES_VERSION 1

// capability.c
#if SERVICES_VERSION != 1
#error "Incompatible services"
#endif
```

## Example: Screenshot PICO
```c
// Without services (50KB)
#include "resolver.h"      // 10KB
#include "utils.h"         // 5KB
void screenshot() { ... }  // 35KB

// With services (35KB)
extern void* resolve(...);
extern void* malloc(...);
void screenshot() { ... }  // 35KB

// Services merged at build time
// But shared across all PICOs
```

## API Convention
```c
// services.h
void* resolve_explicit(DWORD module_hash, DWORD function_hash);
void* resolve_default(const char* module, const char* function);
void* services_malloc(size_t size);
void services_free(void* ptr);
int services_strlen(const char* str);
void* services_memcpy(void* dest, const void* src, size_t n);

// All prefixed with "services_" for clarity
```

## Global State
```c
// services.c (with fixbss)
DWORD g_last_error = 0;
void* g_heap_base = NULL;
BOOL g_initialized = FALSE;

void services_init(void) {
    if (g_initialized) return;

    g_heap_base = KERNEL32$HeapCreate(0, 0x10000, 0);
    g_initialized = TRUE;
}

// All capabilities share this state
```

## Testing
```bash
# 1. Build services PICO standalone
./link services.spec

# 2. Build capability with services
./link capability.spec

# 3. Verify size reduction
ls -lh *.pico

# 4. Test functionality
# Load PICO and execute
```

## Migration Path
```
Step 1: Extract common code to services.c
Step 2: Build services PICO
Step 3: Update capabilities to extern services functions
Step 4: Update spec files to merge services
Step 5: Rebuild and test
Step 6: Deploy
```

## OPSEC Benefits
- Swap evasion quickly (just rebuild)
- Different evasion per operation (different services build)
- No capability code changes (attribution harder)

## Tags
`#pic-organization` `#services` `#modular-design` `#code-reuse` `#dual-resolver`
