/**
 * @file ticker.h
 * @brief A simple collection of functions and data types to help implement real-time (frequency based) and delayed/repeated tasks.
 *
 * @version 1.5
 *
 * @copyright MIT License
 * @author Massimiliano Galanti <massimilianogalanti@gmail.com>
 *
 * Example:
 * @code
 * ticker_t mainTicker;
 * // ...
 * tickerInit(&mainTicker);
 * // ...
 * while (1) {
 * tickerTick(&mainTicker);
 *
 * if (tickerIsHz5(&mainTicker)) {
 * // ...blink led...
 * }
 * }
 * @endcode
 */

#ifndef TICKER_H_
#define TICKER_H_

#include <stdint.h> // Standard integer types
#include <stddef.h> // For size_t and NULL

// --- Platform Configuration ---
/** @defgroup platform_config Platform Configuration
 * @{
 */
#ifndef GetSysCount
    /**
     * @brief Platform-specific include for system time retrieval (e.g., HAL_GetTick).
     */
    #include "main.h"

    /**
     * @brief Macro to retrieve the current system tick count.
     * @return The current system time in milliseconds (or platform equivalent).
     */
    #define GetSysCount() HAL_GetTick()

    /**
     * @brief System count units per millisecond.
     *
     * Defines the multiplier to convert millisecond values to system count units.
     * Typically 1 if GetSysCount() returns milliseconds.
     */
    #define SYSCNTxMS 1U
#endif
/** @} */ // end of platform_config

// --- Constants & Types ---

/** @defgroup constants Constants
 * @{
 */
/**
 * @brief Frequency period constants in system count units (e.g., milliseconds).
 *
 * Defines the required time difference for each standard frequency.
 */
enum {
    PERIOD_05_HZ   = 2000U * SYSCNTxMS, ///< Period for 0.5 Hz (2000 ms)
    PERIOD_1_HZ    = 1000U * SYSCNTxMS, ///< Period for 1 Hz (1000 ms)
    PERIOD_2_HZ    = 500U * SYSCNTxMS,  ///< Period for 2 Hz (500 ms)
    PERIOD_5_HZ    = 200U * SYSCNTxMS,  ///< Period for 5 Hz (200 ms)
    PERIOD_10_HZ   = 100U * SYSCNTxMS,  ///< Period for 10 Hz (100 ms)
    PERIOD_20_HZ   = 50U * SYSCNTxMS,   ///< Period for 20 Hz (50 ms)
    PERIOD_50_HZ   = 20U * SYSCNTxMS,   ///< Period for 50 Hz (20 ms)
    PERIOD_100_HZ  = 10U * SYSCNTxMS,   ///< Period for 100 Hz (10 ms)
    PERIOD_200_HZ  = 5U * SYSCNTxMS,    ///< Period for 200 Hz (5 ms)
    PERIOD_500_HZ  = 2U * SYSCNTxMS,    ///< Period for 500 Hz (2 ms)
    PERIOD_1000_HZ = 1U * SYSCNTxMS,    ///< Period for 1000 Hz (1 ms)
    TICKER_NUM_FREQUENCIES             ///< Helper constant: Total number of supported frequencies.
};

/**
 * @brief Array of all frequency periods in system count units.
 *
 * This array is used internally by tickerTick() for loop-based calculation.
 * The order must match the enum constants above.
 */
static const uint32_t TICKER_PERIODS[] = {
    PERIOD_05_HZ, PERIOD_1_HZ, PERIOD_2_HZ, PERIOD_5_HZ, PERIOD_10_HZ,
    PERIOD_20_HZ, PERIOD_50_HZ, PERIOD_100_HZ, PERIOD_200_HZ, PERIOD_500_HZ,
    PERIOD_1000_HZ
};

/**
 * @brief Defines array indexes for accessing frequency-related data.
 */
enum {
    TICKER_IDX_05_HZ = 0, TICKER_IDX_1_HZ, TICKER_IDX_2_HZ, TICKER_IDX_5_HZ,
    TICKER_IDX_10_HZ, TICKER_IDX_20_HZ, TICKER_IDX_50_HZ, TICKER_IDX_100_HZ,
    TICKER_IDX_200_HZ, TICKER_IDX_500_HZ, TICKER_IDX_1000_HZ
};
/** @} */ // end of constants

/** @defgroup macros Macros and Helpers
 * @{
 */
/**
 * @brief Calculates the time difference between two unsigned 32-bit timestamps.
 *
 * This macro correctly handles the 32-bit integer wrap-around (roll-over)
 * characteristic of typical system counters.
 *
 * @param now The current timestamp.
 * @param prev The previous timestamp.
 * @return The time elapsed from @p prev to @p now.
 */
#define TIME_DIFFERENCE(now, prev) ((uint32_t)((now) - (prev)))
/** @} */ // end of macros

/** @defgroup task_management Task Management
 * @{
 */

