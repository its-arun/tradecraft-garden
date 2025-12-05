# Crystal Palace - Quick Notes

## Core Concept
Specialized linker for building position-independent loaders with binary transformation capabilities.

## Key Commands

### Loading Files
```
load "file.o"          # Load COFF onto stack
load "file.dll"        # Load DLL onto stack
load "file.bin"        # Load raw binary onto stack
```

### Making Outputs
```
make pic               # Create position-independent code
make pic64             # Create 64-bit PIC
make coff              # Export normalized COFF
make pico              # Create PICO object
```

### Binary Transformations (+options)
```
+optimize              # Remove unused functions via link-time optimization
+disco                 # Randomize function order (keeps entry point first)
+mutate                # Break up constants, stack strings for evasion
+gofirst               # Move go() entry point to position 0
```

### Dynamic Function Resolution
```
dfr "resolver" "ror13"                    # Hash-based API resolution
dfr "resolver" "strings"                  # String-based API resolution
dfr "resolve" "ror13" "KERNEL32, NTDLL"  # Module-specific resolver
dfr "resolve_ext" "strings"               # Fallback for other modules
```

### Resource Management
```
append $PIC            # Append to PIC/PIC64
append $PICO           # Append to PICO
append $DLL            # Append to DLL
append $OBJECT         # Append to current object
```

### COFF Operations
```
merge                  # Merge COFF into previous stack item
mergelib "lib.zip"     # Merge shared library
```

### Symbol Operations
```
remap "old" "new"                         # Rename symbol
exportfunc "function" "__tag_function"    # Export with tag
protect "func1, func2"                    # Protect from hooks
preserve "target|MODULE$API" "func"       # Preserve specific targets
optout "function" "hook1, hook2"          # Exclude hooks from function
```

### Instrumentation (AOP)
```
attach "MODULE$Function" "_Hook"          # Hook Win32 API
redirect "function" "_hook"               # Hook local function
addhook "MODULE$Function" "hook"          # Register IAT hook
filterhooks $DLL                          # Remove unused hooks
```

### PIC Fixes
```
fixptrs "_caller"      # Fix x86 direct addressing (requires caller function)
fixbss "_bss_fix"      # Enable global variables in PIC
```

### Linking
```
link "spec.txt"        # Link using specification file
```

## Stack Model

Crystal Palace uses a stack-based composition model:

```
load "module1.o"       # Stack: [module1]
  merge                # (waits for next item)
load "module2.o"       # Stack: [module1, module2]
  merge                # Stack: [module1+module2]
  make pic             # Stack: [PIC(module1+module2)]
load "payload.bin"     # Stack: [PIC, payload]
  append $PIC          # Stack: [PIC+payload]
```

## Symbol Convention

### API Imports
```c
KERNEL32$VirtualAlloc        // Module$Function format
__imp_KERNEL32$Function      // Import prefix
```

### DFR Intrinsics
```c
__tag_function()             // Get export tag
__resolve_hook(hash)         // Resolve IAT hook
```

### Entry Points
```c
void go(void)                // Standard PIC/PICO entry point
```

## Shared Library Format

ZIP archives containing:
- Compiled objects (.o files)
- Unified conventions for x86/x64
- No read/write globals (PIC compatible)

## Common Patterns

### Simple PIC Build
```
load "loader.x64.o"
  dfr "resolve" "ror13"
  make pic +optimize
```

### COFF Merge
```
load "core.x64.o"
  merge
load "utils.x64.o"
  merge
  make coff
```

### Resource Linking
```
load "loader.x64.o"
  make pic +mutate
load "shellcode.bin"
  append $PIC
load "config.bin"
  append $PIC
```

### Dual-Purpose Loader
```
x64:
  remap "go_dll" "go"         # For DLL target
  # OR
  remap "go_object" "go"      # For COFF target
```

## Performance Tips

1. Use `+optimize` to remove unused shared library code
2. Use `+disco` with `+mutate` for better signature evasion
3. Merge COFFs before applying transformations
4. Filter hooks for specific capabilities to reduce size
5. Use shared libraries to avoid code duplication

## Important Constraints

- Shared library code must avoid read/write globals (PIC requirement)
- fixptrs only works with common instruction forms
- fixbss requires unused section slack space
- DFR resolver functions can't use APIs they're meant to resolve
- Module-specific DFR must come before fallback DFR

## Architecture Differences

### x86 vs x64 PIC
- x64 has RIP-relative addressing (easier PIC)
- x86 requires fixptrs for data references
- Both support DFR and fixbss

### COFF vs DLL
- COFF: Smaller, mutable, easier implementation
- DLL: Full PE format, more features
- Crystal Palace transparently handles both

## Debugging

### Check symbol table
Use objdump or similar tools to verify symbols

### Test transformations incrementally
Apply one +option at a time to isolate issues

### Verify stack operations
Ensure load/merge/append sequence is correct

## Quick Reference: Specification File Structure

```
# x64 architecture block
x64:
  load "code.x64.o"
    dfr "resolve" "ror13"
    fixbss "_bss_fix"
    make pic +optimize +mutate

  load "payload.bin"
    append $PIC

  link "output.x64.bin"

# x86 architecture block
x86:
  load "code.x86.o"
    dfr "resolve" "ror13"
    fixptrs "_caller"
    make pic +optimize

  link "output.x86.bin"
```

## Tags for Quick Search

`#linker` `#binary-transformation` `#PIC` `#COFF` `#DFR` `#resource-linking` `#obfuscation` `#AOP` `#instrumentation`
