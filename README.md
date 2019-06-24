# ticker
Very tiny embedded scheduler for HRT applications in just one include file.

### Platform requirements
You need to adapt the code to your hardware platform by simply adding the proper include(s) and defining two macros. Code has currently been ported successfully to STM32, SPC5 and PICmicro platforms.
```
/* Platform dependent */

#include "stm32f4xx_hal.h"

#define GetSysCount() HAL_GetTick()
#define SYSCNTxMS 1
#endif
```

### Setup
```C
#include "ticker.h"
...
static ticker_t mainTicker;
...
tickerInit(&mainTicker);
```
### Basic usage in main loop
This is the most simple usage, just needs to add proper checks in your main loop.
```C
while (1) {
  tickerTick(&mainTicker);
  
  if (mainTicker.Hz1) {
    // toggle led1 @ 1Hz
  }
  
  if (mainTicker.Hz100) {
    // toggle led2 @ 100Hz
  }
  ...
}
```
### Callback mode
Same effect can be obtained using callbacks (note that this syntax uses function pointers).
```C
static task_return_t myTaskOneShot(ticker_t *t, void *pdata)
{
  // do something here

  return TASK_DONE;
}

static task_return_t myTaskPeriodic(ticker_t *t, void *pdata)
{
  // do something here

  return TASK_REPEAT;
}
...
int pdata1 = 7;
char pdata[] = "hello";
...
tickerScheduleTaskMs(&mainTicker, 1000 /* ms */, myTaskOneShot, pdata1, TASK_FLAG_NONE);
tickerScheduleTaskMs(&mainTicker, 100 /* ms */, myTaskPeriodic, pdata2, TASK_FLAG_PERIODIC);

while (1) {
  tickerTick(&mainTicker);
  
  if (mainTicker.Hz100) {
    // this is still valid if you want to mix usages
  }
  ...
}
```
