#ifndef LORA_H
#define LORA_H

#include "lorawan.h"
#include "radio_interface.h"

void lora_init(void);
void radio_tx_callback(void);
void SetRadioSettings(void);
void demo_appdata_callback(void *appHandle, appCbParams_t *data);

#endif // LORA_H