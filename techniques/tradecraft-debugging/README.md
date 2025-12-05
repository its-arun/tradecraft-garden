# Tradecraft Garden Debugging - Development Best Practices

## Overview

Debugging position-independent code and Crystal Palace projects requires specialized techniques. This guide covers debugging strategies for Tradecraft Garden development using WSL, Visual Studio Code, and WinDbg.

## Development Environment Setup

### WSL Integration with VS Code

**The Problem**: Developing on Windows with MSVC compiler causes issues with Crystal Palace (requires mingw-w64).

**The Solution**: Use WSL with VS Code's WSL extension.

### Setup Steps

**1. Install WSL Extension in VS Code**:
```bash
# In VS Code
# Install "Remote - WSL" extension
```

**2. Connect to WSL**:
```bash
# Method 1: From WSL terminal
cd /path/to/tradecraft-garden
code .

# Method 2: From VS Code
# Click blue icon (bottom-left)
# Select "Connect to WSL"
```

**3. Configure C/C++ Extension**:
```json
// .vscode/c_cpp_properties.json
{
    "configurations": [
        {
            "name": "WSL",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/x86_64-w64-mingw32-gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64"
        }
    ]
}
```

**Result**: Intellisense works correctly with mingw-w64.

## Debug Build Configuration

### Standard Makefile (Release)

```makefile
# Makefile
CC_64 = x86_64-w64-mingw32-gcc
CFLAGS = -DWIN_X64 -nostartfiles

bin/loader.x64.o: src/loader.c
	$(CC_64) $(CFLAGS) -c src/loader.c -o bin/loader.x64.o
```

### Debug Makefile Target

```makefile
# Add debug target with -g flag
bin/loader.x64.exe: bin/loader.x64.o
	$(CC_64) -DWIN_X64 -nostartfiles -g \
	  -Wl,-e,go \
	  bin/loader.x64.o \
	  -o bin/loader.x64.exe
```

**Key Flags**:
- `-g` : Include debugging information
- `-Wl,-e,go` : Set entry point to `go()` function

**Result**: Debuggable executable compatible with WinDbg.

## Handling DFR Convention

### The Problem

Crystal Palace uses `MODULE$Function` convention for DFR:

```c
// This works with Crystal Palace
KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_RW);
```

**But standard linkers don't understand this syntax** - they fail with:

```
undefined reference to 'KERNEL32$VirtualAlloc'
```

### The Solution: Conditional Compilation

```c
// Use preprocessor to handle debug vs release builds

#ifdef DEBUG
    // Debug build: Use standard Win32 functions
    #define KERNEL32$VirtualAlloc VirtualAlloc
    #define KERNEL32$CreateThread CreateThread
    // etc.
#else
    // Release build: Use DFR convention (Crystal Palace handles)
    // No defines needed, Crystal Palace processes MODULE$Function
#endif
```

**In code**:
```c
#include <windows.h>

#ifdef DEBUG
    #define KERNEL32$VirtualAlloc VirtualAlloc
    #define KERNEL32$GetProcAddress GetProcAddress
#endif

void go(void) {
    // Works in both debug and release
    void* mem = KERNEL32$VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_RW);
}
```

**Build**:
```makefile
# Debug build
bin/loader_debug.exe:
	$(CC_64) -DDEBUG -g src/loader.c -o bin/loader_debug.exe

# Release build (for Crystal Palace)
bin/loader.x64.o:
	$(CC_64) -c src/loader.c -o bin/loader.x64.o
```

## WinDbg Debugging

### Configuring Source Paths

```
# In WinDbg command window
.srcpath c:\path\to\tradecraft-garden\src

# Or set in WinDbg settings
# File → Source File Path → Add your src directory
```

### Common WinDbg Commands

```
# Set breakpoint at go()
bp go

# Run
g

# Step into
t

# Step over
p

# View call stack
k

# View locals
dv

# View disassembly
u .

# View memory
db <address>

# Continue
g
```

### Debugging Workflow

**1. Build debug executable**:
```bash
make debug
```

**2. Transfer to Windows**:
```bash
# From WSL
cp bin/loader_debug.exe /mnt/c/Users/YourName/Desktop/
```

**3. Open in WinDbg**:
```
File → Open Executable → loader_debug.exe
```

**4. Set source path**:
```
.srcpath C:\path\to\src
```

