#ifndef LORA_H
#define LORA_H

#include "lorawan.h"
#include "radio_interface.h"
#include "application_tasks.h"

void lora_init(void);
void radio_tx_callback(void);
void SetRadioSettings(void);
void demo_appdata_callback(void *appHandle, appCbParams_t *data);
void lora_send_location(void);
AppTaskState_t lora_listen_for_cmd(void);

#endif // LORA_H