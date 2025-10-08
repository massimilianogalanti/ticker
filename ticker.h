/*
 * ticker.h
 *
 * v1.5 (Modern C, Highly Compact)
 *
 * MIT License
 * Massimiliano Galanti <massimilianogalanti@gmail.com>
 *
 * Description:
 * A simple collection of functions and data types to help implement real-time (frequency based) and delayed/repeated tasks.
 */

#ifndef TICKER_H_
#define TICKER_H_

#include <stdint.h> // Standard integer types
#include <stddef.h> // For size_t and NULL

// --- Platform Configuration ---
// The original dependency on "main.h" is preserved via a custom macro check.
#ifndef GetSysCount
    #include "main.h" // Platform-specific include (e.g., for HAL_GetTick)
    #define GetSysCount() HAL_GetTick()
    #define SYSCNTxMS 1U // Use 'U' suffix for unsigned constants
#endif

// --- Constants & Types ---

// 1. Use an anonymous enum for internal frequency constants for type safety.
// These are the periods in milliseconds.
enum {
    PERIOD_05_HZ   = 2000U * SYSCNTxMS,
    PERIOD_1_HZ    = 1000U * SYSCNTxMS,
    PERIOD_2_HZ    = 500U * SYSCNTxMS,
    PERIOD_5_HZ    = 200U * SYSCNTxMS,
    PERIOD_10_HZ   = 100U * SYSCNTxMS,
    PERIOD_20_HZ   = 50U * SYSCNTxMS,
    PERIOD_50_HZ   = 20U * SYSCNTxMS,
    PERIOD_100_HZ  = 10U * SYSCNTxMS,
    PERIOD_200_HZ  = 5U * SYSCNTxMS,
    PERIOD_500_HZ  = 2U * SYSCNTxMS,
    PERIOD_1000_HZ = 1U * SYSCNTxMS,
    TICKER_NUM_FREQUENCIES // 2. Helper constant for array sizing
};

// Array of periods, MUST match the order of the constants above.
static const uint32_t TICKER_PERIODS[] = {
    PERIOD_05_HZ, PERIOD_1_HZ, PERIOD_2_HZ, PERIOD_5_HZ, PERIOD_10_HZ,
    PERIOD_20_HZ, PERIOD_50_HZ, PERIOD_100_HZ, PERIOD_200_HZ, PERIOD_500_HZ,
    PERIOD_1000_HZ
};

// 3. Define the indexes for array access
enum {
    TICKER_IDX_05_HZ = 0, TICKER_IDX_1_HZ, TICKER_IDX_2_HZ, TICKER_IDX_5_HZ,
    TICKER_IDX_10_HZ, TICKER_IDX_20_HZ, TICKER_IDX_50_HZ, TICKER_IDX_100_HZ,
    TICKER_IDX_200_HZ, TICKER_IDX_500_HZ, TICKER_IDX_1000_HZ
};

// 4. Time difference macro using only unsigned integer subtraction for roll-over safety
#define TIME_DIFFERENCE(now, prev) ((uint32_t)((now) - (prev)))

typedef enum {
    TASK_DONE = 0, TASK_REPEAT, TASK_ERROR
} task_return_t;

// Forward declaration of the main struct
struct ticker_s;

typedef task_return_t (*cb_t)(struct ticker_s*, void*); // Use standard typedef naming convention
typedef int8_t task_id_t; // Optimized size (up to 127 tasks)

typedef enum {
    TASK_FLAG_NONE = 0,
    TASK_FLAG_ONESHOT = TASK_FLAG_NONE,
    TASK_FLAG_PERIODIC = 0x01 // Use a power-of-2 flag value
} task_flags_t;

#define TICKER_MAX_TASKS 8

typedef struct {
    task_id_t id;
    task_flags_t flags;
    void *arg;
    cb_t func; // Use the new typedef
    uint32_t exp;
    uint32_t interval;
} ticker_task_t;

// 5. Highly Compact Main Struct
typedef struct ticker_s {
    uint32_t now;
    uint16_t Hz_flags; // Bitmask for all frequency flags (11 bits needed)
    uint32_t tick_history[TICKER_NUM_FREQUENCIES]; // Array stores last tick time
    ticker_task_t tasks[TICKER_MAX_TASKS];
} ticker_t;

// --- Flag Accessors (Simplified & Consistent) ---
// Note: These expose the structure's state cleanly, replacing the old direct member access (t->Hz5).
#define GET_HZ_FLAG(t, idx) ((t)->Hz_flags & (1U << (idx)))