/**
 * @brief Return codes for a task callback function.
 */
typedef enum {
    TASK_DONE = 0,      ///< Task finished; cancel scheduling.
    TASK_REPEAT,        ///< Task finished but should be rescheduled (for periodic tasks).
    TASK_ERROR          ///< Task failed (implementation dependent).
} task_return_t;

/** Forward declaration of the main ticker structure. */
struct ticker_s;

/**
 * @brief Function pointer type for a task callback.
 * @param t Pointer to the current ticker instance.
 * @param arg User-defined argument passed during scheduling.
 * @return A value from the task_return_t enumeration.
 */
typedef task_return_t (*cb_t)(struct ticker_s*, void*);

/**
 * @brief Type definition for a task ID.
 *
 * Optimized to 8-bit signed integer since MAX_TASKS is small.
 */
typedef int8_t task_id_t;

/**
 * @brief Task behavior flags.
 */
typedef enum {
    TASK_FLAG_NONE = 0,     ///< Default: One-shot task.
    TASK_FLAG_ONESHOT = TASK_FLAG_NONE, ///< Alias for one-shot task.
    TASK_FLAG_PERIODIC = 0x01 ///< Task will repeat until it returns TASK_DONE.
} task_flags_t;

/**
 * @brief Maximum number of supported scheduled tasks.
 */
#define TICKER_MAX_TASKS 8

/**
 * @brief Structure defining a single scheduled task.
 */
typedef struct {
    task_id_t id;           ///< The task's index/ID in the tasks array.
    task_flags_t flags;     ///< Behavior flags (e.g., periodic, one-shot).
    void *arg;              ///< User-defined argument passed to the callback function.
    cb_t func;              ///< Pointer to the task function. NULL if slot is free.
    uint32_t exp;           ///< Expiration time (system count units) for the task.
    uint32_t interval;      ///< Interval between runs (in milliseconds).
} ticker_task_t;

/**
 * @brief The main Ticker instance structure.
 *
 * Holds the current time, frequency flags, historical tick times, and scheduled tasks.
 */
typedef struct ticker_s {
    uint32_t now;           ///< The most recent system count recorded by tickerTick().
    uint16_t Hz_flags;      ///< Bitmask where each bit indicates a frequency has just ticked.
    uint32_t tick_history[TICKER_NUM_FREQUENCIES]; ///< Last time each frequency was registered.
    ticker_task_t tasks[TICKER_MAX_TASKS]; ///< Array of scheduled tasks.
} ticker_t;
/** @} */ // end of task_management


/** @defgroup api Public API
 * @{
 */

// --- Flag Accessors ---
/**
 * @brief Internal macro to check if a specific frequency flag bit is set.
 * @param t Pointer to the ticker_t instance.
 * @param idx The index (0-10) corresponding to the desired frequency flag.
 * @return Non-zero if the flag is set (i.e., the frequency just ticked), 0 otherwise.
 */
#define GET_HZ_FLAG(t, idx) ((t)->Hz_flags & (1U << (idx)))

/**
 * @brief Checks if the 0.5 Hz flag is set (ticked once every 2 seconds).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz05(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_05_HZ); }

/**
 * @brief Checks if the 1 Hz flag is set (ticked once every 1 second).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz1(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_1_HZ); }

/**
 * @brief Checks if the 2 Hz flag is set (ticked once every 500 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz2(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_2_HZ); }

/**
 * @brief Checks if the 5 Hz flag is set (ticked once every 200 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz5(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_5_HZ); }

/**
 * @brief Checks if the 10 Hz flag is set (ticked once every 100 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz10(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_10_HZ); }

/**
 * @brief Checks if the 20 Hz flag is set (ticked once every 50 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz20(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_20_HZ); }

/**
 * @brief Checks if the 50 Hz flag is set (ticked once every 20 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz50(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_50_HZ); }

/**
 * @brief Checks if the 100 Hz flag is set (ticked once every 10 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz100(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_100_HZ); }

/**
 * @brief Checks if the 200 Hz flag is set (ticked once every 5 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz200(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_200_HZ); }

/**
 * @brief Checks if the 500 Hz flag is set (ticked once every 2 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz500(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_500_HZ); }

/**
 * @brief Checks if the 1000 Hz flag is set (ticked once every 1 ms).
 * @param t Pointer to the ticker_t instance.
 * @return 1 if ticked, 0 otherwise.
 */
static inline int tickerIsHz1000(const ticker_t *t) { return GET_HZ_FLAG(t, TICKER_IDX_1000_HZ); }


// --- Core Functions ---

/**
 * @brief Initializes the ticker structure.
 *
 * Sets the current time and all historical tick times to the system's current count.
 * @param t Pointer to the ticker_t instance to initialize.
 */
