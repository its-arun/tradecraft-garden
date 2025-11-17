# Position Independent Code - Quick Notes

## Core Principle
Code that runs correctly regardless of memory location.

## Why PIC?
- Reflective loading
- Shellcode execution
- Process injection
- In-memory evasion
- No disk artifacts

## Key Requirements

### 1. No Absolute Addresses
```c
// ❌ Position Dependent
char* msg = "Hello";
void* ptr = 0x00401000;

// ✅ Position Independent
char msg[] = "Hello";  // Embedded in code
void* ptr = get_address_dynamically();
```

### 2. Runtime API Resolution
```c
// ❌ Position Dependent
#include <windows.h>
VirtualAlloc(...);  // Uses IAT

// ✅ Position Independent
FARPROC pVA = GetProcAddress(GetModuleHandle("kernel32"), "VirtualAlloc");
```

### 3. Relative Data Access
```assembly
; x64: RIP-relative (automatic)
lea rax, [rip + data]

; x86: Manual EIP calculation
call next
next: pop eax
add eax, offset_to_data
```

## Building PIC

### Compiler Method
```bash
# x64
gcc -c -fPIC -nostdlib -fno-stack-protector code.c -o code.o

# x86
gcc -m32 -c -fPIC -nostdlib -fno-stack-protector code.c -o code.o

# Extract .text
objcopy -O binary --only-section=.text code.o shellcode.bin
```

### Crystal Palace Method
```
load "code.x64.o"
  dfr "resolve" "ror13"
  fixbss "_bss_fix"
  make pic +optimize
link "output.bin"
```

## API Resolution Patterns

### Manual EAT Walking
```c
HMODULE get_kernel32_from_peb();
FARPROC get_proc_address(HMODULE h, const char* name);
```

### Hash-Based (ROR13)
```c
#define HASH_KERNEL32 0x6A4ABC5B
#define HASH_NTDLL    0x3CFA685D

FARPROC resolve_by_hash(DWORD moduleHash, DWORD funcHash);
```

### String-Based
```c
FARPROC resolve(const char* module, const char* function) {
    HMODULE h = LoadLibraryA(module);
    return GetProcAddress(h, function);
}
```

## x64 vs x86

### x64 Advantages
- RIP-relative addressing (built-in)
- Easier data access
- Modern compilers support PIC by default

### x86 Challenges
- No RIP-relative addressing
- Manual EIP tricks needed
- Crystal Palace `fixptrs` solves this

## PIC Entry Point

Standard convention:
```c
void go(void) {
    // Entry point at offset 0
    // No parameters, no return value
}
```

Crystal Palace:
```
make pic +gofirst  # Ensures go() is first
```

## Global Variables in PIC

### Problem
```c
int counter = 0;  // Where is this in memory?
```

### Solution 1: Avoid Globals
```c
void function(int* counter) {
    (*counter)++;
}
```

### Solution 2: Crystal Palace fixbss
```c
// Provide BSS fixing function
void* _bss_fix(void) {
    return find_rw_memory();  // Slack space, heap, etc.
}
```

Specification:
```
fixbss "_bss_fix"
```

## Common Patterns

### Pattern: Minimal Shellcode
```c
void go(void) {
    // Resolve APIs manually
    HMODULE k32 = get_kernel32();
    pVirtualAlloc VA = get_proc(k32, "VirtualAlloc");
    pCreateThread CT = get_proc(k32, "CreateThread");

    // Allocate memory, spawn thread, etc.
}
```

### Pattern: Resource Access
```c
// With Crystal Palace resource linking
extern unsigned char _binary_payload_bin_start[];
extern unsigned int  _binary_payload_bin_size;

void go(void) {
    execute(_binary_payload_bin_start);
}
```

### Pattern: Multi-Stage
```c
void go(void) {
    // Stage 1: Decrypt stage 2
    decrypt(stage2_data, stage2_size);

    // Stage 2: Execute
    void (*stage2)() = (void(*)())stage2_data;
    stage2();
}
```

## Testing PIC

### Relocation Test
```c
// Load at different addresses
void* addr1 = VirtualAlloc((void*)0x10000000, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
void* addr2 = VirtualAlloc((void*)0x20000000, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

memcpy(addr1, pic, size);
memcpy(addr2, pic, size);

// Both should execute successfully
((void(*)())addr1)();
((void(*)())addr2)();
```

