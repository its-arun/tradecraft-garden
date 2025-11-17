# Dynamic Function Resolution (DFR)

## Overview

Dynamic Function Resolution is the technique of resolving Windows API function addresses at runtime without using import tables. This is fundamental to position-independent code and in-memory evasion.

## The Import Problem

Traditional Windows programs use Import Address Tables (IAT):

```c
#include <windows.h>

// This uses IAT - absolute addresses
VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
```

**Problem**: IAT contains absolute addresses that break PIC and expose API usage to analysts.

## DFR Solution

Resolve function addresses dynamically at runtime:

```c
// Get module handle
HMODULE hKernel32 = /* resolve kernel32.dll */;

// Get function address
FARPROC pVirtualAlloc = GetProcAddress(hKernel32, "VirtualAlloc");

// Call function
((LPVOID (WINAPI*)(LPVOID,SIZE_T,DWORD,DWORD))pVirtualAlloc)(
    NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE
);
```

## DFR Methods

### Method 1: String-Based Resolution

**Simplest approach** - pass module and function names as strings

```c
FARPROC resolve(const char* module, const char* function) {
    HMODULE hModule = LoadLibraryA(module);
    return GetProcAddress(hModule, function);
}

// Usage
void* pVA = resolve("kernel32.dll", "VirtualAlloc");
```

**Pros**:
- Simple to implement
- Easy to debug
- Can load new modules

**Cons**:
- Strings visible in binary
- Larger code size
- Slower (string comparisons)

### Method 2: Hash-Based Resolution (ROR13)

**Most common approach** - use hashes instead of strings

```c
#define HASH_KERNEL32     0x6A4ABC5B
#define HASH_VIRTUALALLOC 0x91AFCA54

FARPROC resolve_hash(DWORD module_hash, DWORD function_hash) {
    // Find module by hash
    HMODULE hModule = find_module_by_hash(module_hash);

    // Find function by hash
    return find_function_by_hash(hModule, function_hash);
}
```

**Pros**:
- No strings in binary
- Smaller code size
- Faster (integer comparisons)
- Signature evasion

**Cons**:
- More complex
- Can't load new modules easily
- Debugging harder

### Method 3: Ordinal-Based Resolution

**Use function ordinals** instead of names

```c
FARPROC resolve_ordinal(const char* module, WORD ordinal) {
    HMODULE hModule = LoadLibraryA(module);
    return GetProcAddress(hModule, (LPCSTR)ordinal);
}

// VirtualAlloc is ordinal 1234 (example)
void* pVA = resolve_ordinal("kernel32.dll", 1234);
```

**Pros**:
- Smaller than strings
- Fast resolution

**Cons**:
- Ordinals change between Windows versions
- Not all functions exported by ordinal

## ROR13 Hash Algorithm

The most popular hashing algorithm for DFR:

```c
DWORD ror13_hash(const char* str) {
    DWORD hash = 0;

    while (*str) {
        hash = (hash >> 13) | (hash << (32 - 13));  // Rotate right 13
        hash += (DWORD)(*str);                       // Add character
        str++;
    }

    return hash;
}

// Pre-compute at build time
#define HASH_KERNEL32 ror13_hash("KERNEL32.DLL")
```

**Why ROR13?**
- Simple implementation
- Good distribution
- Fast computation
- Industry standard (from Metasploit)

## Finding Module Base Address

### Method 1: Walk PEB (Process Environment Block)

```c
HMODULE get_module_by_name(const char* name) {
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    // Get loader data
    PPEB_LDR_DATA pLdr = pPeb->Ldr;

    // Walk InMemoryOrderModuleList
    PLIST_ENTRY pList = pLdr->InMemoryOrderModuleList.Flink;

    while (pList != &pLdr->InMemoryOrderModuleList) {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(
            pList,
            LDR_DATA_TABLE_ENTRY,
            InMemoryOrderLinks
        );

        // Compare module name
        if (compare_unicode_string(&pEntry->BaseDllName, name)) {
            return (HMODULE)pEntry->DllBase;
        }

        pList = pList->Flink;
    }

    return NULL;
}
```

### Method 2: Walk PEB by Hash

```c
HMODULE find_module_by_hash(DWORD hash) {
    PPEB pPeb = get_peb();
    PLIST_ENTRY pList = pPeb->Ldr->InMemoryOrderModuleList.Flink;

    while (pList != &pPeb->Ldr->InMemoryOrderModuleList) {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(
            pList,
            LDR_DATA_TABLE_ENTRY,
            InMemoryOrderLinks
        );

        // Hash the module name
        DWORD module_hash = unicode_ror13_hash(&pEntry->BaseDllName);

        if (module_hash == hash) {
            return (HMODULE)pEntry->DllBase;
        }

        pList = pList->Flink;
    }

    return NULL;
}
```

## Walking Export Address Table (EAT)

```c
FARPROC find_function_by_hash(HMODULE hModule, DWORD function_hash) {
    // Get DOS header
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;

    // Get NT headers
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + pDosHeader->e_lfanew
    );

    // Get export directory
    DWORD export_rva = pNtHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT
    ].VirtualAddress;

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(
        (BYTE*)hModule + export_rva
    );

    // Get export tables
    DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
    DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    // Search for function
    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        char* funcName = (char*)((BYTE*)hModule + pNames[i]);
        DWORD hash = ror13_hash(funcName);

        if (hash == function_hash) {
            // Return function address
            return (FARPROC)((BYTE*)hModule + pFunctions[pOrdinals[i]]);
        }
    }

    return NULL;
}
```

