# Binary Transformation Framework

## Overview

Binary transformation is the ability to disassemble programs, modify them at the instruction level, and reassemble them. Crystal Palace's binary transformation framework enables powerful code mutations, optimizations, and instrumentation impossible with source code alone.

## Core Concept

```
Source Code → Compile → COFF
                          ↓
            Crystal Palace Disassembles
                          ↓
            Transform Instructions
                          ↓
            Reassemble → Output
```

## Available Transformations

### +optimize: Link-Time Optimization

**Purpose**: Remove unused code

**Usage**:
```
make pic +optimize
```

**How it works**:
1. Start from entry point `go()`
2. Build complete call graph
3. Mark all reachable functions
4. Remove unreachable functions and data
5. Compact sections

**Benefits**:
- 30-70% size reduction (especially with shared libraries)
- Faster loading
- Smaller signature surface
- Removes unused defensive code

**Example**:
```c
// Shared library has 100 functions
// Your code uses 10 functions

Without +optimize: 500KB
With +optimize: 50KB (90% reduction)
```

### +disco: Function Randomization

**Purpose**: Randomize function ordering

**Usage**:
```
make pic +disco
```

**How it works**:
1. Identify function boundaries in .text
2. Keep entry point (first function) fixed
3. Randomly shuffle remaining functions
4. Update all call instructions and relocations

**Benefits**:
- Breaks signature patterns based on function order
- Defeats static analysis expecting specific layout
- Each build has different function arrangement

**Example**:
```
Original:    [go] [func1] [func2] [func3] [func4]
Randomized:  [go] [func3] [func1] [func4] [func2]
```

### +mutate: Code Mutation

**Purpose**: Signature evasion through code mutation

**Usage**:
```
make pic +mutate
```

**How it works**:
1. **Constant Breaking**: Replace immediate values with equivalent calculations
2. **Stack Strings**: Convert string constants to stack-based construction
3. **Noise Injection**: Insert benign instructions
4. **Instruction Substitution**: Replace with functional equivalents

**Benefits**:
- Unique binary signature per build
- Defeats static signatures
- Maintains readability (not obfuscation)
- "Content signature resilience"

**Examples**:
```assembly
; Original
mov eax, 0x1000

; Mutated
xor eax, eax
add eax, 0x800
add eax, 0x800

; ----

; Original
push offset string_data

; Mutated (stack string)
push 0x6C6C6568  ; "lleh"
push 0x00000000
mov eax, esp
; Use eax as string pointer
```

### +gofirst: Entry Point Positioning

**Purpose**: Move entry point to offset 0

**Usage**:
```
make pic +gofirst
```

**How it works**:
1. Locate `go()` function
2. Move it to first position in .text
3. Update all relocations

**Benefits**:
- PICO convention compliance
- Predictable entry point
- No trampoline needed

### Combined Transformations

**Order matters**:
```
make pic +optimize +disco +mutate +gofirst
```

**Processing order**:
1. Parse and normalize COFF
2. Apply DFR transformations (if specified)
3. **+optimize** (before disco to avoid shuffling dead code)
4. **+disco** (randomize before mutation)
5. **+mutate** (mutate the randomized layout)
6. **+gofirst** (final positioning)
7. Generate output

## Binary Transformation Internals

### Disassembly Engine

Crystal Palace includes an x86/x64 disassembler:

```
Instructions → Decoder → IR (Intermediate Representation)
                              ↓
                         Transform IR
                              ↓
                         Encoder → Instructions
```

### Instruction Recognition

Crystal Palace recognizes:
- **MOV**: All forms (reg-reg, reg-mem, reg-imm)
- **LEA**: Address calculation
- **CALL**: Direct and indirect
- **JMP**: All conditional and unconditional jumps
- **PUSH/POP**: Stack operations
- **Arithmetic**: ADD, SUB, XOR, etc.
- **Comparison**: CMP, TEST

### Relocation Handling

After transformation, Crystal Palace updates:
- Call targets
- Jump targets
- Data references
- Symbol offsets

## Transformation Techniques

### Constant Breaking

```c
// Original source
int value = 0x12345678;

// Compiler generates
mov eax, 0x12345678

// Crystal Palace transforms
mov eax, 0x12340000
add eax, 0x5678
```

