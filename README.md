# ticker
Ticker is a naive embedded scheduler for HRT applications in just one include file. Code has currently been ported successfully to STM32, SPC5 and PICmicro families.

### sample code
```C
/**
 * @file main.c
 * @brief Example implementation file demonstrating the usage of ticker.h.
 *
 * This file simulates the main loop of an embedded system (like an STM32
 * using the HAL library) and defines the tasks executed by the ticker.
 */

#include "ticker.h" // Include the optimized header
#include <stdio.h>  // For printf/logging in a simulated environment
#include <stdbool.h> // For bool type

// --- Simulation of Platform Functions ---

// Global counter to simulate the system tick (like HAL_GetTick())
static uint32_t simulated_tick = 0;

// Since GetSysCount() is defined as HAL_GetTick() in ticker.h,
// we must define a dummy HAL_GetTick() function for a simple simulation.
uint32_t HAL_GetTick(void) {
    return simulated_tick;
}

// --- Task Definitions ---

/**
 * @brief A periodic task that toggles an imaginary LED every 1 second.
 */
task_return_t PeriodicToggleTask(struct ticker_s *t, void *arg) {
    static bool led_state = false;
    led_state = !led_state;

    printf("TICKER: [%lu ms] Periodic task: LED is now %s\n",
           t->now, led_state ? "ON" : "OFF");

    // Must return TASK_REPEAT for periodic tasks
    return TASK_REPEAT;
}

/**
 * @brief A one-shot task that executes once and cancels itself.
 */
task_return_t OneShotPrintTask(struct ticker_s *t, void *arg) {
    printf("TICKER: [%lu ms] One-shot task executed! Cancelling now.\n", t->now);

    // Returns TASK_DONE, automatically cancelling the task
    return TASK_DONE;
}

// --- Main Application ---

// Global ticker instance
ticker_t app_ticker;

// Task ID constants
enum {
    TASK_ID_TOGGLE = 0,
    TASK_ID_ONESHOT = 1
};

void main_app_init(void) {
    printf("--- Ticker Application Initializing ---\n");
    tickerInit(&app_ticker);

    // 1. Schedule the periodic LED toggle (every 1000 ms = 1 second)
    tickerScheduleTaskMs(
        &app_ticker,
        TASK_ID_TOGGLE,
        1000,
        PeriodicToggleTask,
        NULL,
        TASK_FLAG_PERIODIC
    );
    printf("Scheduled PeriodicToggleTask (1000ms) at slot %d.\n", TASK_ID_TOGGLE);

    // 2. Schedule a one-shot task to run after 5 seconds
    tickerScheduleTaskSec(
        &app_ticker,
        TASK_ID_ONESHOT,
        5, // 5 seconds
        OneShotPrintTask,
        NULL,
        TASK_FLAG_ONESHOT
    );
    printf("Scheduled OneShotPrintTask (5s) at slot %d.\n", TASK_ID_ONESHOT);
    printf("---------------------------------------\n");
}

void main_app_loop(void) {
    // 1. Check for high-frequency events (e.g., 50 Hz = 20 ms)
    if (tickerIsHz50(&app_ticker)) {
        // This is a time-critical check that doesn't need task overhead.
        // E.g., read a sensor, check buttons, etc.
        // printf("50 Hz event triggered.\n");
    }

    // 2. Check for low-frequency events (e.g., 5 Hz = 200 ms)
    if (tickerIsHz5(&app_ticker)) {
        // E.g., update a display, check battery status, etc.
        // printf("5 Hz event triggered.\n");
    }

    // 3. Process the scheduled tasks
    tickerTick(&app_ticker);
}

/**
 * @brief The main function (simulated embedded main).
 */
int main(void) {
    main_app_init();

    // Simulate time passing in 10ms steps for 12 seconds
    const uint32_t SIMULATION_DURATION_MS = 12000;
    const uint32_t STEP_MS = 10;

    while (simulated_tick < SIMULATION_DURATION_MS) {
        // Advance the time
        simulated_tick += STEP_MS;

        // Run the main application loop
        main_app_loop();
    }

    printf("--- Simulation finished at %lu ms ---\n", simulated_tick);
    
    // Check if the periodic task is still active
    if (tickerTaskIsPending(&app_ticker, TASK_ID_TOGGLE)) {
        printf("RESULT: Periodic task is still running.\n");
    }
    // Check if the one-shot task is canceled (it should be)
    if (!tickerTaskIsPending(&app_ticker, TASK_ID_ONESHOT)) {
        printf("RESULT: One-shot task was successfully canceled.\n");
    }

    return 0;
}
```
