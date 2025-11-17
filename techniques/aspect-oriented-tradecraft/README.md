# Aspect-Oriented Programming for Tradecraft

## Overview

Aspect-Oriented Programming (AOP) is a programming paradigm that separates cross-cutting concerns from core logic. Crystal Palace applies AOP principles to offensive tradecraft, allowing you to separate evasion logic from capability logic through binary instrumentation.

## Core AOP Concepts

### Join Point
A point in program execution where instrumentation can occur
- Win32 API calls
- Local function calls
- Function entry/exit

### Advice
Code that implements cross-cutting functionality
- Logging
- Encryption
- Anti-analysis
- EDR evasion

### Pointcut
Specification of which Join Points receive which Advice
- "All VirtualAlloc calls"
- "All network sends"
- "All file operations"

### Weaving
Process of adding Aspects to the program
- Crystal Palace uses binary transformation
- Happens at link-time
- No source code changes needed

## Why AOP for Tradecraft?

### Traditional Approach (Monolithic)

```c
void my_capability(void) {
    // Mix of capability AND evasion logic
    if (check_debugger()) return;  // Evasion
    encrypt_traffic();             // Evasion
    do_actual_work();              // Capability
    obfuscate_memory();            // Evasion
}
```

**Problems**:
- Tightly coupled code
- Hard to reuse evasion logic
- Testing difficult
- Changes require recompilation

### AOP Approach (Separated)

```c
// Capability code (clean, focused)
void my_capability(void) {
    do_actual_work();
}

// Evasion aspects (separate, reusable)
attach "KERNEL32$CreateFileA" "_StealthCreateFile"
attach "WS2_32$send" "_EncryptSend"
```

**Benefits**:
- Clean separation
- Reusable aspects
- Easy to swap evasion strategies
- No capability code changes

## Crystal Palace AOP Commands

### attach: Hook Win32 APIs

**Syntax**:
```
attach "MODULE$Function" "_HookFunction"
```

**Example**:
```
attach "KERNEL32$VirtualAlloc" "_LogVirtualAlloc"
attach "WS2_32$send" "_EncryptSend"
```

**What happens**:
1. Crystal Palace finds all `KERNEL32$VirtualAlloc` calls
2. Rewrites them to call `_LogVirtualAlloc` instead
3. Your hook can call original API if needed

**Hook implementation**:
```c
LPVOID WINAPI _LogVirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
) {
    // Log the allocation
    log("VirtualAlloc: size=%lu, protect=%lu", dwSize, flProtect);

    // Call original API
    return KERNEL32$VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}
```

### redirect: Hook Local Functions

**Syntax**:
```
redirect "function_name" "_hook_function"
```

**Example**:
```
redirect "parse_command" "_audit_command"
```

**Hook implementation**:
```c
void parse_command(char* cmd) {
    // Original implementation
    execute(cmd);
}

void _audit_command(char* cmd) {
    // Audit before executing
    log_command(cmd);

    // Original functionality
    execute(cmd);
}
```

### Hook Stacking

Multiple hooks execute in order:

```
attach "KERNEL32$CreateFileA" "_Hook1"
attach "KERNEL32$CreateFileA" "_Hook2"
attach "KERNEL32$CreateFileA" "_Hook3"
```

**Execution order**:
```
Capability → _Hook1 → _Hook2 → _Hook3 → Real API
```

This implements the **Chain of Responsibility** pattern.

## Hook Protection Mechanisms

### protect: Complete Isolation

**Syntax**:
```
protect "function1, function2, function3"
```

**Effect**: No hooks apply within listed functions

**Use case**: Performance-critical code

```c
void performance_critical_loop(void) {
    // Direct API calls, no hook overhead
    for (int i = 0; i < 1000000; i++) {
        KERNEL32$VirtualAlloc(...);
    }
}
```

```
protect "performance_critical_loop"
```

### preserve: Selective Protection

**Syntax**:
```
preserve "target|MODULE$Function" "function1, function2"
```

**Effect**: Within function1 and function2, calls to MODULE$Function aren't hooked

**Use case**: Prevent recursion in hooks

```c
HANDLE WINAPI _LogCreateFile(...) {
    // This call should NOT be hooked (infinite recursion!)
    log_to_file("CreateFile called");

    // Call real API
    return KERNEL32$CreateFileA(...);
}
```

```
attach "KERNEL32$CreateFileA" "_LogCreateFile"
preserve "KERNEL32$CreateFileA" "_LogCreateFile"
```

### optout: Fine-Grained Control

**Syntax**:
```
optout "function" "hook1, hook2, hook3"
```

**Effect**: Within function, specified hooks don't apply (others do)

**Use case**: Selective hook application

```
optout "sensitive_function" "_DebugLogging, _Telemetry"
```

## IAT Hooking

For capabilities with import tables:

### addhook: Register IAT Hook

**Syntax**:
```
addhook "MODULE$Function" "hook_function"
```

**Example**:
```
addhook "KERNEL32$CreateProcessA" "_HookCreateProcess"
```

### __resolve_hook: Runtime Resolution

```c
// Linker intrinsic
FARPROC __resolve_hook(DWORD functionHash);

// Usage
DWORD hash = ror13_hash("CreateProcessA");
pCreateProcess = (FARPROC)__resolve_hook(hash);
```

### filterhooks: Optimization

**Syntax**:
```
filterhooks $DLL
```

**Effect**: Removes hooks not needed for this DLL

