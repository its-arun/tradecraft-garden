# Crystal Palace - Specialized PIC Linker

## Overview

Crystal Palace is a specialized linker and linker script language designed specifically for building position-independent DLL loaders and capability containers. It's the foundational tool of the Tradecraft Garden ecosystem.

## What It Does

Crystal Palace enables you to:
- Link position-independent code (PIC) and PICOs
- Access appended resources via linked symbols
- Transform binaries with optimization, mutation, and obfuscation
- Package multiple resources into a single capability
- Manage global variables in PIC code
- Build sophisticated multi-stage loaders

## Key Features

### 🔗 Linker Script Language
- Custom specification files for capability assembly
- Stack-based program composition
- Support for DLL, COFF, and PIC targets

### 🔄 Binary Transformation
- **+optimize** - Link-time optimization to remove unused code
- **+disco** - Randomize function order while preserving entry points
- **+mutate** - Break up constants, stack strings, create noise for signature evasion

### 📦 Resource Management
- Append multiple resources to capabilities
- Access resources via linked symbols
- Support for DLL, COFF, shellcode, and custom data

### 🛠️ Multi-Format Support
- DLL files (traditional capabilities)
- COFF files (smaller, more flexible)
- PIC/PIC64 (position-independent code)
- PICO (position-independent capability objects)

## Basic Commands

```
# Load a COFF file onto the stack
load "bin/capability.x64.o"

# Apply binary transformation
make pic +optimize +mutate

# Link with resources
link "loader.spec.txt"

# Create COFF export
make coff

# Merge multiple COFFs
load "bin/module1.x64.o"
  merge
load "bin/module2.x64.o"
  merge
```

## Quick Example Specification

```crystal-palace
# Simple PIC loader specification
load "bin/loader.x64.o"
  dfr "resolve" "ror13"
  fixptrs "_caller"
  make pic +optimize +gofirst

# Append shellcode resource
load "payload.bin"
  append $PIC
```

## Use Cases

1. **Reflective DLL Loaders** - Build custom in-memory loaders
2. **Staged Payloads** - Create multi-resource capability packages
3. **COFF Runners** - Execute COFF files from memory
4. **Obfuscated Capabilities** - Apply mutations for signature evasion
5. **Modular Implants** - Assemble capabilities from components

## Architecture

```
┌─────────────────────────────────────┐
│     Crystal Palace Linker           │
├─────────────────────────────────────┤
│  • Specification Parser             │
│  • Stack-based Composition          │
│  • Binary Transformer               │
│  • Symbol Linker                    │
│  • Resource Appender                │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  Output Capability                  │
├─────────────────────────────────────┤
│  PIC/PICO/DLL Header                │
│  Transformed Code                   │
│  Appended Resources                 │
│  Symbol Table                       │
└─────────────────────────────────────┘
```

## Related Techniques

- [Position Independent Code](../position-independent-code/) - Understanding PIC fundamentals
- [Binary Transformation](../binary-transformation/) - Deep dive into transformations
- [COFF Operations](../coff-operations/) - Working with COFF files
- [PICO](../pico/) - Position Independent Capability Objects

## Resources

- [Tradecraft Garden Documentation](https://tradecraftgarden.org/)
- [Release Notes](https://tradecraftgarden.org/releasenotes.txt)
- [Video Tutorials](https://tradecraftgarden.org/videos.html)

## Next Steps

1. Read [NOTES.md](./NOTES.md) for quick reference
2. Study [DETAILED_EXPLANATION.md](./DETAILED_EXPLANATION.md) for in-depth understanding
3. Explore [POC](./POC/) for practical examples
4. Review [RED_TEAM_USAGE.md](./RED_TEAM_USAGE.md) for operational guidance