**Variations**:
- XOR chains: `xor eax, eax; xor eax, 0x1234...`
- Multiple additions: `mov eax, 0x1000; add eax, 0x234; add eax, 0x5678`
- Subtractions: `mov eax, 0xFFFFFFFF; sub eax, 0xEDCBA987`

### Stack Strings

```c
// Original source
const char* msg = "Hello World";

// Normal compilation
push offset str_hello_world

// Crystal Palace stack string
push 0x00000000  ; Null terminator
push 0x6C72576F  ; "lrWo"
push 0x57206F6C  ; "W ol"
push 0x6C65486C  ; "leHl"
mov eax, esp     ; eax points to string
```

**Benefits**:
- No strings in .rdata
- Harder to find with strings tool
- Unique per build (can randomize push order)

### Noise Injection

```assembly
; Original
mov eax, ebx
call function

; With noise
mov eax, ebx
push ecx         ; Noise
pop ecx          ; Noise
nop              ; Noise
call function
```

**Safe noise instructions**:
- `nop` (no operation)
- `push reg; pop reg` (no net effect)
- `mov reg, reg` (redundant move)
- `xchg reg, reg` (same register)

### Instruction Substitution

```assembly
; Original
mov eax, 0

; Alternatives
xor eax, eax
sub eax, eax
and eax, 0
```

## Limitations and Constraints

### What Can Be Transformed

✅ Common instruction patterns
✅ Standard calling conventions
✅ Normal control flow
✅ Regular data access

### What Cannot Be Transformed

❌ Exotic compiler optimizations
❌ Hand-written assembly with unusual patterns
❌ Self-modifying code
❌ Obfuscated input code

### Compiler Compatibility

**Best Results**:
- GCC with `-O2`
- Clang with `-O2`
- MSVC with `/O2`

**Problematic**:
- Aggressive optimization (`-O3`)
- LTO (Link-Time Optimization) before Crystal Palace
- Vectorized code (SIMD)

## Practical Examples

### Example 1: Small Capability

```
load "minimal_beacon.x64.o"
  make pic +optimize +mutate
```

**Result**:
- Unused functions removed
- Constants mutated
- Unique signature

### Example 2: Modular Capability

```
load "core.x64.o"
  merge
load "http.x64.o"
  merge
load "crypto.x64.o"
  merge
  make pic +optimize +disco +mutate
```

**Result**:
- All modules merged
- Unused code from all modules removed
- Functions randomized across modules
- Mutated for signature evasion

### Example 3: Shared Library

```
load "capability.x64.o"
  mergelib "libtcg.x64.zip"
  make pic +optimize +mutate
```

**Result**:
- Library included
- Only used library functions retained
- Library and capability code both mutated

## Debugging Transformed Code

### Compare Before/After

```bash
# Before transformation
objdump -d original.o > original.asm

# After transformation
objdump -d transformed.o > transformed.asm

# Compare
diff -u original.asm transformed.asm
```

### Disable Transformations Incrementally

```
# Test without mutation
make pic +optimize +disco

# Test without disco
make pic +optimize +mutate

# Test without optimize
make pic +disco +mutate

# Isolate problematic transformation
```

### Verbose Mode

```bash
# Crystal Palace with debugging
./link --verbose capability.spec.txt
```

## Performance Impact

| Transformation | Size Impact | Speed Impact |
|----------------|-------------|--------------|
| +optimize      | -30 to -70% | Faster (less code) |
| +disco         | 0%          | No change |
| +mutate        | +5 to +15%  | Slight slowdown |
| +gofirst       | 0%          | No change |

## Security Benefits

### Signature Evasion

**Without mutation**:
```
Build 1: SHA256: abc123...
Build 2: SHA256: abc123...  (identical)
```

**With mutation**:
```
Build 1: SHA256: abc123...
Build 2: SHA256: def456...  (unique)
Build 3: SHA256: 789xyz...  (unique)
```

### Attribution Resistance

Different transformations prevent linking implants:
```
# Target A
make pic +disco +mutate

# Target B
make pic +mutate  # Different disco order

# No two targets have same binary
```

## Related Techniques

- [Crystal Palace](../crystal-palace/) - Transformation framework
- [Position Independent Code](../position-independent-code/) - PIC generation
- [Aspect-Oriented Programming](../aspect-oriented-tradecraft/) - Instrumentation

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [POC](./POC/) for examples
3. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational guidance