### Import Check
```bash
# Should have no imports
dumpbin /imports code.obj
# Output: no imports found
```

## Debugging PIC

### Add Debug Logging
```c
void debug_log(const char* msg) {
    // Write to file, console, or debugger
}

void go(void) {
    debug_log("Entry point");
    // ...
}
```

### Use Debugger
```bash
# x64dbg, WinDbg, gdb
# Set breakpoint at entry point
# Step through execution
# Watch for crashes on data access
```

## Common Pitfalls

### ❌ Using C Runtime Functions
```c
printf("Hello");  // Needs imports
malloc(100);      // Needs imports
```

### ✅ Avoid or Implement Manually
```c
// Use Windows APIs directly
WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "Hello", 5, &written, NULL);

// Or provide mini CRT
void* my_malloc(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}
```

### ❌ Static String Arrays
```c
const char* messages[] = {  // Array of pointers - absolute addresses!
    "Message 1",
    "Message 2"
};
```

### ✅ Embedded Strings
```c
const char messages[][32] = {  // Embedded strings - relative!
    "Message 1",
    "Message 2"
};
```

### ❌ Function Pointers to Imports
```c
void (*func)() = GetProcAddress(...);  // Global var - absolute address!
```

### ✅ Resolve on Use
```c
void use_func(void) {
    void (*func)() = GetProcAddress(...);  // Local - on stack
    func();
}
```

## PIC Flags Summary

### GCC/Clang
```bash
-fPIC              # Generate position-independent code
-fno-stack-protector  # Disable stack canaries (needs imports)
-nostdlib          # No standard library
-fno-builtin       # No builtin functions
-masm=intel        # Intel syntax (optional)
```

### MSVC
```bash
/c                 # Compile only
/GS-               # Disable security checks
/Oi-               # Disable intrinsics
```

## Crystal Palace PIC Commands

```
dfr "resolver" "method"           # Dynamic function resolution
fixptrs "_caller"                 # Fix x86 pointers
fixbss "_bss_fix"                 # Enable globals
make pic                          # Generate x86 PIC
make pic64                        # Generate x64 PIC
make pic +optimize                # Remove unused code
make pic +mutate                  # Signature evasion
make pic +disco                   # Randomize functions
make pic +gofirst                 # Entry point first
```

## Size Optimization

### Minimize Size
```c
// Avoid large string constants
// Avoid redundant code
// Use Crystal Palace +optimize
// Strip debug info
```

### Typical Sizes
- Minimal shellcode: 300-500 bytes
- Simple loader: 1-3 KB
- Full capability: 10-50 KB

## OPSEC Considerations

### Good Practices
- Use hash-based resolution (smaller, no strings)
- Embed strings in code (encrypted if sensitive)
- Use +mutate for unique signatures
- Minimize API calls (fewer indicators)

### Avoid
- Suspicious API combinations (VirtualAlloc + WriteProcessMemory + CreateRemoteThread)
- Hardcoded IPs/domains
- Debug strings
- Predictable patterns

## Integration Patterns

### With Loaders
```c
// Loader allocates memory
void* mem = VirtualAlloc(NULL, pic_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

// Copy PIC
memcpy(mem, pic_data, pic_size);

// Execute
((void(*)())mem)();
```

### With Injection
```c
// Inject PIC into remote process
void* remote_mem = VirtualAllocEx(hProcess, NULL, pic_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
WriteProcessMemory(hProcess, remote_mem, pic_data, pic_size, NULL);
CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)remote_mem, NULL, 0, NULL);
```

## Quick Reference: API Resolution

### Get Kernel32
```c
#ifdef _WIN64
PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

// Walk InMemoryOrderModuleList
// Find kernel32.dll
```

### Walk EAT
```c
PIMAGE_EXPORT_DIRECTORY pExport = /* from PE headers */;
DWORD* pNames = (DWORD*)((BYTE*)hModule + pExport->AddressOfNames);
DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExport->AddressOfFunctions);
WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExport->AddressOfNameOrdinals);

// Compare function names or hashes
// Return function address
```

## Tags
`#PIC` `#shellcode` `#reflective-loading` `#position-independent` `#in-memory` `#evasion`
