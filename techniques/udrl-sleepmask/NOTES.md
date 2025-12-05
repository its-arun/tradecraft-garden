# UDRL and Sleep Mask - Quick Notes

## Core Concepts

**UDRL**: Replace Beacon's default reflective loader with custom implementation
**Sleep Mask**: Obfuscate Beacon memory during sleep periods
**BeaconGate**: Proxy API calls through Sleep Mask with evasion

## UDRL Basics

### What It Does
- Custom PE loading logic
- Custom memory allocation
- Custom syscall resolution
- Bypasses Malleable C2 `stage` block limitations

### Minimum Requirements
```c
1. Locate DLL image
2. Parse NT Headers
3. Resolve APIs via PEB
4. Allocate memory
5. Map PE sections
6. Resolve imports/relocations
7. Execute DllMain
```

### Communication with Sleep Mask
```c
ALLOCATED_MEMORY_REGION region = {
    .purpose = BEACON_CODE,
    .base_address = beacon_base,
    .size = image_size,
    .memory_type = VirtualAlloc_RWX
};

pass_via_DLL_BEACON_USER_DATA(&region);
```

## Sleep Mask Basics

### What It Does
- Receives allocation metadata from UDRL
- Masks memory before sleep
- Unmasks after sleep
- Integrates with BeaconGate

### Masking Strategies
```c
// XOR
xor_encrypt(beacon, size, key);

// RC4
rc4_encrypt(beacon, size, key, keylen);

// Custom
custom_obfuscate(beacon, size);
```

### Selective Masking
```c
if (region->purpose == BEACON_CODE) {
    mask(region);  // Mask code
}

if (region->memory_type == HEAP) {
    skip;  // Don't mask heap
}
```

## BeaconGate Basics

### What It Does
- Proxies API calls via Sleep Mask
- Enables call stack spoofing
- Uses custom syscalls from UDRL
- Transparent to BOFs

### Function Call Structure
```c
FUNCTION_CALL call = {
    .function_ptr = &VirtualAlloc,
    .api_enum = API_VIRTUALALLOC,
    .arg_count = 4,
    .args = {NULL, 0x1000, MEM_COMMIT, PAGE_RW},
    .should_mask = TRUE
};

result = BeaconGateWrapper(&call);
```

### Execution Flow
```
1. Mask Beacon (optional)
2. Spoof call stack
3. Execute API (custom syscall or direct)
4. Unmask Beacon
5. Return result
```

## Integration Patterns

### Pattern 1: UDRL + Sleep Mask
```c
// UDRL
allocate_beacon_with_custom_strategy();
populate_allocation_metadata();

// Sleep Mask
receive_metadata_from_udrl();
mask_based_on_actual_allocations();
```

### Pattern 2: UDRL + BeaconGate
```c
// UDRL
resolve_custom_syscalls();
populate_syscall_info();

// BeaconGate
get_syscalls_from_udrl();
use_for_api_calls();
```

### Pattern 3: All Three
```c
// UDRL
custom_allocate();
resolve_syscalls();
pass_metadata();

// Sleep Mask
receive_metadata();
mask_intelligently();

// BeaconGate
use_syscalls();
mask_during_calls();
spoof_stack();
```

## Syscall Resolution

### Populate in UDRL
```c
SYSCALL_API api = {
    .function_ptr = resolve_function("NtAllocateVirtualMemory"),
    .jump_address = find_syscall_instruction(api.function_ptr),
    .syscall_number = extract_syscall_number(api.jump_address)
};
```

### Access in BeaconGate
```c
// MUST be before masking
BEACON_SYSCALLS* sc = BeaconGetSyscallInformation();

// Then mask
mask_beacon();

// Use syscalls
sc->nt_allocate_virtual_memory(...);
```

### Execution Modes
```c
// C2 Profile
set syscall_method "direct";   // Use function_ptr
set syscall_method "indirect";  // Use jump + syscall_number
```

## BOF Integration

### Transparent Usage
```c
// BOF code unchanged
void* mem = KERNEL32$VirtualAlloc(...);

// Automatically uses:
// - Custom syscalls (if configured)
// - BeaconGate (if enabled)
// - Memory masking (if configured)
```

### Helper Functions
```c
// Abstraction layer
BeaconVirtualAlloc();
BeaconOpenProcess();
BeaconReadProcessMemory();
// etc.
```

## Critical Warnings

### ⚠️ Stage Block Ignored
```c
// With UDRL, these are IGNORED:
stage {
    set userwx "false";      // IGNORED
    set obfuscate "true";    // IGNORED
    set allocator "...";     // IGNORED
}
```

