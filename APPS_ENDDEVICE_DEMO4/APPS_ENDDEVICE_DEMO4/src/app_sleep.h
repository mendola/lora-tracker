/*
 * sleep.h
 *
 * Created: 3/31/2019 7:43:34 PM
 *  Author: alexm
 */ 


#ifndef SLEEP_H_
#define SLEEP_H_

#include "application_tasks.h"

void set_sleep_time_ms(uint32_t sleep_time);
AppTaskState_t sleep(void);

#endif /* SLEEP_H_ */