## Crystal Palace DFR

### Basic DFR

```
dfr "resolve" "ror13"
```

**What it does**:
- Scans code for `__imp_MODULE$Function` references
- Inserts calls to `resolve(module_hash, function_hash)`
- Computes ROR13 hashes at build time

**Your resolver**:
```c
void* resolve(DWORD module_hash, DWORD function_hash) {
    HMODULE hModule = find_module_by_hash(module_hash);
    return find_function_by_hash(hModule, function_hash);
}
```

### String-Based DFR

```
dfr "resolve" "strings"
```

**What it does**:
- Inserts calls to `resolve("MODULE", "Function")`
- Passes string pointers

**Your resolver**:
```c
void* resolve(const char* module, const char* function) {
    HMODULE hModule = LoadLibraryA(module);
    return GetProcAddress(hModule, function);
}
```

### Multi-Resolver Pattern

```
dfr "resolve_core" "ror13" "KERNEL32, NTDLL"
dfr "resolve_ext" "strings"
```

**Why**:
- Core APIs (KERNEL32, NTDLL) via fast hash resolution
- Extended APIs via string resolution (can load new DLLs)
- `resolve_ext` can use KERNEL32 APIs (already resolved by `resolve_core`)

**Implementation**:
```c
// Fast resolver for core modules
void* resolve_core(DWORD module_hash, DWORD function_hash) {
    // Only handles KERNEL32 and NTDLL
    HMODULE hModule = find_module_by_hash(module_hash);
    return find_function_by_hash(hModule, function_hash);
}

// Fallback for other modules
void* resolve_ext(const char* module, const char* function) {
    // Can safely use KERNEL32 APIs (resolved by resolve_core)
    HMODULE hModule = KERNEL32$GetModuleHandleA(module);

    if (!hModule) {
        hModule = KERNEL32$LoadLibraryA(module);
    }

    return (void*)KERNEL32$GetProcAddress(hModule, function);
}
```

## DFR Code Patterns

### Pattern 1: Bootstrap Resolver

```c
// Minimal resolver to get started
FARPROC bootstrap_resolve(DWORD hash) {
    HMODULE hKernel32 = get_kernel32_from_peb();
    return find_function_by_hash(hKernel32, hash);
}

// Get initial APIs
pLoadLibraryA LoadLibraryA = bootstrap_resolve(HASH_LOADLIBRARYA);
pGetProcAddress GetProcAddress = bootstrap_resolve(HASH_GETPROCADDRESS);

// Now can resolve anything
```

### Pattern 2: Cached Resolution

```c
typedef struct {
    DWORD hash;
    FARPROC address;
} RESOLVED_FUNCTION;

RESOLVED_FUNCTION cache[256];
int cache_count = 0;

FARPROC resolve_cached(DWORD module_hash, DWORD function_hash) {
    // Check cache
    for (int i = 0; i < cache_count; i++) {
        if (cache[i].hash == function_hash) {
            return cache[i].address;
        }
    }

    // Resolve
    FARPROC addr = resolve(module_hash, function_hash);

    // Cache
    if (cache_count < 256) {
        cache[cache_count].hash = function_hash;
        cache[cache_count].address = addr;
        cache_count++;
    }

    return addr;
}
```

### Pattern 3: Lazy Resolution

```c
// Global function pointers (initially NULL)
pVirtualAlloc g_pVirtualAlloc = NULL;
pCreateThread g_pCreateThread = NULL;

void ensure_resolved(void) {
    if (!g_pVirtualAlloc) {
        g_pVirtualAlloc = resolve(HASH_KERNEL32, HASH_VIRTUALALLOC);
        g_pCreateThread = resolve(HASH_KERNEL32, HASH_CREATETHREAD);
    }
}

void use_apis(void) {
    ensure_resolved();
    g_pVirtualAlloc(...);
    g_pCreateThread(...);
}
```

## DFR Security Considerations

### 1. Module Not Loaded

```c
HMODULE hModule = find_module_by_hash(hash);
if (!hModule) {
    // Option 1: Load the module
    hModule = LoadLibraryA("module.dll");

    // Option 2: Fail gracefully
    return NULL;
}
```

### 2. Function Not Found

```c
FARPROC func = find_function_by_hash(hModule, hash);
if (!func) {
    // Windows version mismatch?
    // Try alternative API
    // Or fail gracefully
    return NULL;
}
```

### 3. Hash Collisions

ROR13 has potential for collisions. Mitigations:
- Use full module path in hash
- Verify function signature
- Use alternative hash algorithm (CRC32, DJB2)

## DFR Best Practices

1. ✅ Use hash-based resolution for OPSEC
2. ✅ Cache frequently-used functions
3. ✅ Handle resolution failures gracefully
4. ✅ Use multi-resolver for flexibility
5. ✅ Pre-compute hashes at build time
6. ❌ Don't resolve in tight loops (performance)
7. ❌ Don't expose clear strings in binary

## Related Techniques

- [Position Independent Code](../position-independent-code/) - DFR is essential for PIC
- [Crystal Palace](../crystal-palace/) - Automated DFR insertion
- [Binary Transformation](../binary-transformation/) - DFR transformations

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [POC](./POC/) for working implementations
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational use
