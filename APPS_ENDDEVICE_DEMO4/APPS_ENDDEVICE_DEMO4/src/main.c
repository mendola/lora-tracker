/**
* \file  main.c
*
* \brief LORAWAN Demo Application main file
*		
*
* Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/
 
/****************************** INCLUDES **************************************/
#include "system_low_power.h"
#include "radio_driver_hal.h"
#include "sys.h"
#include "system_init.h"
#include "system_assert.h"
#include "aes_engine.h"
#include "application_tasks.h"
#include "sio2host.h"
#include "extint.h"
#include "conf_app.h"
#include "sw_timer.h"
#ifdef CONF_PMM_ENABLE
#include "sleep_timer.h"
#include "sleep.h"
#endif
#include "conf_sio2host.h"
#if (ENABLE_PDS == 1)
#include "pds_interface.h"
#endif
#if (CERT_APP == 1)
#include "conf_certification.h"
#include "enddevice_cert.h"
#endif

#include "gps.h"
#include "radio_interface.h"
#include "lora.h"

/************************** Macro definition ***********************************/
/* Button debounce time in ms */
#define APP_DEBOUNCE_TIME       50
/************************** Global variables ***********************************/
bool button_pressed = false;
bool factory_reset = false;
bool bandSelected = false;
uint32_t longPress = 0;
extern bool certAppEnabled;
#ifdef CONF_PMM_ENABLE
bool deviceResetsForWakeup = false;
#endif
/************************** Extern variables ***********************************/
int16_t getMostRecentCondensedRmcPacket(char* dest, uint8_t max_len);

/************************** Function Prototypes ********************************/
static void driver_init(void);

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif /* #if (_DEBUG_ == 1) */

/****************************** FUNCTIONS **************************************/

static void print_reset_causes(void)
{
    enum system_reset_cause rcause = system_get_reset_cause();
    printf("Last reset cause: ");
    if(rcause & (1 << 6)) {
        printf("System Reset Request\r\n");
    }
    if(rcause & (1 << 5)) {
        printf("Watchdog Reset\r\n");
    }
    if(rcause & (1 << 4)) {
        printf("External Reset\r\n");
    }
    if(rcause & (1 << 2)) {
        printf("Brown Out 33 Detector Reset\r\n");
    }
    if(rcause & (1 << 1)) {
        printf("Brown Out 12 Detector Reset\r\n");
    }
    if(rcause & (1 << 0)) {
        printf("Power-On Reset\r\n");
    }
}

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code)
{
    printf("\r\n%04x\r\n", code);
    (void)level;
}
#endif /* #if (_DEBUG_ == 1) */


/**
 * \mainpage
 * \section preface Preface
 * This is the reference manual for the LORAWAN Demo Application of EU Band
 */
int main(void)
{
    /* System Initialization */
    system_init();
    /* Initialize the delay driver */
    delay_init();
    /* Initialize the board target resources */
    board_init();

    INTERRUPT_GlobalInterruptEnable();
    /* Initialize Hardware and Software Modules */
    driver_init();
    /* Initialize the Serial Interface */
    uart_init();

    print_reset_causes();
#if (_DEBUG_ == 1)
    SYSTEM_AssertSubscribe(assertHandler);
#endif
    /* Initialize demo application (start APP_TASK_ID task) */
    Stack_Init();

	SleepTimerInit();
	
    lora_init();
	
    StartApplicationTask();
	
    while (1)
    {
        usb_serial_data_handler();  // Interrupt-based usart drivers may have left stuff in buffer
		//gps_serial_data_handler();
		SYSTEM_RunTasks();
		//printf("Main looped");
	}
}

/* Initializes all the hardware and software modules used for Stack operation */
static void driver_init(void)
{
    /* Initialize the Radio Hardware */
    HAL_RadioInit();
    /* Initialize the AES Hardware Engine */
    AESInit();
    /* Initialize the Software Timer Module */
    SystemTimerInit();
#ifdef CONF_PMM_ENABLE
    /* Initialize the Sleep Timer Module */
    SleepTimerInit();
#endif
#if (ENABLE_PDS == 1)
    /* PDS Module Init */
    PDS_Init();
#endif
}

/**
 End of File
 */
