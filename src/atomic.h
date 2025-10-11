#ifndef ATOMIC_H
#define ATOMIC_H

/* Work-around for LVGL's FreeRTOS integration, which includes "atomic.h" */

// Try to include the ESP-IDF atomic header if available
#if __has_include(<freertos/atomic.h>)
#include <freertos/atomic.h>
#else
// If not available, provide minimal stubs or error
#warning "<freertos/atomic.h> not found. Provide atomic operation stubs or update ESP-IDF."
// Minimal fallback: no-op atomics (NOT thread safe, but allows build)
#define Atomic_Increment_u32(ptr) (++(*(ptr)))
#define ATOMIC_COMPARE_AND_SWAP_SUCCESS 1
static inline int Atomic_CompareAndSwap_u32(volatile uint32_t *ptr, uint32_t newval, uint32_t oldval) {
    if (*ptr == oldval) { *ptr = newval; return 1; } else { return 0; }
}
#endif

#endif // ATOMIC_H

