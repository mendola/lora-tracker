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
#include "lorawan.h"
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
#include "pmm.h"
#include  "conf_pmm.h"
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
/************************** Macro definition ***********************************/
/* Button debounce time in ms */
#define APP_DEBOUNCE_TIME       50
/************************** Global variables ***********************************/
bool button_pressed = false;
bool factory_reset = false;
bool bandSelected = false;
uint32_t longPress = 0;
uint8_t demoTimerId = 0xFF;
uint8_t lTimerId = 0xFF;
extern bool certAppEnabled;
#ifdef CONF_PMM_ENABLE
bool deviceResetsForWakeup = false;
#endif
/************************** Extern variables ***********************************/

/************************** Function Prototypes ********************************/
static void driver_init(void);

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif /* #if (_DEBUG_ == 1) */

/*********************************************************************//**
 \brief      Uninitializes app resources before going to low power mode
*************************************************************************/
#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void);
#endif

char g_payload[50] = {0};


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

#ifdef CONF_PMM_ENABLE
static void appWakeup(uint32_t sleptDuration)
{
    HAL_Radio_resources_init();
    uart_init();
    printf("\r\nsleep_ok %ld ms\r\n", sleptDuration);

}
#endif

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code)
{
    printf("\r\n%04x\r\n", code);
    (void)level;
}
#endif /* #if (_DEBUG_ == 1) */

void filler(void){
	for (int j = 0; j < 1000; j++){
		int k = j*j -1000;
	}
}


void radio_tx_callback(void) {
	static int call_counter = 0;
	//char payload[20] = {0};
	strcpy(g_payload, "Team ALINA's LoRa packet #");
	itoa(++call_counter, g_payload+26,10);
	//strcpy(payload+10,itoa(++call_counter));
	
	RadioTransmitParam_t tx_packet;
	//payload[13] = "\r\n";
	tx_packet.bufferLen = 30;
	tx_packet.bufferPtr = (uint8_t*)g_payload;
	
	RadioError_t status = RADIO_Transmit(&tx_packet);  //TODO move to a task (not inside callback)
	printf("Payload: %s  Ret=%d\r\n", g_payload, status);
	SleepTimerStart(MS_TO_SLEEP_TICKS(5000), (void*)radio_tx_callback);
}

void SetRadioSettings(void) {
	// Configure Radio Parameters
	// --------------------------
	// Bandwidth = BW_125KHZ
	// Channel frequency = FREQ_915200KHZ
	// Channel frequency deviation = 25000
	// CRC = enabled
	// Error Coding Rate = 4/5
	// IQ Inverted = disabled
	// LoRa Sync Word = 0x34
	// Modulation = LoRa
	// PA Boost = disabled (disabled for EU , enabled for NA)
	// Output Power = 1 (up to +14dBm for EU / up to +20dBm for NA)
	// Spreading Factor = SF_12
	// Watchdog timeout = 60000

	// Bandwidth
	RadioLoRaBandWidth_t bw = BW_125KHZ ;
	RADIO_SetAttr(BANDWIDTH, &bw) ;
	printf("Configuring Radio Bandwidth: 125kHz\r\n") ;
	// Channel Frequency
	uint32_t freq = FREQ_915200KHZ ;
	RADIO_SetAttr(CHANNEL_FREQUENCY, &freq) ;
	printf("Configuring Channel Frequency %ld\r\n", freq) ;
	// Channel Frequency Deviation
	uint32_t fdev = 25000 ;
	RADIO_SetAttr(CHANNEL_FREQUENCY_DEVIATION, &fdev) ;
	printf("Configuring Channel Frequency Deviation %ld\r\n", fdev) ;
	// CRC
	uint8_t crc_state = 1 ;
	RADIO_SetAttr(CRC, &crc_state) ;
	printf("Configuring CRC state: %d\r\n", crc_state) ;
	// Error Coding Rate
	RadioErrorCodingRate_t cr = CR_4_5 ;
	RADIO_SetAttr(ERROR_CODING_RATE, &cr) ;
	printf("Configuring Error Coding Rate 4/5\r\n") ;
	// IQ Inverted
	uint8_t iqi = 0 ;
	RADIO_SetAttr(IQINVERTED, &iqi) ;
	printf("Configuring IQ Inverted: %d\r\n", iqi) ;
	// LoRa Sync Word
	uint8_t sync_word = 0x34 ;
	RADIO_SetAttr(LORA_SYNC_WORD, &sync_word) ;
	printf("Configuring LoRa sync word 0x%x\r\n", sync_word) ;
	// Modulation
	RadioModulation_t mod = MODULATION_LORA ;
	RADIO_SetAttr(MODULATION, &mod) ;
	printf("Configuring Modulation: LORA\r\n") ;
	// PA Boost
	uint8_t pa_boost = 0 ;
	RADIO_SetAttr(PABOOST, &pa_boost) ;
	printf("Configuring PA Boost: %d\r\n", pa_boost) ;
	// Tx Output Power
	int16_t outputPwr = 1 ;
	RADIO_SetAttr(OUTPUT_POWER, (void *)&outputPwr) ;
	printf("Configuring Radio Output Power %d\r\n", outputPwr) ;
	// Spreading Factor
	int16_t sf = SF_12 ;
	RADIO_SetAttr(SPREADING_FACTOR, (void *)&sf) ;
	printf("Configuring Radio SF %d\r\n", sf) ;
	// Watchdog Timeout
	uint32_t wdt = 60000 ;
	RADIO_SetAttr(WATCHDOG_TIMEOUT, (void *)&wdt) ;
	printf("Configuring Radio Watch Dog Timeout %ld\r\n", wdt) ;
	//appTaskState = JOIN_SEND_STATE ;
	//appPostTask(DISPLAY_TASK_HANDLER) ;
}

