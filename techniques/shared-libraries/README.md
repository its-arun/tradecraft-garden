# Shared Libraries for PIC/PICO

## Overview

Crystal Palace shared libraries are ZIP-based archives containing compiled objects that follow unified conventions across x86/x64 for both PIC and PICOs. They enable code reuse and modular development while maintaining position independence.

## What are Shared Libraries?

Shared libraries in Tradecraft Garden:
- **ZIP archives** containing `.x86.o` and `.x64.o` files
- **Unified conventions** across architectures
- **PIC-compatible** code (with constraints)
- **Link-time merged** into capabilities
- **Optimizable** - unused code removed automatically

## LibTCG: The Tradecraft Garden Library

**LibTCG** is the standard shared library for Tradecraft Garden, providing common functionality previously duplicated across examples.

### LibTCG Features

- **DLL Parsing**: Parse PE headers, find exports
- **PICO Execution**: Load and execute PICO objects
- **Printf Debugging**: Debug output for development
- **EAT Resolution**: Export Address Table walking
- **Common Utilities**: String manipulation, memory operations

### Using LibTCG

```c
// In your code
#include "tcg.h"

void go(void) {
    // Use LibTCG functions
    tcg_printf("Debug: capability starting\n");

    HMODULE hKernel32 = tcg_get_module_by_hash(HASH_KERNEL32);
    FARPROC pVA = tcg_get_export(hKernel32, "VirtualAlloc");
}
```

### Linking LibTCG

```
load "capability.x64.o"
  mergelib "lib/libtcg.x64.zip"
  make pic +optimize
```

## Shared Library Format

### Directory Structure

```
mylib.x64.zip
  ├── module1.x64.o
  ├── module2.x64.o
  ├── module3.x64.o
  └── mylib.h

mylib.x86.zip
  ├── module1.x86.o
  ├── module2.x86.o
  ├── module3.x86.o
  └── mylib.h
```

### Architecture Naming

- **x64**: `*.x64.o`, `lib.x64.zip`
- **x86**: `*.x86.o`, `lib.x86.zip`

## Creating Shared Libraries

### Step 1: Write Library Code

```c
// mylib.c
#include <windows.h>

// Utility function
void* my_malloc(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void my_free(void* ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

// String function
int my_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}
```

### Step 2: Create Header

```c
// mylib.h
#ifndef MYLIB_H
#define MYLIB_H

void* my_malloc(size_t size);
void my_free(void* ptr);
int my_strlen(const char* str);

#endif
```

### Step 3: Compile for Both Architectures

```bash
# x64
x86_64-w64-mingw32-gcc -c mylib.c -o mylib.x64.o

# x86
i686-w64-mingw32-gcc -c mylib.c -o mylib.x86.o
```

### Step 4: Package as ZIP

```bash
# x64 library
zip mylib.x64.zip mylib.x64.o mylib.h

# x86 library
zip mylib.x86.zip mylib.x86.o mylib.h
```

## Using Shared Libraries

### Merge into Capability

```
load "capability.x64.o"
  mergelib "mylib.x64.zip"
  make pic
```

### Multiple Libraries

```
load "capability.x64.o"
  mergelib "libtcg.x64.zip"
  mergelib "libcrypto.x64.zip"
  mergelib "libnet.x64.zip"
  make pic +optimize
```

### Link-Time Optimization

**Without +optimize**: Entire library included
```
mylib.x64.zip: 500KB
Capability uses 2 functions
Result: 500KB (100% library)
```

**With +optimize**: Only used functions included
```
mylib.x64.zip: 500KB
Capability uses 2 functions
Result: 50KB (10% library)
```

## PIC Compatibility Constraints

### Critical Rule: No Read/Write Globals

**❌ NOT PIC-Compatible**:
```c
// This breaks PIC for shared libraries
int global_counter = 0;

void increment(void) {
    global_counter++;  // Read/write global!
}
```

**✅ PIC-Compatible**:
```c
// Pass state as parameter
void increment(int* counter) {
    (*counter)++;
}

// Caller (with fixbss) can have globals
int global_counter = 0;  // OK in main code with fixbss

void use_library(void) {
    increment(&global_counter);  // Pass pointer
}
```

### Why This Constraint?

- **Shared library code** must work in any PIC/PICO context
- **PICOs support globals** (each instance has own copy)
- **PIC for shared libs** cannot guarantee .bss fix
- **Solution**: Pass pointers or use caller's storage

### What IS Allowed

✅ **Read-only data**:
```c
const char* messages[] = {
    "Error",
    "Success"
};
```

✅ **Local variables**:
```c
void function(void) {
    int local_var = 42;  // On stack, always OK
}
```

✅ **Function-local statics** (read-only):
```c
const char* get_version(void) {
    static const char version[] = "1.0";
    return version;
}
```

## Code Reuse Patterns

### Pattern 1: Utility Functions

```c
// lib_utils.c
int string_compare(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a - *b;
}

void* mem_copy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}
```

### Pattern 2: API Resolution

