#include "lorawan.h"
#include "pmm.h"
#include  "conf_pmm.h"

uint32_t sleep_time_ms_ = 120000000;  // 2 minutes

extern void appWakeup(uint32_t sleptDuration);

static void app_resources_uninit(void){
    /* Disable USART TX and RX Pins */
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.powersave  = true;
    port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
    port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
    /* Disable UART module */
    uart_usb_deinit();
    /* Disable Transceiver SPI Module */
    HAL_RadioDeInit();
}

void set_sleep_time_ms(uint32_t sleep_time) {
    sleep_time_ms_ = sleep_time;
}

void sleep(void) {
    static bool deviceResetsForWakeup = false;
    PMM_SleepReq_t sleepReq;

    /* Put the application to sleep */
    sleepReq.sleepTimeMs = sleep_time_ms_;
    sleepReq.pmmWakeupCallback = appWakeup;
    sleepReq.sleep_mode = CONF_PMM_SLEEPMODE_WHEN_IDLE;
    if (CONF_PMM_SLEEPMODE_WHEN_IDLE == SLEEP_MODE_STANDBY)
    {
        deviceResetsForWakeup = false;
    }
    if (true == LORAWAN_ReadyToSleep(deviceResetsForWakeup))
    {
        app_resources_uninit();
        if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq))
        {
            HAL_Radio_resources_init();
            sio2host_init();
            appTaskState = JOIN_SEND_STATE;
            appPostTask(HEARTBEAT_TASK_HANDLER);
            printf("\r\nsleep_not_ok\r\n");	
        }
    }
    else
    {
        printf("\r\nsleep_not_ok\r\n");
        appTaskState = JOIN_SEND_STATE;
        appPostTask(HEARTBEAT_TASK_HANDLER);
    }
}