static inline void tickerInit(ticker_t *t) {
    uint32_t current_time = GetSysCount();
    t->now = current_time;
    t->Hz_flags = 0;
    for (size_t i = 0; i < TICKER_NUM_FREQUENCIES; i++) {
        t->tick_history[i] = current_time;
    }
}

/**
 * @brief Provides a blocking delay with an optional watchdog reset function.
 *
 * @warning This function is blocking and consumes CPU cycles while waiting.
 * @param val Delay duration in milliseconds.
 * @param wd Optional pointer to a function to call inside the loop (e.g., watchdog reset). Can be NULL.
 */
static inline void tickerDelayMs(uint32_t val, void (*wd)(void)) {
    uint32_t now = GetSysCount();
    uint32_t required_diff = val * SYSCNTxMS;

    while (TIME_DIFFERENCE(GetSysCount(), now) < required_diff)
        if (wd)
            wd();
}

/**
 * @brief Schedules a task to run after a specified delay in milliseconds.
 *
 * @param t Pointer to the ticker_t instance.
 * @param i The slot index (ID) to use for the task (0 to TICKER_MAX_TASKS - 1).
 * @param val The time interval in milliseconds.
 * @param func The callback function to execute.
 * @param arg A user-defined argument passed to the callback.
 * @param flags Task behavior flags (TASK_FLAG_ONESHOT or TASK_FLAG_PERIODIC).
 * @return The task ID on success (i), -1 for bad arguments, or -2 if the slot is occupied.
 */
static inline task_id_t tickerScheduleTaskMs(ticker_t *t, task_id_t i,
    uint32_t val, cb_t func, void *arg, task_flags_t flags) {

    if (t == NULL || func == NULL || i < 0 || i >= TICKER_MAX_TASKS) {
        return -1;
    }
    if (t->tasks[i].func != NULL) {
        return -2;
    }

    t->tasks[i].interval = val;
    t->tasks[i].exp = t->now + (t->tasks[i].interval * SYSCNTxMS);
    t->tasks[i].func = func;
    t->tasks[i].arg = arg;
    t->tasks[i].id = i;
    t->tasks[i].flags = flags;
    return i;
}

/**
 * @brief Schedules a task to run after a specified delay in seconds.
 *
 * A convenience wrapper around tickerScheduleTaskMs().
 * @param t Pointer to the ticker_t instance.
 * @param i The slot index (ID) to use for the task.
 * @param val_sec The time interval in seconds.
 * @param func The callback function to execute.
 * @param arg A user-defined argument passed to the callback.
 * @param flags Task behavior flags.
 * @return The task ID on success (i), -1 for bad arguments, or -2 if the slot is occupied.
 */
static inline task_id_t tickerScheduleTaskSec(ticker_t *t, task_id_t i,
    uint32_t val_sec, cb_t func, void *arg, task_flags_t flags) {
    return tickerScheduleTaskMs(t, i, val_sec * 1000U, func, arg, flags);
}

/**
 * @brief Checks if a specific task slot is currently active and pending execution.
 * @param t Pointer to the ticker_t instance.
 * @param id The task ID/slot index to check.
 * @return Non-zero (true) if pending, 0 (false) otherwise.
 */
static inline int tickerTaskIsPending(ticker_t *t, task_id_t id) {
    if (id >= 0 && id < TICKER_MAX_TASKS) {
        return (t->tasks[id].func != NULL);
    }
    return 0;
}

/**
 * @brief Immediately cancels a scheduled task by clearing its function pointer.
 * @param t Pointer to the ticker_t instance.
 * @param id The task ID/slot index to cancel.
 */
static inline void tickerCancelTask(ticker_t *t, task_id_t id) {
    if (id >= 0 && id < TICKER_MAX_TASKS) {
        t->tasks[id].func = NULL;
    }
}

/**
 * @brief The main execution loop function. Must be called repeatedly and frequently.
 *
 * This function updates the system time, executes expired scheduled tasks,
 * and sets the frequency flags (e.g., t->Hz_flags) for the current cycle.
 *
 * @param t Pointer to the ticker_t instance.
 */
static inline void tickerTick(ticker_t *t) {
    t->now = GetSysCount();

    // Process scheduled tasks
    for (task_id_t i = 0; i < TICKER_MAX_TASKS; i++) {
        // Check if the slot is active AND time has expired (handling roll-over)
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

    // Process fixed-frequency ticks
    t->Hz_flags = 0; // Reset all flags in one instruction

    for (size_t i = 0; i < TICKER_NUM_FREQUENCIES; i++) {
        // Time elapsed since last tick > required period?
        if (TIME_DIFFERENCE(t->now, t->tick_history[i]) >= TICKER_PERIODS[i]) {
            t->Hz_flags |= (1U << i);        // Set the corresponding bit flag
            t->tick_history[i] += TICKER_PERIODS[i]; // Advance the timer
        }
    }
}
/** @} */ // end of api

#endif /* TICKER_H_ */
