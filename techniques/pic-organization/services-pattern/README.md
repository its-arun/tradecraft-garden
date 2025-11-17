# Services PICO Pattern - Modular PIC Organization

## Overview

The **Services PICO Pattern** is an architectural approach to organizing position-independent code that centralizes common functionality (like API resolution) in a shared "services" component, eliminating code duplication across multiple PICOs.

## The Problem

Traditional PICO development leads to duplication:

```c
// pico1.c
FARPROC resolve_api(DWORD hash) { /* implementation */ }
void capability1() {
    void* mem = resolve_api(HASH_VIRTUALALLOC)(...);
}

// pico2.c
FARPROC resolve_api(DWORD hash) { /* SAME implementation */ }
void capability2() {
    HANDLE h = resolve_api(HASH_OPENPROCESS)(...);
}

// pico3.c
FARPROC resolve_api(DWORD hash) { /* SAME implementation */ }
void capability3() {
    // ...
}
```

**Result**: Every PICO contains duplicate resolver code, increasing size and maintenance burden.

## The Solution: Services PICO

Centralize shared functionality in a "services" PICO that other PICOs inherit:

```
┌──────────────────────────────┐
│      Services PICO           │
│  • API resolution functions  │
│  • Global variables          │
│  • Common utilities          │
└──────────────┬───────────────┘
               │
               │ (merged into)
               ↓
┌──────────────────────────────┐
│    Capability PICOs          │
│  • capability1.pico          │
│  • capability2.pico          │
│  • capability3.pico          │
│  (all include services)      │
└──────────────────────────────┘
```

## Implementation

### Services PICO Structure

```c
// services.c
#include <windows.h>

// ============================================================================
// SHARED RESOLVERS
// ============================================================================

// Explicit resolver for guaranteed modules (KERNEL32, NTDLL)
void* resolve_explicit(DWORD module_hash, DWORD function_hash) {
    // EAT walking only - no LoadLibrary
    HMODULE hModule = find_module_by_hash(module_hash);
    return find_function_by_hash(hModule, function_hash);
}

// Default resolver for optional modules
void* resolve_default(const char* module, const char* function) {
    // Can call GetModuleHandleA/LoadLibraryA
    HMODULE hModule = KERNEL32$GetModuleHandleA(module);
    if (!hModule) {
        hModule = KERNEL32$LoadLibraryA(module);
    }
    return (void*)KERNEL32$GetProcAddress(hModule, function);
}

// ============================================================================
// SHARED GLOBALS
// ============================================================================

// Global state (with fixbss, these work in PICOs)
DWORD g_last_error = 0;
void* g_heap_base = NULL;

// ============================================================================
// SHARED UTILITIES
// ============================================================================

void* services_malloc(size_t size) {
    if (!g_heap_base) {
        g_heap_base = KERNEL32$HeapCreate(0, 0x10000, 0);
    }
    return KERNEL32$HeapAlloc(g_heap_base, HEAP_ZERO_MEMORY, size);
}

void services_free(void* ptr) {
    if (g_heap_base && ptr) {
        KERNEL32$HeapFree(g_heap_base, 0, ptr);
    }
}
```

### Capability PICO Using Services

```c
// capability1.c
#include <windows.h>

// Services functions are available after merge
extern void* resolve_explicit(DWORD module_hash, DWORD function_hash);
extern void* resolve_default(const char* module, const char* function);
extern void* services_malloc(size_t size);
extern void services_free(void* ptr);

// Capability-specific code
void go(void) {
    // Use shared resolver
    void* mem = KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);

    // Use shared memory allocator
    char* buffer = services_malloc(256);

    // Do capability work
    // ...

    services_free(buffer);
}
```

### Crystal Palace Specification

```
# Build services PICO
x64:
  load "services.x64.o"

  # Configure dual-resolver pattern
  dfr "resolve_explicit" "ror13" "KERNEL32, NTDLL"
  dfr "resolve_default" "strings"

  # Enable globals
  fixbss "_bss_fix"

  make pico
  link "services.x64.pico"

# Build capability using services
x64:
  # Load services first
  load "services.x64.o"
    merge

  # Load capability
  load "capability1.x64.o"
    merge

  # Configure (inherits services resolvers)
  dfr "resolve_explicit" "ror13" "KERNEL32, NTDLL"
  dfr "resolve_default" "strings"
  fixbss "_bss_fix"

  make pico +optimize
  link "capability1.x64.pico"
```

## Dual-Resolver Pattern

### The Pattern

```
Explicit Resolver → Guaranteed modules (KERNEL32, NTDLL)
Default Resolver  → Optional modules (can load new DLLs)
```

### Why This Works

**Explicit Resolver**:
- Handles KERNEL32 and NTDLL only
- Uses EAT walking (no LoadLibrary)
- Fast and safe

**Default Resolver**:
- Handles all other modules
- **Can safely use KERNEL32 APIs** (already resolved by explicit resolver)
- Loads modules on demand

### Configuration

```
# Specification
dfr "resolve_explicit" "ror13" "KERNEL32, NTDLL"
dfr "resolve_default" "strings"
```

