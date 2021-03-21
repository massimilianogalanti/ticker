/*
 * ticker.h
 *
 *  v1.2
 *
 *  MIT License
 *  Massimiliano Galanti <massimiliano.galanti@gmail.com>
 *
 *  Description:
 *
 *  A simple collection of functions and data types to help implement real-time (frequency based) and delayed/repeated tasks.
 *
 *  Example:
 *
 *  ticker_t mainTicker;
 *  ...
 *  tickerInit(&mainTicker);
 *  ...
 *  while (1) {
 *  	tickerTick(&mainTicker);
 *
 *  	if (mainTicker.Hz5) {
 *  		...blink led...
 *  	}
 *  }
 */

#ifndef TICKER_H_
#define TICKER_H_

#ifndef GetSysCount
/* Platform dependent */

#include "main.h"

#define GetSysCount() HAL_GetTick()
#define SYSCNTxMS 1
#endif
/* Platform Independent */

#define syscntPERIOD_Hz1    (1000 * SYSCNTxMS)
#define syscntPERIOD_Hz2    ( 500 * SYSCNTxMS)
#define syscntPERIOD_Hz5    ( 200 * SYSCNTxMS)
#define syscntPERIOD_Hz10   ( 100 * SYSCNTxMS)
#define syscntPERIOD_Hz20   (  50 * SYSCNTxMS)
#define syscntPERIOD_Hz50   (  20 * SYSCNTxMS)
#define syscntPERIOD_Hz100  (  10 * SYSCNTxMS)
#define syscntPERIOD_Hz200  (   5 * SYSCNTxMS)
#define syscntPERIOD_Hz500  (   2 * SYSCNTxMS)
#define syscntPERIOD_Hz1000  (   1 * SYSCNTxMS)

struct ticker_s;

typedef enum { TASK_DONE = 0, TASK_REPEAT, TASK_ERROR } task_return_t;
typedef task_return_t cb(struct ticker_s*, void *);
typedef int task_id_t;

#define TASK_FLAG_NONE 0
#define TASK_FLAG_PERIODIC 1
typedef uint8_t task_flags_t;

typedef struct {
	task_id_t id;
	task_flags_t flags;
	void *arg;
	cb *func;
	uint32_t exp;
	uint32_t interval;
} ticker_task_t;

#define TICKER_MAX_TASKS 8
typedef struct ticker_s {
	uint32_t now;
	uint32_t tick1;
	uint32_t tick2;
	uint32_t tick5;
	uint32_t tick10;
	uint32_t tick20;
	uint32_t tick50;
	uint32_t tick100;
	uint32_t tick200;
	uint32_t tick500;
	uint32_t tick1000;

	uint8_t Hz1 :1;
	uint8_t Hz2 :1;
	uint8_t Hz5 :1;
	uint8_t Hz10 :1;
	uint8_t Hz20 :1;
	uint8_t Hz50 :1;
	uint8_t Hz100 :1;
	uint8_t Hz200 :1;
	uint8_t Hz500 :1;
	uint8_t Hz1000 :1;

	ticker_task_t tasks[TICKER_MAX_TASKS];
} ticker_t;

static inline void tickerInit(ticker_t *t) {
	t->now = t->tick1 = t->tick2 = t->tick5 = t->tick10 = t->tick20 = t->tick50 = t->tick100 = t->tick200 = t->tick500 = t->tick1000 =
	GetSysCount();
}

#define DIFFU32(x, y) (uint32_t) (x - y)

static inline void tickerDelayMs(uint32_t val, void (*wd)(void)) {
	uint32_t now = GetSysCount();

	while (DIFFU32(GetSysCount(), now) < val * SYSCNTxMS)
		if (wd)
			wd();
}

static inline task_id_t tickerScheduleTaskMs(ticker_t *t, uint32_t val, cb *func, void *arg, task_flags_t flags) {
	if (t && func) {
		for (task_id_t i = 0; i < TICKER_MAX_TASKS; i++) {
			if (0 == t->tasks[i].func) {
				t->tasks[i].interval = val;
				t->tasks[i].exp = t->now + t->tasks[i].interval * SYSCNTxMS;
				t->tasks[i].func = func;
				t->tasks[i].arg = arg;
				t->tasks[i].id = i;
				t->tasks[i].flags = flags;
				return i;
			}
		}
	}
	return -1;
}

static inline void tickerTick(ticker_t *t) {
	t->now = GetSysCount();

	for (task_id_t i = 0; i < TICKER_MAX_TASKS; i++) {
		if (t->tasks[i].func && t->now >= t->tasks[i].exp) {
			task_return_t res = t->tasks[i].func(t, t->tasks[i].arg);
			if (t->tasks[i].flags & TASK_FLAG_PERIODIC && TASK_REPEAT == res) {
				t->tasks[i].exp = t->now + t->tasks[i].interval * SYSCNTxMS;
			} else {
				t->tasks[i].func = 0;
			}
		}
	}

	t->Hz1 = t->Hz2 =t->Hz5 = t->Hz10 = t->Hz20 = t->Hz50 = t->Hz100 = t->Hz200 = t->Hz500 = t->Hz1000 = 0;

	if (DIFFU32(t->now, t->tick1000) >= syscntPERIOD_Hz1000) {
		t->Hz1000 = 1;
		t->tick1000 += syscntPERIOD_Hz1000;
	}

	if (DIFFU32(t->now, t->tick500) >= syscntPERIOD_Hz500) {
		t->Hz500 = 1;
		t->tick500 += syscntPERIOD_Hz500;
	}

	if (DIFFU32(t->now, t->tick200) >= syscntPERIOD_Hz200) {
		t->Hz200 = 1;
		t->tick200 += syscntPERIOD_Hz200;
	}

	if (DIFFU32(t->now, t->tick100) >= syscntPERIOD_Hz100) {
		t->Hz100 = 1;
		t->tick100 += syscntPERIOD_Hz100;
	}

	if (DIFFU32(t->now, t->tick50) >= syscntPERIOD_Hz50) {
		t->Hz50 = 1;
		t->tick50 += syscntPERIOD_Hz50;
	}

	if (DIFFU32(t->now, t->tick20) >= syscntPERIOD_Hz20) {
		t->Hz20 = 1;
		t->tick20 += syscntPERIOD_Hz20;
	}

	if (DIFFU32(t->now, t->tick10) >= syscntPERIOD_Hz10) {
		t->Hz10 = 1;
		t->tick10 += syscntPERIOD_Hz10;
	}

	if (DIFFU32(t->now, t->tick5) >= syscntPERIOD_Hz5) {
		t->Hz5 = 1;
		t->tick5 += syscntPERIOD_Hz5;
	}

	if (DIFFU32(t->now, t->tick2) >= syscntPERIOD_Hz2) {
		t->Hz2 = 1;
		t->tick2 += syscntPERIOD_Hz2;
	}


	if (DIFFU32(t->now, t->tick1) >= syscntPERIOD_Hz1) {
		t->Hz1 = 1;
		t->tick1 += syscntPERIOD_Hz1;
	}
}

#endif /* TICKER_H_ */