void PrintRadioSettings(void) {
    printf("********* RADIO Settings *********\r\n");
    uint32_t frequency = 0;
    RadioMode_t freq_ret = RADIO_GetAttr(CHANNEL_FREQUENCY, &frequency);
    if (freq_ret != ERR_NONE) {
        printf("Failed to read CHANNEL_FREQUENCY\r\n");
        return freq_ret;
    } else {
        printf("%d\r\n", frequency);
    }

    RadioModulation_t modulation;
    RadioMode_t mod_ret = RADIO_GetAttr(MODULATION, &modulation);
    if (mod_ret != ERR_NONE) {
        printf("Failed to read MODULATION\r\n");
        return mod_ret;
    } else {
        if(modulation == MODULATION_FSK) {
            printf("MODULATION: MODULATION_FSK\r\n");
        } else if (modulation == MODULATION_LORA) {
            printf("MODULATION: MODULATION_LORA\r\n");
        } else {
            printf("Invalid Modulation type.\r\n");
        }
    }

    RadioLoRaBandWidth_t bandwidth;
    RadioMode_t bw_ret = RADIO_GetAttr(BANDWIDTH, &bandwidth);
    if (bw_ret != ERR_NONE) {
        printf("Failed to read BANDWIDTH\r\n");
        return bw_ret;
    } else {
        if(bandwidth == BW_125KHZ) {
            printf("BANDWIDTH: BW_125KHZ\r\n");
        } else if (bandwidth == BW_250KHZ) {
            printf("BANDWIDTH: BW_250KHZ\r\n");
        } else if (bandwidth == BW_500KHZ) {
            printf("BANDWIDTH: BW_500KHZ\r\n");
        } else if (bandwidth == BW_UNDEFINED) {
            printf("BANDWIDTH: BW_UNDEFINED\r\n");
        } else {
            printf("Invalid Modulation type (FSK?). \r\n");
        }
    }
}


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

    SwTimerCreate(&demoTimerId);
	SwTimerCreate(&lTimerId);

	
	SleepTimerInit();
	
    LORAWAN_Init(demo_appdata_callback, demo_joindata_callback);
    printf("\n\n\r*******************************************************\n\r");
    printf("\n\rMicrochip LoRaWAN Stack %s\r\n",STACK_VER);
    printf("\r\nInit - Successful\r\n");
	
	StackRetStatus_t rst_ret = LORAWAN_Reset(ISM_NA915);
	if (rst_ret != LORAWAN_RADIO_SUCCESS && rst_ret != LORAWAN_SUCCESS) {
		printf("Failed to reset LORAWAN: %d\r\n", rst_ret);
	}
	
	// Lets us mess with LoRa settings
	LORAWAN_Pause();
	
	// Set LoRa Settings
	SetRadioSettings();
	PrintRadioSettings();

	//SleepTimerStart(MS_TO_SLEEP_TICKS(1000), (void*)radio_tx_callback);

    //ConfigureGps();
    StartGpsTask();
	StartHeartbeatTask();
	
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

#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void)
{
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
#endif
/**
 End of File
 */
