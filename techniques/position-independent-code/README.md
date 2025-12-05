# Position Independent Code (PIC)

## Overview

Position Independent Code is code that executes correctly regardless of where it's loaded in memory. This is essential for reflective DLL loading, shellcode, and in-memory evasion techniques used in red team operations.

## Why PIC Matters for Red Teams

Traditional executables rely on fixed memory addresses. They use:
- Import Address Tables (IAT) at known offsets
- Global variables at fixed addresses
- String constants at predictable locations

**The Problem**: When loading code into arbitrary memory locations (reflective loading, process injection, shellcode execution), these assumptions break.

**The Solution**: Position Independent Code adapts to any memory location.

## Core Concepts

### 1. No Absolute Addresses
❌ **Bad** (Position Dependent):
```c
char* message = "Hello";  // Absolute address to string
void* ptr = &message;      // Absolute address to variable
```

✅ **Good** (Position Independent):
```c
// Calculate addresses relative to current position
char* get_message(void) {
    static const char msg[] = "Hello";
    return (char*)msg;  // Relative addressing
}
```

### 2. Dynamic API Resolution
Traditional programs import functions via IAT:
```c
#include <windows.h>
VirtualAlloc(...);  // Uses IAT - not position independent
```

PIC must resolve APIs at runtime:
```c
// Get function pointer dynamically
FARPROC pVirtualAlloc = GetProcAddress(GetModuleHandle("kernel32"), "VirtualAlloc");
((void*(WINAPI*)(void*,size_t,DWORD,DWORD))pVirtualAlloc)(...);
```

### 3. Relative Data Access
**x64 Advantage**: RIP-relative addressing
```assembly
; x64 can access data relative to instruction pointer
lea rax, [rip + offset_to_data]
```

**x86 Challenge**: No RIP-relative addressing
```assembly
; x86 requires tricks to get current EIP
call get_eip
get_eip:
  pop eax          ; EAX now contains current EIP
  add eax, offset  ; Calculate data address
```

## Architecture Differences

### x64 PIC (Easier)

**Advantages**:
- RIP-relative addressing built-in
- Modern compilers generate PIC by default (with flags)
- Easier to access data sections

**Compiler Flags**:
```bash
gcc -fPIC -c code.c -o code.o
```

### x86 PIC (Harder)

**Challenges**:
- No RIP-relative addressing
- Must manually calculate addresses
- Requires position-independent data access patterns

**Crystal Palace Solution**: `fixptrs` command handles this automatically

## Building PIC

### Method 1: Compiler Flags

```bash
# Generate position-independent object file
gcc -c -fPIC -fno-stack-protector -nostdlib code.c -o code.o

# Extract .text section as raw shellcode
objcopy -O binary --only-section=.text code.o code.bin
```

### Method 2: Crystal Palace

```
load "code.x64.o"
  dfr "resolve" "ror13"
  make pic +optimize
link "output.bin"
```

**Benefits**:
- Automatic DFR insertion
- Resource linking
- Binary transformations
- Cross-architecture support

## Common PIC Patterns

### Pattern 1: Manual API Resolution

```c
typedef HMODULE (WINAPI *pLoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *pGetProcAddress)(HMODULE, LPCSTR);

// Bootstrap: Get kernel32 base from PEB
HMODULE get_kernel32(void) {
    // Walk Process Environment Block
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    // Walk loaded module list
    PLIST_ENTRY pListHead = &pPeb->Ldr->InMemoryOrderModuleList;
    PLIST_ENTRY pListEntry = pListHead->Flink;

    // Second entry is usually kernel32
    pListEntry = pListEntry->Flink;
    PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

    return (HMODULE)pEntry->DllBase;
}

// Get function address from export table
FARPROC get_function(HMODULE hModule, const char* name) {
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule +
        pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
    DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        char* funcName = (char*)((BYTE*)hModule + pNames[i]);
        if (strcmp(funcName, name) == 0) {
            return (FARPROC)((BYTE*)hModule + pFunctions[pOrdinals[i]]);
        }
    }
    return NULL;
}
```