**Crystal Palace Processing**:
1. Identifies KERNEL32/NTDLL API calls → uses resolve_explicit
2. All other module API calls → uses resolve_default
3. resolve_default can use KERNEL32$LoadLibraryA safely

### Implementation

```c
// Explicit: Hash-based, no module loading
void* resolve_explicit(DWORD module_hash, DWORD function_hash) {
    // Walk PEB to find KERNEL32 or NTDLL
    HMODULE hModule = find_loaded_module_by_hash(module_hash);

    // Walk EAT
    return find_function_by_hash(hModule, function_hash);
}

// Default: String-based, can load modules
void* resolve_default(const char* module, const char* function) {
    // Safe to use KERNEL32 APIs (resolved by explicit)
    HMODULE hModule = KERNEL32$GetModuleHandleA(module);

    if (!hModule) {
        // Can load new modules
        hModule = KERNEL32$LoadLibraryA(module);
    }

    return (void*)KERNEL32$GetProcAddress(hModule, function);
}
```

## Swappable Evasion

### The Benefit

By centralizing resolution in services, you can swap evasion techniques without modifying capabilities.

### Example: Thread Pool Proxying

```c
// services_evasive.c
#include "libtp.h"  // Thread Pool proxying library

void* resolve_default_evasive(const char* module, const char* function) {
    // Use LdrLoadDll via Thread Pool instead of LoadLibraryA
    HMODULE hModule = tp_ldr_load_dll(module);

    // Use GetProcAddress via Thread Pool
    return tp_get_proc_address(hModule, function);
}
```

**Rebuild capabilities**:
```
# Just change services implementation
# Capabilities inherit new evasion automatically

load "services_evasive.x64.o"
  merge
load "capability1.x64.o"
  merge
  make pico +optimize
```

**Result**: All capabilities now use Thread Pool evasion without any code changes.

## Advantages

### 1. Code Reuse
- Write resolver once
- Use in all PICOs
- Single source of truth

### 2. Size Reduction
```
Without services:
capability1.pico: 50KB (includes resolver)
capability2.pico: 50KB (includes resolver)
capability3.pico: 50KB (includes resolver)
Total: 150KB

With services:
capability1.pico: 30KB (no resolver)
capability2.pico: 30KB (no resolver)
capability3.pico: 30KB (no resolver)
Total: 90KB (40% reduction)
```

### 3. Consistent Behavior
- All PICOs use same resolver
- Same global state
- Same utilities

### 4. Easy Evasion Updates
- Update services once
- Rebuild all capabilities
- All inherit new evasion

### 5. Simplified Development
- Capability developers don't write resolvers
- Focus on capability logic
- Less room for errors

## Design Patterns

### Pattern 1: Minimal Services
```c
// Just resolver
void* resolve(DWORD hash);
```

### Pattern 2: Standard Services
```c
// Resolver + utilities
void* resolve(DWORD hash);
void* services_malloc(size_t size);
void services_free(void* ptr);
```

### Pattern 3: Full Services
```c
// Resolver + utilities + globals
void* resolve_explicit(...);
void* resolve_default(...);
void* services_malloc(size_t size);
void services_free(void* ptr);

DWORD g_last_error;
void* g_heap_base;
DWORD g_session_id;
```

### Pattern 4: Evasive Services
```c
// All of above + evasion
void* resolve_via_threadpool(...);
void* syscall_direct(...);
void spoof_stack_before_call(...);
```

## Best Practices

### 1. Keep Services Focused
✅ Do:
- API resolution
- Basic utilities
- Shared state

❌ Don't:
- Capability-specific logic
- Complex algorithms
- Large dependencies

### 2. Version Services
```c
#define SERVICES_VERSION_MAJOR 1
#define SERVICES_VERSION_MINOR 2

// Capabilities can check version
#if SERVICES_VERSION_MAJOR != 1
#error "Incompatible services version"
#endif
```

### 3. Document Services API
```c
/**
 * Resolve API by hash (explicit resolver)
 * @param module_hash ROR13 hash of module (KERNEL32/NTDLL only)
 * @param function_hash ROR13 hash of function
 * @return Function pointer or NULL
 */
void* resolve_explicit(DWORD module_hash, DWORD function_hash);
```

### 4. Use Link-Time Optimization
```
make pico +optimize
```
Removes unused services functions from each capability.

## Related Techniques

- [Crystal Palace](../../crystal-palace/) - Building and merging PICOs
- [Dynamic Function Resolution](../../dynamic-function-resolution/) - DFR fundamentals
- [Shared Libraries](../../shared-libraries/) - Related code reuse pattern

## Resources

- [Arranging the PIC Parterre](https://rastamouse.me/arranging-the-pic-parterre/) - Rasta Mouse
- [Tradecraft Garden](https://tradecraftgarden.org/) - Official site

## Next Steps

1. Study the POC example for services implementation
2. Understand dual-resolver pattern benefits
3. Practice building modular PICO architectures
4. Experiment with swappable evasion techniques