**5. Set breakpoint**:
```
bp go
```

**6. Run and debug**:
```
g     # Go
t     # Step
k     # Stack
```

## Embedded DLL Testing

### The Problem

Crystal Palace appends DLLs to loaders. During debugging, how do you test without building the full package?

### The Solution: Embed DLL as C Array

```bash
# Convert DLL to C array using xxd
xxd -i target.dll > target_dll.h
```

**target_dll.h**:
```c
unsigned char target_dll[] = {
  0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
  // ... DLL bytes
};
unsigned int target_dll_len = 12345;
```

**In loader code**:
```c
#ifdef DEBUG
    // Debug: Use embedded DLL
    #include "target_dll.h"

    void go(void) {
        load_dll(target_dll, target_dll_len);
    }
#else
    // Release: Use appended DLL
    extern unsigned char _binary_target_dll_start[];
    extern unsigned int _binary_target_dll_size;

    void go(void) {
        load_dll(_binary_target_dll_start, _binary_target_dll_size);
    }
#endif
```

**Benefits**:
- Test loader logic without Crystal Palace
- Faster iteration (no build pipeline)
- Source-level debugging

## PICO Debugging Limitations

### Important

**PICOs cannot be debugged** with source-level information. Crystal Palace generates:
- Position-independent code
- No debug symbols
- Transformed code (may not match source)

**For PICOs**: Assembly-level debugging only.

### Workaround

Test PICO logic in debug executable first:
```
1. Write PICO code
2. Build as debug executable
3. Debug and fix issues
4. Build as PICO (release)
5. Test PICO execution
```

## Common Issues and Solutions

### Issue 1: Linker Errors on DFR Syntax

**Error**: `undefined reference to 'KERNEL32$VirtualAlloc'`

**Solution**: Add conditional defines:
```c
#ifdef DEBUG
    #define KERNEL32$VirtualAlloc VirtualAlloc
#endif
```

### Issue 2: No Symbols in WinDbg

**Error**: WinDbg shows no source/symbols

**Solution**:
- Verify `-g` flag used in compilation
- Set source path: `.srcpath C:\path\to\src`
- Reload symbols: `.reload`

### Issue 3: Wrong Compiler

**Error**: Compilation errors, missing types

**Solution**:
- Verify using mingw-w64, not MSVC
- Check `which x86_64-w64-mingw32-gcc`
- Use WSL with VS Code Remote

### Issue 4: PIC Crashes

**Error**: Compiled code crashes when tested

**Solution**:
- Test as regular executable first (debug build)
- Verify logic works before building PIC
- Check for position-dependent code patterns

## Best Practices

### 1. Separate Debug and Release

```makefile
# Debug builds: Regular executables
debug: CFLAGS += -DDEBUG -g
debug: bin/loader_debug.exe

# Release builds: PIC objects
release: bin/loader.x64.o
```

### 2. Use Conditional Compilation Extensively

```c
#ifdef DEBUG
    // Debug-specific code
    #include <stdio.h>
    printf("Debug: ...\n");
#else
    // Release-specific code
    // No printf, minimal imports
#endif
```

### 3. Test Logic Before PIC Build

```
Development cycle:
1. Write code
2. Build debug executable
3. Test and debug
4. Fix issues
5. Build release PIC
6. Test PIC
7. Repeat if needed
```

### 4. Version Control .vscode

```
.vscode/
  c_cpp_properties.json   # Compiler settings
  launch.json             # Debug configs
  tasks.json              # Build tasks
```

## VS Code Tasks

### tasks.json

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build debug",
            "type": "shell",
            "command": "make debug",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        },
        {
            "label": "build release",
            "type": "shell",
            "command": "make release"
        }
    ]
}
```

**Usage**: `Ctrl+Shift+B` to build debug.

## Related Techniques

- [Crystal Palace](../crystal-palace/) - Understanding the linker
- [Position Independent Code](../position-independent-code/) - PIC fundamentals

## Resources

- [Debugging the Tradecraft Garden](https://rastamouse.me/debugging-the-tradecraft-garden/) - Rasta Mouse
- [WinDbg Documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/) - Microsoft

## Next Steps

1. Set up WSL + VS Code environment
2. Configure debug build targets
3. Practice with WinDbg
4. Develop debugging workflow
