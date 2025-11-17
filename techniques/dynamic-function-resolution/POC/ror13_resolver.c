/*
 * ROR13 Hash-Based Function Resolver
 *
 * Complete implementation of ROR13 hash-based API resolution
 * for position-independent code.
 *
 * Features:
 * - Walk PEB to find modules
 * - Hash-based module lookup
 * - Hash-based function lookup
 * - Caching for performance
 */

#include <windows.h>

// ============================================================================
// ROR13 HASHING
// ============================================================================

DWORD ror13_hash(const char* str) {
    DWORD hash = 0;
    while (*str) {
        hash = (hash >> 13) | (hash << (32 - 13));
        hash += (DWORD)(*str);
        str++;
    }
    return hash;
}

// Unicode version for module names
DWORD unicode_ror13_hash(PUNICODE_STRING str) {
    DWORD hash = 0;
    WCHAR* ptr = str->Buffer;

    for (int i = 0; i < str->Length / sizeof(WCHAR); i++) {
        WCHAR c = ptr[i];

        // Convert to uppercase
        if (c >= 'a' && c <= 'z') {
            c -= 0x20;
        }

        hash = (hash >> 13) | (hash << (32 - 13));
        hash += (DWORD)c;
    }

    return hash;
}

// ============================================================================
// MODULE LOOKUP
// ============================================================================

HMODULE find_module_by_hash(DWORD module_hash) {
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    PPEB_LDR_DATA pLdr = pPeb->Ldr;
    PLIST_ENTRY pListHead = &pLdr->InMemoryOrderModuleList;
    PLIST_ENTRY pListEntry = pListHead->Flink;

    while (pListEntry != pListHead) {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(
            pListEntry,
            LDR_DATA_TABLE_ENTRY,
            InMemoryOrderLinks
        );

        DWORD hash = unicode_ror13_hash(&pEntry->BaseDllName);

        if (hash == module_hash) {
            return (HMODULE)pEntry->DllBase;
        }

        pListEntry = pListEntry->Flink;
    }

    return NULL;
}

// ============================================================================
// FUNCTION LOOKUP
// ============================================================================

FARPROC find_function_by_hash(HMODULE hModule, DWORD function_hash) {
    if (!hModule) return NULL;

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + pDosHeader->e_lfanew
    );

    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return NULL;
    }

    DWORD export_rva = pNtHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT
    ].VirtualAddress;

    if (!export_rva) return NULL;

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(
        (BYTE*)hModule + export_rva
    );

    DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
    DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        char* funcName = (char*)((BYTE*)hModule + pNames[i]);
        DWORD hash = ror13_hash(funcName);

        if (hash == function_hash) {
            DWORD func_rva = pFunctions[pOrdinals[i]];
            return (FARPROC)((BYTE*)hModule + func_rva);
        }
    }

    return NULL;
}

// ============================================================================
// CACHED RESOLVER
// ============================================================================

#define MAX_CACHE_ENTRIES 256

typedef struct {
    DWORD module_hash;
    DWORD function_hash;
    FARPROC address;
} CACHE_ENTRY;

CACHE_ENTRY g_cache[MAX_CACHE_ENTRIES];
int g_cache_count = 0;

FARPROC resolve_cached(DWORD module_hash, DWORD function_hash) {
    // Check cache
    for (int i = 0; i < g_cache_count; i++) {
        if (g_cache[i].module_hash == module_hash &&
            g_cache[i].function_hash == function_hash) {
            return g_cache[i].address;
        }
    }

    // Not in cache, resolve
    HMODULE hModule = find_module_by_hash(module_hash);
    FARPROC addr = find_function_by_hash(hModule, function_hash);

    // Add to cache if space available
    if (addr && g_cache_count < MAX_CACHE_ENTRIES) {
        g_cache[g_cache_count].module_hash = module_hash;
        g_cache[g_cache_count].function_hash = function_hash;
        g_cache[g_cache_count].address = addr;
        g_cache_count++;
    }

    return addr;
}

// ============================================================================
// COMMON API HASHES
// ============================================================================

// Module hashes
#define HASH_KERNEL32           0x6A4ABC5B
#define HASH_NTDLL              0x3CFA685D
#define HASH_USER32             0x74FA1C3E
#define HASH_ADVAPI32           0x85B9F5E2

// Kernel32 function hashes
#define HASH_VIRTUALALLOC       0x91AFCA54
#define HASH_VIRTUALFREE        0x030633D6
#define HASH_VIRTUALPROTECT     0x7946C61B
#define HASH_LOADLIBRARYA       0xEC0E4E8E
#define HASH_GETPROCADDRESS     0x7C0DFCAA
#define HASH_CREATETHREAD       0x3F9287AE
#define HASH_WAITFORSINGLEOBJECT 0x601D8708
#define HASH_SLEEP              0xE035F044
#define HASH_CREATEFILEA        0x4FDAF6DA
#define HASH_READFILE           0xBB5F9EAD
#define HASH_WRITEFILE          0x5BAE572D
#define HASH_CLOSEHANDLE        0x0FFD97FB

// ============================================================================
// EXAMPLE USAGE
// ============================================================================

void example_usage(void) {
    // Resolve VirtualAlloc
    typedef LPVOID (WINAPI *pVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
    pVirtualAlloc VirtualAlloc = (pVirtualAlloc)resolve_cached(
        HASH_KERNEL32,
        HASH_VIRTUALALLOC
    );

    // Use the function
    if (VirtualAlloc) {
        void* mem = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
        // Use mem...
    }

    // Resolve CreateThread
    typedef HANDLE (WINAPI *pCreateThread)(
        LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE,
        LPVOID, DWORD, LPDWORD
    );
    pCreateThread CreateThread = (pCreateThread)resolve_cached(
        HASH_KERNEL32,
        HASH_CREATETHREAD
    );

    if (CreateThread) {
        HANDLE hThread = CreateThread(NULL, 0, my_thread_func, NULL, 0, NULL);
        // Use hThread...
    }
}

// ============================================================================
// HASH GENERATOR UTILITY
// ============================================================================

void generate_hashes(void) {
    // Example: Generate hash for a new API

    const char* modules[] = {
        "KERNEL32.DLL",
        "NTDLL.DLL",
        "USER32.DLL",
        "ADVAPI32.DLL",
        NULL
    };

    const char* functions[] = {
        "VirtualAlloc",
        "CreateThread",
        "LoadLibraryA",
        "GetProcAddress",
        NULL
    };

    // Generate module hashes
    for (int i = 0; modules[i]; i++) {
        DWORD hash = ror13_hash(modules[i]);
        // Output: #define HASH_<MODULE> 0x<hash>
    }

    // Generate function hashes
    for (int i = 0; functions[i]; i++) {
        DWORD hash = ror13_hash(functions[i]);
        // Output: #define HASH_<FUNCTION> 0x<hash>
    }
}