```c
// lib_resolve.c
HMODULE get_kernel32(void) {
    // Walk PEB to find kernel32
    // ...
}

FARPROC resolve_by_hash(DWORD module_hash, DWORD func_hash) {
    // Hash-based resolution
    // ...
}
```

### Pattern 3: Crypto Functions

```c
// lib_crypto.c
void xor_encrypt(void* data, size_t len, const char* key, size_t keylen) {
    char* d = (char*)data;
    for (size_t i = 0; i < len; i++) {
        d[i] ^= key[i % keylen];
    }
}

void aes_encrypt(void* data, size_t len, const unsigned char* key) {
    // AES implementation
    // ...
}
```

### Pattern 4: Network Helpers

```c
// lib_network.c
int tcp_connect(const char* host, unsigned short port) {
    // Socket creation and connection
    // ...
}

int http_get(const char* url, char* buffer, size_t buflen) {
    // HTTP GET request
    // ...
}
```

## Function Disco with Libraries

```
make pic +optimize +disco
```

**Effect**: Interweaves library and capability functions

**Without disco**:
```
[Capability functions...]
[Library functions...]
```

**With disco**:
```
[go()]              # Entry point always first
[cap_func1]
[lib_func3]
[cap_func2]
[lib_func1]
[cap_func3]
[lib_func8]
```

**Benefit**: Breaks up predictable library patterns for signature evasion

## Building a Library Ecosystem

### Standard Libraries

```
libtcg.zip       - Tradecraft Garden core library
libcrypto.zip    - Cryptography functions
libnet.zip       - Network operations
libwin32.zip     - Windows API helpers
libpico.zip      - PICO execution framework
```

### Project-Specific Libraries

```
myproject/
  └── lib/
      ├── libcore.zip      - Project core utilities
      ├── libhttp.zip      - HTTP-specific code
      └── libpersist.zip   - Persistence mechanisms
```

### Using Multiple Libraries

```
load "capability.x64.o"
  mergelib "lib/libtcg.x64.zip"
  mergelib "lib/libcrypto.x64.zip"
  mergelib "lib/libnet.x64.zip"
  mergelib "myproject/lib/libcore.x64.zip"
  make pic +optimize +disco +mutate
```

## Library Best Practices

### 1. Clear API Surface

```c
// mylib.h - Public API
void public_function1(void);
void public_function2(void);

// mylib.c - Internal helpers
static void internal_helper(void) {
    // Not exposed
}
```

### 2. Minimal Dependencies

```c
// ✅ Good - self-contained
int my_strlen(const char* str) {
    // Doesn't depend on other functions
}

// ❌ Bad - complex dependency chain
void complex_function(void) {
    helper1();  // which calls helper2()
    helper2();  // which calls helper3()
    helper3();  // which calls helper4()
}
```

### 3. Documentation

```c
/**
 * Resolves a Windows API function by hash
 *
 * @param module_hash ROR13 hash of module name (e.g., "KERNEL32.DLL")
 * @param func_hash ROR13 hash of function name
 * @return Function pointer or NULL if not found
 */
FARPROC resolve_by_hash(DWORD module_hash, DWORD func_hash);
```

### 4. Error Handling

```c
void* my_malloc(size_t size) {
    void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

    if (!ptr) {
        // Graceful failure
        return NULL;
    }

    return ptr;
}
```

### 5. Architecture Abstraction

```c
// lib_arch.h
#ifdef _WIN64
    #define ARCH_PTR_SIZE 8
    #define ARCH_PAGE_SIZE 0x1000
#else
    #define ARCH_PTR_SIZE 4
    #define ARCH_PAGE_SIZE 0x1000
#endif

// Use in library code
void* allocate_page(void) {
    return VirtualAlloc(NULL, ARCH_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
}
```

## Size Optimization

### Before Optimization

```
Capability: 10KB
Library (full): 500KB
Total: 510KB
```

### After Link-Time Optimization

```
load "capability.x64.o"
  mergelib "huge_library.x64.zip"
  make pic +optimize

Result:
Capability: 10KB
Library (used functions only): 25KB
Total: 35KB (93% reduction)
```

### Optimization Strategy

```
# Maximize size reduction
make pic +optimize

# Add mutation without sacrificing optimization
make pic +optimize +mutate

# Full transformation pipeline
make pic +optimize +disco +mutate
```

## Versioning and Compatibility

### Version in Header

```c
// mylib.h
#define MYLIB_VERSION_MAJOR 1
#define MYLIB_VERSION_MINOR 2
#define MYLIB_VERSION_PATCH 0

#define MYLIB_VERSION "1.2.0"
```

### Compatibility Checks

```c
// In capability code
#if MYLIB_VERSION_MAJOR != 1
    #error "Incompatible library version"
#endif
```

## Related Techniques

- [Crystal Palace](../crystal-palace/) - Library merging
- [Position Independent Code](../position-independent-code/) - PIC constraints
- [Binary Transformation](../binary-transformation/) - Library optimization

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [POC](./POC/) for library examples
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational use
