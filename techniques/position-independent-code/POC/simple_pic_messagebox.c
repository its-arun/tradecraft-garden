/*
 * Simple PIC Example - MessageBox
 *
 * This demonstrates basic position-independent code that displays a message box
 * using manual API resolution via hash-based DFR.
 *
 * Compile:
 *   x86_64-w64-mingw32-gcc -c simple_pic_messagebox.c -o simple_pic_messagebox.x64.o
 *
 * Use with Crystal Palace:
 *   load "simple_pic_messagebox.x64.o"
 *     dfr "resolve" "ror13"
 *     make pic +optimize
 *   link "messagebox.bin"
 */

#include <windows.h>

// ROR13 hash algorithm
DWORD ror13_hash(const char* str) {
    DWORD hash = 0;
    while (*str) {
        hash = (hash >> 13) | (hash << (32 - 13));
        hash += (DWORD)(*str);
        str++;
    }
    return hash;
}

// Pre-computed hashes
#define HASH_KERNEL32           0x6A4ABC5B
#define HASH_USER32             0x74FA1C3E
#define HASH_LOADLIBRARYA       0xEC0E4E8E
#define HASH_GETPROCADDRESS     0x7C0DFCAA
#define HASH_MESSAGEBOXA        0xBC4DA2A8

// Get kernel32 base from PEB
HMODULE get_kernel32(void) {
    #ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif

    PLIST_ENTRY pListHead = &pPeb->Ldr->InMemoryOrderModuleList;
    PLIST_ENTRY pListEntry = pListHead->Flink;

    // Second entry is usually kernel32
    pListEntry = pListEntry->Flink;
    PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(
        pListEntry,
        LDR_DATA_TABLE_ENTRY,
        InMemoryOrderLinks
    );

    return (HMODULE)pEntry->DllBase;
}

// Resolve function by hash
FARPROC resolve_by_hash(HMODULE hModule, DWORD function_hash) {
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(
        (BYTE*)hModule + pDosHeader->e_lfanew
    );

    DWORD export_rva = pNtHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT
    ].VirtualAddress;

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
            return (FARPROC)((BYTE*)hModule + pFunctions[pOrdinals[i]]);
        }
    }

    return NULL;
}

// Entry point
void go(void) {
    // Get kernel32
    HMODULE hKernel32 = get_kernel32();

    // Resolve LoadLibraryA and GetProcAddress
    typedef HMODULE (WINAPI *pLoadLibraryA)(LPCSTR);
    typedef FARPROC (WINAPI *pGetProcAddress)(HMODULE, LPCSTR);

    pLoadLibraryA LoadLibraryA = (pLoadLibraryA)resolve_by_hash(
        hKernel32, HASH_LOADLIBRARYA
    );

    pGetProcAddress GetProcAddress = (pGetProcAddress)resolve_by_hash(
        hKernel32, HASH_GETPROCADDRESS
    );

    // Load user32.dll
    HMODULE hUser32 = LoadLibraryA("user32.dll");

    // Resolve MessageBoxA
    typedef int (WINAPI *pMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
    pMessageBoxA MessageBoxA = (pMessageBoxA)resolve_by_hash(
        hUser32, HASH_MESSAGEBOXA
    );

    // Display message
    MessageBoxA(NULL, "Hello from PIC!", "Tradecraft Garden", MB_OK);
}
