/*
 * Simple PICO Capability Example
 *
 * Demonstrates a PICO capability with:
 * - Entry point (go)
 * - Multiple exported functions
 * - Resource access
 *
 * Build with Crystal Palace:
 *   load "simple_pico_capability.x64.o"
 *     exportfunc "capability_init" "__tag_init"
 *     exportfunc "capability_execute" "__tag_execute"
 *     exportfunc "capability_cleanup" "__tag_cleanup"
 *     make pico +optimize
 *   load "config.bin"
 *     append $PICO
 *   link "simple_capability.pico"
 */

#include <windows.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// Access appended configuration (if any)
extern unsigned char _binary_config_bin_start[];
extern unsigned int  _binary_config_bin_size;

typedef struct {
    DWORD sleep_time;
    DWORD max_iterations;
    char target_process[256];
} CONFIG;

CONFIG g_config = {0};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Simple string length
int my_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

// Simple string compare
int my_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a - *b;
}

// Simple memory copy
void* my_memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// ============================================================================
// CAPABILITY FUNCTIONS
// ============================================================================

// Initialize capability
int capability_init(void) {
    // Parse configuration if available
    if (_binary_config_bin_size >= sizeof(CONFIG)) {
        my_memcpy(&g_config, _binary_config_bin_start, sizeof(CONFIG));
    } else {
        // Default configuration
        g_config.sleep_time = 1000;
        g_config.max_iterations = 10;
        my_memcpy(g_config.target_process, "explorer.exe", 13);
    }

    return 1;  // Success
}

// Execute capability
int capability_execute(void) {
    // Example: Simple beacon-like behavior

    for (DWORD i = 0; i < g_config.max_iterations; i++) {
        // Perform capability action
        // (In real capability: check tasks, execute, report back)

        // Sleep between iterations
        Sleep(g_config.sleep_time);

        // Simple counter check
        if (i >= g_config.max_iterations - 1) {
            break;
        }
    }

    return 1;  // Success
}

// Cleanup capability
int capability_cleanup(void) {
    // Clean up any resources
    // Zero out configuration
    my_memcpy(&g_config, "\x00", sizeof(CONFIG));

    return 1;  // Success
}

// ============================================================================
// PICO ENTRY POINT
// ============================================================================

// Main entry point - executes full capability lifecycle
void go(void) {
    // Initialize
    if (!capability_init()) {
        return;  // Initialization failed
    }

    // Execute capability
    capability_execute();

    // Cleanup
    capability_cleanup();
}

// ============================================================================
// ADVANCED EXAMPLE: Process Enumeration PICO
// ============================================================================

typedef struct {
    DWORD process_count;
    DWORD process_ids[256];
    char process_names[256][MAX_PATH];
} PROCESS_LIST;

// Enumerate processes (simplified example)
int enumerate_processes(PROCESS_LIST* list) {
    // In real implementation:
    // 1. Use CreateToolhelp32Snapshot
    // 2. Walk process list
    // 3. Collect PIDs and names
    // 4. Store in list structure

    // For now, just return success
    list->process_count = 0;
    return 1;
}

// Find process by name
DWORD find_process_by_name(const char* name) {
    PROCESS_LIST list;

    if (!enumerate_processes(&list)) {
        return 0;
    }

    for (DWORD i = 0; i < list.process_count; i++) {
        if (my_strcmp(list.process_names[i], name) == 0) {
            return list.process_ids[i];
        }
    }

    return 0;  // Not found
}

// Alternate entry point for process enumeration PICO
void go_enumerate(void) {
    DWORD target_pid = find_process_by_name(g_config.target_process);

    if (target_pid) {
        // Found target process
        // Perform actions...
    }
}

// ============================================================================
// NOTES ON PICO DEVELOPMENT
// ============================================================================

/*
 * PICO Best Practices:
 *
 * 1. Keep entry point (go) simple and focused
 * 2. Export modular functions for flexibility
 * 3. Handle errors gracefully (no exceptions)
 * 4. Minimize dependencies (avoid CRT)
 * 5. Use resources for configuration/data
 * 6. Clean up after execution
 * 7. Return status codes from exported functions
 * 8. Document exported function signatures
 *
 * Memory Management:
 * - PICOs support global variables (unlike PIC shared libs)
 * - Each PICO instance has its own .bss
 * - Use fixbss if needed for globals
 *
 * Size Optimization:
 * - Use +optimize to remove unused code
 * - Avoid large static arrays
 * - Use shared libraries for common code
 *
 * Signature Evasion:
 * - Use +mutate for unique instances
 * - Combine with +disco for function randomization
 * - Encrypt resources before appending
 */
