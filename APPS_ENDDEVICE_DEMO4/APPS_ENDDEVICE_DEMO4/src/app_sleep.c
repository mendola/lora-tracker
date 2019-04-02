#include "lorawan.h"
#include "pmm.h"
#include  "conf_pmm.h"
#include "radio_driver_hal.h"
#include "sio2host.h"
#include "app_sleep.h"
#include "atomic.h"

uint32_t sleep_time_ms_ = 15000; 

extern void appWakeup(uint32_t sleptDuration);
extern void app_resources_uninit(void);
void SYSTEM_Kill(void);

void set_sleep_time_ms(uint32_t sleep_time) {
    sleep_time_ms_ = sleep_time;
}

AppTaskState_t sleep(void) {
	AppTaskState_t next_state = APP_STATE_GO_TO_SLEEP;
    static bool deviceResetsForWakeup = false;
    PMM_SleepReq_t sleepReq;

    /* Put the application to sleep */
    sleepReq.sleepTimeMs = sleep_time_ms_;
    sleepReq.pmmWakeupCallback = appWakeup;
    sleepReq.sleep_mode = SLEEP_MODE_STANDBY; //SLEEP_MODE_BACKUP; //SLEEP_MODE_STANDBY //CONF_PMM_SLEEPMODE_WHEN_IDLE;
	
    if (true == LORAWAN_ReadyToSleep(false))
    {
        app_resources_uninit();
		//SYSTEM_Kill();
        if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq))
        {
            HAL_Radio_resources_init();
            uart_init();
            set_app_state(APP_STATE_GO_TO_SLEEP);
            appPostTask(PROCESS_TASK_HANDLER);
            //printf("\r\nsleep_not_ok\r\n");	
        } else { // Sleep sucess
			printf("\r\nSLEEPY\r\n");
			next_state = APP_STATE_LISTEN_GPS_OFF;
		}
    }
    else
    {
        //printf("\r\nsleep_not_ok\r\n");
        appPostTask(PROCESS_TASK_HANDLER);
    }
	return next_state;
}