**Benefit**: Smaller size, link-time optimization removes unused hook code

## Practical AOP Examples

### Example 1: API Logging

**Goal**: Log all file operations

```
attach "KERNEL32$CreateFileA" "_LogCreateFile"
attach "KERNEL32$ReadFile" "_LogReadFile"
attach "KERNEL32$WriteFile" "_LogWriteFile"
```

```c
HANDLE WINAPI _LogCreateFile(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
) {
    log("CreateFile: %s", lpFileName);

    return KERNEL32$CreateFileA(
        lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile
    );
}
```

### Example 2: Network Encryption

**Goal**: Encrypt all network traffic

```
attach "WS2_32$send" "_EncryptSend"
attach "WS2_32$recv" "_DecryptRecv"
```

```c
int WINAPI _EncryptSend(SOCKET s, const char* buf, int len, int flags) {
    // Encrypt buffer
    char* encrypted = malloc(len + 16);
    int enc_len = aes_encrypt(buf, len, encrypted);

    // Send encrypted
    int result = WS2_32$send(s, encrypted, enc_len, flags);

    free(encrypted);
    return result;
}

int WINAPI _DecryptRecv(SOCKET s, char* buf, int len, int flags) {
    // Receive encrypted
    char* encrypted = malloc(len + 16);
    int result = WS2_32$recv(s, encrypted, len + 16, flags);

    if (result > 0) {
        // Decrypt
        result = aes_decrypt(encrypted, result, buf);
    }

    free(encrypted);
    return result;
}
```

### Example 3: Anti-Debugging

**Goal**: Bypass debugger checks

```
attach "KERNEL32$IsDebuggerPresent" "_FakeDebugCheck"
attach "NTDLL$NtQueryInformationProcess" "_FakeQueryInfo"
```

```c
BOOL WINAPI _FakeDebugCheck(void) {
    // Always return FALSE (no debugger)
    return FALSE;
}

NTSTATUS WINAPI _FakeQueryInfo(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
) {
    // If querying for debug port
    if (ProcessInformationClass == ProcessDebugPort) {
        *(PDWORD)ProcessInformation = 0;  // No debugger
        return STATUS_SUCCESS;
    }

    // Otherwise call real API
    return NTDLL$NtQueryInformationProcess(
        ProcessHandle, ProcessInformationClass,
        ProcessInformation, ProcessInformationLength,
        ReturnLength
    );
}
```

### Example 4: Memory Protection

**Goal**: Hide RWX memory allocations

```
attach "KERNEL32$VirtualAlloc" "_StealthVirtualAlloc"
```

```c
LPVOID WINAPI _StealthVirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
) {
    // If requesting RWX, allocate as RW first
    if (flProtect == PAGE_EXECUTE_READWRITE) {
        LPVOID mem = KERNEL32$VirtualAlloc(
            lpAddress, dwSize, flAllocationType, PAGE_READWRITE
        );

        // Caller will change to RX later (less suspicious)
        return mem;
    }

    // Normal allocations pass through
    return KERNEL32$VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}
```

## Building Reusable Aspect Libraries

### Aspect Library Structure

```
aspects/
  ├── anti_debug.c      - Anti-debugging hooks
  ├── network_crypto.c  - Network encryption
  ├── api_logging.c     - API logging
  ├── memory_evasion.c  - Memory evasion
  └── telemetry.c       - Telemetry/metrics
```

### Using Aspect Libraries

```
# Build capability
load "capability.x64.o"
  merge

# Merge aspect library
load "aspects/anti_debug.x64.o"
  merge
load "aspects/network_crypto.x64.o"
  merge

# Apply hooks
attach "KERNEL32$IsDebuggerPresent" "_AntiDebug_IsDebuggerPresent"
attach "WS2_32$send" "_NetworkCrypto_Send"

make pic +optimize
```

## Hook Implementation Requirements

### Function Signature

Must match target API exactly:

```c
// Target API
HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    ...
);

// Hook must match
HANDLE WINAPI _MyCreateFileHook(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    ...
);
```

### Calling Convention

Must use correct convention:
- `WINAPI` (`__stdcall`)
- `CALLBACK`
- `NTAPI`

### x86 Considerations

x86 `__stdcall` requires name decoration:

```c
// x64: No decoration
HANDLE WINAPI _MyHook(...);

// x86: Add @## suffix (bytes)
HANDLE WINAPI _MyHook@28(...);  // 28 bytes of args
```

## AOP Best Practices

1. ✅ Keep hooks focused and simple
2. ✅ Handle errors gracefully
3. ✅ Use preserve to prevent recursion
4. ✅ Document hook purpose and behavior
5. ✅ Test hooks independently
6. ❌ Don't create complex hook chains (performance)
7. ❌ Don't modify function parameters unexpectedly
8. ❌ Don't assume hook execution order

## Performance Considerations

### Hook Overhead

Each hook adds:
- Function call overhead
- Hook logic execution time
- Potential memory allocations

### Optimization Strategies

```
# Remove unused hooks
filterhooks $DLL

# Use protect for hot paths
protect "performance_critical"

# Combine hooks
# Instead of 3 separate hooks, one hook handles all
```

## Related Techniques

- [Crystal Palace](../crystal-palace/) - AOP framework
- [Binary Transformation](../binary-transformation/) - Weaving mechanism
- [Position Independent Code](../position-independent-code/) - Target for instrumentation

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [POC](./POC/) for hook examples
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational use