### ⚠️ Allocation Strategy Alignment
```c
// UDRL and Sleep Mask must match

// ✅ GOOD: Both expect RWX
UDRL: VirtualAlloc(..., PAGE_EXECUTE_READWRITE);
Sleep Mask: expects single RWX region

// ❌ BAD: Mismatch
UDRL: Separate RW + RX allocations
Sleep Mask: expects single RWX region
// RESULT: Crash or failed masking
```

### ⚠️ BeaconGate Priority
```c
// BeaconGate takes priority
set syscall_method "direct";  // Ignored if:
set beacon_gate "true";        // This is enabled
```

### ⚠️ Syscall Access Timing
```c
// MUST get syscalls BEFORE masking
BEACON_SYSCALLS* sc = BeaconGetSyscallInformation();  // Before mask
mask_beacon();                                         // After getting syscalls

// NOT:
mask_beacon();                                         // Wrong order!
BEACON_SYSCALLS* sc = BeaconGetSyscallInformation();  // Too late - crash
```

## Common Use Cases

### Use Case 1: Custom Allocator
```c
// UDRL
void* beacon = NtAllocateVirtualMemory(...);  // Direct syscall
// Or
void* beacon = allocate_via_mapped_section();
// Or
void* beacon = allocate_via_process_heap();
```

### Use Case 2: Syscall Evasion
```c
// UDRL resolves syscalls
hell_gate_resolve(&syscalls);
// Or
halo_gate_resolve(&syscalls);

// BeaconGate uses them
// Bypasses all userland hooks
```

### Use Case 3: Memory Scan Evasion
```c
// Sleep Mask
before_sleep: RC4_encrypt(beacon, size, key);
after_sleep: RC4_decrypt(beacon, size, key);

// Scanner sees encrypted garbage
```

### Use Case 4: Stack Analysis Evasion
```c
// BeaconGate
spoof_stack_via_gadgets();
call_api();
restore_stack();

// Analyzer sees fake call stack
```

## Quick Implementation Checklist

### UDRL Checklist
- [ ] Parse PE headers
- [ ] Resolve critical APIs
- [ ] Implement custom allocation
- [ ] Map PE sections
- [ ] Resolve imports
- [ ] Process relocations
- [ ] Populate syscall info (optional)
- [ ] Pass metadata to Beacon
- [ ] Call DllMain

### Sleep Mask Checklist
- [ ] Receive SLEEPMASK_INFO
- [ ] Parse allocation regions
- [ ] Implement masking algorithm
- [ ] Handle selective masking
- [ ] Integrate with BeaconGate (optional)

### BeaconGate Checklist
- [ ] Implement FUNCTION_CALL handler
- [ ] Get syscalls before masking
- [ ] Implement call stack spoofing
- [ ] Execute API calls
- [ ] Handle masking/unmasking
- [ ] Return results properly

## Configuration Examples

### Minimal UDRL
```c
// Basic custom loader
// No syscalls, no special features
// Just custom allocation
```

### UDRL + Syscalls
```c
// Custom loader + syscall resolution
// No Sleep Mask or BeaconGate
// For syscall evasion only
```

### UDRL + Sleep Mask
```c
// Custom loader + memory masking
// No BeaconGate
// For memory scan evasion
```

### Full Stack
```c
// UDRL + Sleep Mask + BeaconGate
// Complete evasion suite
// Maximum capability, maximum complexity
```

## Debugging Tips

### UDRL Debugging
```c
// Add logging
log("Beacon DLL at: %p", beacon_dll);
log("Allocated at: %p", beacon_base);
log("DllMain returned: %d", result);
```

### Sleep Mask Debugging
```c
// Verify region data
log("Regions: %d", info->region_count);
log("Region 0: %p, size: %zu", region->base, region->size);
```

### BeaconGate Debugging
```c
// Log API calls
log("API call: %d, args: %d", call->api_enum, call->arg_count);
log("Result: %p", result);
```

## Performance Considerations

### UDRL Impact
- **Startup time**: +10-50ms (PE parsing/mapping)
- **Memory**: Same as standard loader
- **Runtime**: No impact after load

### Sleep Mask Impact
- **Sleep time**: +5-20ms (encryption overhead)
- **Memory**: Minimal (key storage)
- **Detection**: Significantly reduces memory signatures

### BeaconGate Impact
- **API calls**: +2-10ms per call (spoofing overhead)
- **Memory**: Minimal
- **Detection**: Breaks call stack analysis

## Resource Links

- **UDRL Guide**: Cobalt Strike documentation
- **Sleep Mask**: Community implementations (SleepyCrypt)
- **BeaconGate**: Cobalt Strike 4.5+ documentation
- **Syscall Resolution**: Hell's Gate, Halo's Gate papers

## Tags
`#UDRL` `#SleepMask` `#BeaconGate` `#CobaltStrike` `#memory-evasion` `#syscalls` `#reflective-loading`