### Pattern 2: Hash-Based Resolution (ROR13)

```c
// ROR13 hash algorithm
DWORD ror13_hash(const char* str) {
    DWORD hash = 0;
    while (*str) {
        hash = (hash >> 13) | (hash << (32 - 13));
        hash += *str;
        str++;
    }
    return hash;
}

// Pre-computed hashes (at build time)
#define HASH_KERNEL32     0x6A4ABC5B
#define HASH_VIRTUALALLOC 0x91AFCA54

FARPROC resolve_by_hash(DWORD moduleHash, DWORD functionHash) {
    // Walk PEB to find module with matching hash
    // Walk EAT to find function with matching hash
    // Return function pointer
}
```

### Pattern 3: Global Variable Access (with fixbss)

```c
// .bss section (uninitialized globals)
int global_counter;
char buffer[1024];

// Access function (Crystal Palace injects this)
void* _bss_fix(void) {
    static void* bss_base = NULL;
    if (bss_base) return bss_base;

    // Find slack space in loaded DLL
    HMODULE hNtdll = /* get ntdll */;
    bss_base = find_slack_in_module(hNtdll);
    return bss_base;
}

// Crystal Palace rewrites global access to use _bss_fix
```

## PIC Challenges & Solutions

| Challenge | Problem | Solution |
|-----------|---------|----------|
| **API Resolution** | No import table | Manual EAT walking or hash-based resolution |
| **Global Variables** | No fixed .data/.bss | fixbss or pass pointers |
| **String Constants** | Absolute addresses | Embedding in code or RIP-relative |
| **x86 Data Access** | No RIP-relative | fixptrs transformation |
| **C Runtime** | Depends on imports | Mini CRT or avoid CRT functions |
| **Thread-Local Storage** | Uses absolute addresses | Avoid TLS or custom implementation |

## Crystal Palace PIC Features

### 1. Dynamic Function Resolution (DFR)
```
dfr "resolve" "ror13"
```
Automatically converts API calls to hash-based resolution.

### 2. Pointer Fixing (x86)
```
fixptrs "_caller"
```
Transforms x86 absolute addressing to position-independent.

### 3. BSS Fixing
```
fixbss "_bss_fix"
```
Enables global variables in PIC.

### 4. Entry Point Control
```
make pic +gofirst
```
Positions entry point at offset 0.

## Testing PIC

### Verify Position Independence

```c
// Test harness
void test_pic(void) {
    // Allocate two different memory regions
    void* region1 = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    void* region2 = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    // Copy PIC to both regions
    memcpy(region1, pic_code, pic_size);
    memcpy(region2, pic_code, pic_size);

    // Execute from both locations - both should work
    ((void(*)())region1)();
    ((void(*)())region2)();
}
```

If code works from both locations, it's position-independent.

## Real-World Applications

1. **Reflective DLL Injection** - Load DLLs without LoadLibrary
2. **Shellcode** - Executable code in arbitrary memory
3. **Process Hollowing** - Replace process memory with PIC payload
4. **Module Stomping** - Overwrite loaded module with PIC
5. **In-Memory Evasion** - Execute without touching disk

## Best Practices

1. ✅ Avoid global variables (or use fixbss)
2. ✅ Use hash-based API resolution for smaller code
3. ✅ Embed strings in code sections when possible
4. ✅ Test at multiple memory locations
5. ✅ Minimize C runtime dependencies
6. ❌ Don't use static constructors (C++)
7. ❌ Don't use exceptions (C++)
8. ❌ Don't rely on import tables

## Next Steps

- Read [NOTES.md](./NOTES.md) for quick reference
- Study [DETAILED_EXPLANATION.md](./DETAILED_EXPLANATION.md) for deep dive
- Explore [POC](./POC/) for code examples
- Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational guidance

## Related Techniques

- [Crystal Palace](../crystal-palace/) - Automated PIC building
- [Dynamic Function Resolution](../dynamic-function-resolution/) - API resolution techniques
- [PICO](../pico/) - Position Independent Capability Objects