static inline int tickerIsHz05(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_05_HZ); }
static inline int tickerIsHz1(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_1_HZ); }
static inline int tickerIsHz2(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_2_HZ); }
static inline int tickerIsHz5(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_5_HZ); }
static inline int tickerIsHz10(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_10_HZ); }
static inline int tickerIsHz20(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_20_HZ); }
static inline int tickerIsHz50(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_50_HZ); }
static inline int tickerIsHz100(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_100_HZ); }
static inline int tickerIsHz200(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_200_HZ); }
static inline int tickerIsHz500(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_500_HZ); }
static inline int tickerIsHz1000(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_1000_HZ); }

// --- Static Inline Functions ---

static inline void tickerInit(ticker_t *t) {
    uint32_t current_time = GetSysCount();
    t->now = current_time;
    t->Hz_flags = 0;
    // 6. Use a loop for array initialization, more efficient and less code
    for (size_t i = 0; i < TICKER_NUM_FREQUENCIES; i++) {
        t->tick_history[i] = current_time;
    }
}

static inline void tickerDelayMs(uint32_t val, void (*wd)(void)) {
    uint32_t now = GetSysCount();
    // 7. Simplified and grouped multiplication for clarity
    uint32_t required_diff = val * SYSCNTxMS;

    while (TIME_DIFFERENCE(GetSysCount(), now) < required_diff)
        if (wd)
            wd();
}

static inline task_id_t tickerScheduleTaskMs(ticker_t *t, task_id_t i,
    uint32_t val, cb_t func, void *arg, task_flags_t flags) {

    if (t == NULL || func == NULL || i < 0 || i >= TICKER_MAX_TASKS) {
        return -1; // Standard error for bad arguments
    }
    if (t->tasks[i].func != NULL) {
        return -2; // Slot already taken
    }

    t->tasks[i].interval = val;
    t->tasks[i].exp = t->now + (t->tasks[i].interval * SYSCNTxMS);
    t->tasks[i].func = func;
    t->tasks[i].arg = arg;
    t->tasks[i].id = i;
    t->tasks[i].flags = flags;
    return i;
}

static inline int tickerTaskIsPending(ticker_t *t, task_id_t id) {
    if (id >= 0 && id < TICKER_MAX_TASKS) {
        return (t->tasks[id].func != NULL);
    }
    return 0;
}

static inline void tickerCancelTask(ticker_t *t, task_id_t id) {
    if (id >= 0 && id < TICKER_MAX_TASKS) {
        t->tasks[id].func = NULL;
    }
}

static inline void tickerTick(ticker_t *t) {
    t->now = GetSysCount();

    // 8. Process scheduled tasks
    for (task_id_t i = 0; i < TICKER_MAX_TASKS; i++) {
        if (t->tasks[i].func != NULL && TIME_DIFFERENCE(t->now, t->tasks[i].exp) <= 0) {
            task_return_t res = t->tasks[i].func(t, t->tasks[i].arg);
            
            if ((t->tasks[i].flags & TASK_FLAG_PERIODIC) && (res == TASK_REPEAT)) {
                // Prevent drift: calculate next exp relative to the *last* exp.
                t->tasks[i].exp += (t->tasks[i].interval * SYSCNTxMS);
            } else {
                t->tasks[i].func = NULL; // Cancel one-shot or non-TASK_REPEAT tasks
            }
        }
    }

    // 9. Process fixed-frequency ticks using a loop over the constant array
    t->Hz_flags = 0; // Reset all flags in one instruction

    for (size_t i = 0; i < TICKER_NUM_FREQUENCIES; i++) {
        // Time elapsed since last tick > required period?
        if (TIME_DIFFERENCE(t->now, t->tick_history[i]) >= TICKER_PERIODS[i]) {
            t->Hz_flags |= (1U << i);        // Set the corresponding bit flag
            t->tick_history[i] += TICKER_PERIODS[i]; // Advance the timer
        }
    }
}

// 10. Added seconds helper function
static inline task_id_t tickerScheduleTaskSec(ticker_t *t, task_id_t i,
    uint32_t val_sec, cb_t func, void *arg, task_flags_t flags) {
    return tickerScheduleTaskMs(t, i, val_sec * 1000U, func, arg, flags);
}

#endif /* TICKER_H_ */
