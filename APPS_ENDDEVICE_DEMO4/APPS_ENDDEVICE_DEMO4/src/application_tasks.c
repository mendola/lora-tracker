/**
* \file  application_tasks.h
*
* \brief LORAWAN Demo Application
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

/**
* \mainpage
* \section preface Preface
* The EndDevice_Demo_application available in Atmel Studio, 
* is used to send the temperature sensor data through the 
* LoRaWAN network to the network server.
* <P>� This example provides option to user to configure regional band in run time.</P>
* <P>� Using this example application, LoRaWAN Functionalities such as Joining, Sending data
* and changing end device class is demonstrated.</P>
* <P>� This example showcases sleep functionality of LoRaWAN Stack and the Hardware.</P>
* <P>� This example demonstrates storing stack parameters in NVM using PDS. </P>
*/

/****************************** INCLUDES **************************************/
#include "asf.h"
#include "lorawan.h"
#include "system_task_manager.h"
#include "application_tasks.h"
#include "conf_app.h"
#include "sio2host.h"
#include "resources.h"
#include "delay.h"
#include "sw_timer.h"
#include "LED.h"
#include "pmm.h"
#include "radio_driver_hal.h"
#include "conf_pmm.h"
#include "conf_sio2host.h"
#include "pds_interface.h"

#if (CERT_APP == 1)
#include "conf_certification.h"
#include "enddevice_cert.h"
#endif
#if (EDBG_EUI_READ == 1)
#include "edbg_eui.h"
#endif
#include "atomic.h"
#include <stdint.h>
#include "lora.h"
#include "gps.h"
/******************************** MACROS ***************************************/
#define USB_SERIAL_BUFFER_LENGTH  10

/************************** GLOBAL VARIABLES ***********************************/
static bool joined = false;
static float cel_val;
static float fahren_val;
static char temp_sen_str[25];
static uint8_t data_len = 0;
bool certAppEnabled = false;

static uint8_t on = LON;
static uint8_t off = LOFF;
static uint8_t toggle = LTOGGLE;

static volatile uint8_t appTaskFlags = 0x00u;
/* Default Regional band start delay time */
volatile static uint8_t count = 5;

static uint8_t rxchar[11];
static bool startReceiving = false;

static AppTaskState_t appTaskState;

static const char* bandStrings[] =
{
    "FactoryDefaultReset",
#if (EU_BAND == 1)
    "EU868",
#endif
#if (NA_BAND == 1)
    "NA915",
#endif
#if (AU_BAND == 1)
    "AU915",
#endif
#if (AS_BAND == 1)
    "AS923",
#endif
#if (JPN_BAND == 1)
    "JPN923",
#endif
#if (KR_BAND == 1)
    "KR920",
#endif
#if (IND_BAND == 1)
    "IND865",
#endif
    "Clear PDS",
    "Reset Board"
};


uint8_t bandTable[] =
{
    0xFF,
#if (EU_BAND == 1)
    ISM_EU868,
#endif
#if (NA_BAND == 1)
    ISM_NA915,
#endif
#if (AU_BAND == 1)
    ISM_AU915,
#endif
#if (AS_BAND == 1)
    ISM_THAI923,
#endif
#if (JPN_BAND == 1)
    ISM_JPN923,
#endif
#if (KR_BAND == 1)
    ISM_KR920,
#endif
#if (IND_BAND == 1)
    ISM_IND865,
#endif
    0xFF,
    0xFF
};

/*ABP Join Parameters */
static uint32_t demoDevAddr = DEMO_DEVICE_ADDRESS;
static uint8_t demoNwksKey[16] = DEMO_NETWORK_SESSION_KEY;
static uint8_t demoAppsKey[16] = DEMO_APPLICATION_SESSION_KEY;
/* OTAA join parameters */
static uint8_t demoDevEui[8] = DEMO_DEVICE_EUI;
static uint8_t demoAppEui[8] = DEMO_APPLICATION_EUI;
static uint8_t demoAppKey[16] = DEMO_APPLICATION_KEY;

static LorawanSendReq_t lorawanSendReq;

static char serialBuffer[USB_SERIAL_BUFFER_LENGTH];
static uint8_t serialBufferCount = 0;

/* Muticast Parameters */
static bool demoMcastEnable = DEMO_APP_MCAST_ENABLE;
static uint32_t demoMcastDevAddr = DEMO_APP_MCAST_GROUP_ADDRESS;
static uint8_t demoMcastNwksKey[16] = DEMO_APP_MCAST_NWK_SESSION_KEY;
static uint8_t demoMcastAppsKey[16] = DEMO_APP_MCAST_APP_SESSION_KEY;
/************************** EXTERN VARIABLES ***********************************/
extern bool button_pressed;
extern bool factory_reset;
extern bool bandSelected;
extern uint32_t longPress;
extern void set_ping_period(uint16_t period_s);
extern void set_cmd_sleep_time(uint16_t period_s);

void appPostTask(AppTaskIds_t id);
static SYSTEM_TaskStatus_t (*appTaskHandlers[])(void);
static void demoTimerCb(void * cnt);
static void lTimerCb(void * data);
static SYSTEM_TaskStatus_t radioTask(void);
static SYSTEM_TaskStatus_t gpsTask(void);
static SYSTEM_TaskStatus_t heartbeatTask(void);
static SYSTEM_TaskStatus_t keyboardTask(void);

static void processRunDemoCertApp(void);
static void processRunRestoreBand(void);
static void processJoinAndSend(void);
static void displayRunDemoCertApp(void);
static void displayRunRestoreBand(void);
static void displayJoinAndSend(void);
static void displayRunDemoApp(void);


void appWakeup(uint32_t sleptDuration);
void app_resources_uninit(void);

static void dev_eui_read(void);
/************************** FUNCTION PROTOTYPES ********************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void);
static float convert_celsius_to_fahrenheit(float cel_val);
extern void runGpsTask(void);

void set_app_state(AppTaskState_t state) {
	appTaskState = state;
}

/*********************************************************************//*
 \brief      Function that processes the Rx data
 \param[in]  data - Rx data payload
 \param[in]  dataLen - The number of Rx bytes
 ************************************************************************/
static void demo_handle_evt_rx_data(void *appHandle, appCbParams_t *appdata);

/***************************** FUNCTIONS ***************************************/

static SYSTEM_TaskStatus_t (*appTaskHandlers[APP_TASKS_COUNT])(void) = {
    /* In the order of descending priority */
	keyboardTask,
    heartbeatTask,
    radioTask,
};

/*********************************************************************//**
\brief    Calls appropriate functions based on state variables
*************************************************************************/
static SYSTEM_TaskStatus_t gpsTask(void) {
    runGpsTask();
	return SYSTEM_TASK_SUCCESS;
}

/* TODO This is trash and needs refactoring (circular deps)*/
void StartHeartbeatTask(void) {
	appPostTask(HEARTBEAT_TASK_HANDLER);
}

void StartApplicationTask(void) {
    appTaskState = APP_STATE_LORA_LISTENING;
    appPostTask(RADIO_TASK_HANDLER);
    appPostTask(KEYBOARD_TASK_HANDLER);
    StartHeartbeatTask();
}

static void print_options(void) {
	printf("\r\n******** READY FOR COMMAND ********\r\n");
	printf("> l <duration> or L <duration> -- Put the node into localization mode with a beacon every <duration> seconds.\r\n");
	printf("> s <duration> or S <duration> -- Put the node into sleep mode for <duration> seconds, before listening for commands for 10 seconds.\r\n");
	printf("\r\n***********************************\r\n");
}

static AppTaskState_t check_for_uart_input(void) {
	AppTaskState_t next_state = APP_STATE_UNKNOWN;
	const uint8_t first_byte = (uint8_t)serialBuffer[0];
	int i = 1;
	uint16_t period = 0;
	uint8_t curr_char;
	
	switch (first_byte) {
		case 'l':
		case 'L': ;
		next_state = APP_STATE_LORA_LISTENING;

		while (i < serialBufferCount &&  serialBuffer[i] != '\r' && serialBuffer[i] != '\n' && serialBuffer[i] != '.') {
			curr_char = serialBuffer[i];
			if ((curr_char < 48 || curr_char > 57) && (curr_char != ' ')) {
				period = 0;
				printf("Invalid Localization Sleep Time\r\n");
				next_state = APP_STATE_UNKNOWN;
				break;
				} else if (curr_char != ' ') {
				period = period * 10 + (curr_char - 48);
				next_state = APP_STATE_SEND_LORA_LOCALIZE_CMD;
			}
			i++;
		}
		if (period > 0 && period < 10000) {
			set_ping_period(period);
			} else {
			printf("Invalid sleep period\r\n");
			next_state = APP_STATE_UNKNOWN;
		}
		break;
		case 's':
		case 'S': ;
		next_state = APP_STATE_UNKNOWN;

		while (i < serialBufferCount &&  serialBuffer[i] != '\r' && serialBuffer[i] != '\n' && serialBuffer[i] != '.') {
			uint8_t curr_char = serialBuffer[i];
			if ((curr_char < 48 || curr_char > 57) && (curr_char != ' ')) {
				period = 0;
				printf("Invalid Power-Save Sleep Time\r\n");
				next_state = APP_STATE_SEND_LORA_SLEEP_CMD;
				break;
				} else if (curr_char != ' ') {
				period = period * 10 + (curr_char - 48);
				next_state = APP_STATE_SEND_LORA_SLEEP_CMD;
			}
			i++;
		}
		if (period > 0 && period < 10000) {
			set_cmd_sleep_time(period);
			} else {
			printf("Invalid sleep period\r\n");
			next_state = APP_STATE_UNKNOWN;
		}
		break;
		default:
		next_state = APP_STATE_UNKNOWN;
		break;
	}
	memset((void*)serialBuffer, 0, USB_SERIAL_BUFFER_LENGTH);
	serialBufferCount = 0;
	
	return next_state;
}

static SYSTEM_TaskStatus_t keyboardTask(void) {
    if (!startReceiving) {
	    print_options();
	    startReceiving = true;
    }
    bool received_full_packet = usb_serial_data_handler();
    if (received_full_packet) {
		AppTaskState_t next_state = check_for_uart_input();
	    if (next_state != APP_STATE_UNKNOWN) {
		    appTaskState = next_state;
	    }
	}
   	return SYSTEM_TASK_SUCCESS;
}

static SYSTEM_TaskStatus_t heartbeatTask(void) {
	static uint8_t ledstate = 0;
	static uint64_t prevtime_us = 0;
	
	uint64_t now_us = SwTimerGetTime();
	
	if (now_us - prevtime_us >= 1000000) {
		if (ledstate == 0 ) {
			ledstate = 1;
		} else {
			ledstate = 0;
		}
		
		prevtime_us = now_us;
		set_LED_data(LED_GREEN,&ledstate);
		//printf("Heartbeat: %d", ledstate);
	}
	return SYSTEM_TASK_SUCCESS;
}


/*********************************************************************//**
\brief    Pulls the data from UART when activated
*************************************************************************/
bool usb_serial_data_handler(void)
{
	int charRet;
	bool received_full_packet = false;
	/* verify if there was any character received*/
	while (startReceiving == true && (-1) != (charRet = usb_uart_getchar_nowait())) {
		char rxChar = (char) charRet;
		printf("%c", rxChar);
		if((rxChar == '\r') || (rxChar == '\n') || (rxChar == '\b')) {
			startReceiving = false;
			received_full_packet = true;
			//appPostTask(RADIO_TASK_HANDLER);
		} else {
			if (serialBufferCount < USB_SERIAL_BUFFER_LENGTH) {
  				serialBuffer[serialBufferCount++] = rxChar;
			} 
			if (serialBufferCount >= USB_SERIAL_BUFFER_LENGTH) {
				startReceiving = false;
				received_full_packet = true;
			}
			
		}
	}
	return received_full_packet;
}

void gps_serial_data_handler(void) {
	
}

static AppTaskState_t go_to_sleep(void) {

}


/*********************************************************************//**
\brief    Calls appropriate functions based on state variables
*************************************************************************/
static SYSTEM_TaskStatus_t radioTask(void)
{
    AppTaskState_t next_state = APP_STATE_UNKNOWN;
	switch(appTaskState)
	{
        case APP_STATE_SEND_LORA_LOCALIZE_CMD:
            next_state = send_lora_localize_cmd();
            if (next_state == APP_STATE_UNKNOWN) {
                next_state = APP_STATE_SEND_LORA_LOCALIZE_CMD; 
            }
            break;
        case APP_STATE_SEND_LORA_SLEEP_CMD:
            next_state = send_lora_sleep_cmd();
            if (next_state == APP_STATE_UNKNOWN) {
                next_state = APP_STATE_SEND_LORA_SLEEP_CMD; 
            }
            break;
        case APP_STATE_LORA_LISTENING:
            next_state = lora_listen();
            if (next_state == APP_STATE_UNKNOWN) {
                next_state = APP_STATE_LORA_LISTENING; 
            }
            break;
		case APP_STATE_WAIT_FOR_LOCALIZE_ACK:
			next_state = lora_listen_for_localize_ack();
            if (next_state == APP_STATE_UNKNOWN) {
	            next_state = APP_STATE_WAIT_FOR_LOCALIZE_ACK;
            }
            break;
		case APP_STATE_WAIT_FOR_SLEEP_ACK:
			next_state = lora_listen_for_sleep_ack();
			if (next_state == APP_STATE_UNKNOWN) {
				next_state = APP_STATE_WAIT_FOR_SLEEP_ACK;
			}
		break;
		default:
			printf("Error STATE Entered\r\n");
			break;
	}
    appTaskState = next_state;
    appPostTask(RADIO_TASK_HANDLER);
	appPostTask(HEARTBEAT_TASK_HANDLER);
	appPostTask(KEYBOARD_TASK_HANDLER);
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief    Displays and activates LED's to choose between Demo
		  and Certification application
*************************************************************************/
static void displayRunDemoCertApp(void)
{
	//sio2host_rx(rxchar,10);
	set_LED_data(LED_AMBER,&off);
	set_LED_data(LED_GREEN,&off);
	printf("1. Demo application\r\n");
	#if (CERT_APP == 1)
	printf("2. Certification application\r\n");
	#endif
	printf("\r\n Select Application : ");
	startReceiving = true;
}

/*********************************************************************//**
\brief    Activates LED's to indicate restoring of band
*************************************************************************/
static void displayRunRestoreBand(void)
{
	//sio2host_rx(rxchar,10);
	set_LED_data(LED_AMBER,&off);
	set_LED_data(LED_GREEN,&off);
	appPostTask(RADIO_TASK_HANDLER);
}

/*********************************************************************//**
\brief    Displays and activates LED's for joining to a network
		  and sending data to a network
*************************************************************************/
static void displayJoinAndSend(void)
{
    printf("\r\n1. Send Join Request\r\n");
    printf("2. Send Data\r\n");
#ifdef CONF_PMM_ENABLE
    printf("3. Sleep\r\n");
    printf("4. Main Menu\r\n");
#else
    printf("3. Main Menu\r\n");
#endif /* CONF_PMM_ENABLE */
    printf("\r\nEnter your choice: ");
    set_LED_data(LED_AMBER,&off);
    set_LED_data(LED_GREEN,&off);	
	startReceiving = true;
}

/*********************************************************************//**
\brief    Displays and activates LED's for selecting Demo application
*************************************************************************/
static void displayRunDemoApp(void)
{
	uint8_t i = 0;
	
    set_LED_data(LED_AMBER,&off);
    set_LED_data(LED_GREEN,&off);

    printf("\r\nPlease select one of the band given below\r\n");
    for(i = 1;i < sizeof(bandTable); i++)
    {
	    printf("%d. %s\r\n",i,bandStrings[i]);
    }

    printf("Select Regional Band : ");
	startReceiving = true;
}

/*********************************************************************//**
\brief    Initialization the Demo application
*************************************************************************/

/*********************************************************************//*
\brief Callback function for the ending of Activation procedure
 ************************************************************************/
void demo_joindata_callback(bool status) {
    (void) status;
}

#ifdef CONF_PMM_ENABLE
void appWakeup(uint32_t sleptDuration)
{
    HAL_Radio_resources_init();
    uart_init();
	appTaskState = APP_STATE_LORA_LISTENING;
    appPostTask(HEARTBEAT_TASK_HANDLER);
    appPostTask(RADIO_TASK_HANDLER);
	appPostTask(KEYBOARD_TASK_HANDLER);
    printf("\r\nsleep_ok %ld ms\r\n", sleptDuration);
}
#endif


void app_resources_uninit(void)
{
    /* Disable USART TX and RX Pins */
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.powersave  = true;
#ifdef HOST_SERCOM_PAD0_PIN
    port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
#endif
#ifdef HOST_SERCOM_PAD1_PIN
    port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
#endif
    /* Disable UART module */
    uart_usb_deinit();
	uart_gps_deinit();
    /* Disable Transceiver SPI Module */
    HAL_RadioDeInit();
	//HAL_DisbleDIO1Interrupt();
}


#if (CERT_APP == 1)
/*********************************************************************//*
 \brief      Function to runs certification application.
 ************************************************************************/
static void  runCertApp(void)
{
    certAppEnabled = true;
    cert_app_init();
}
#endif


/*********************************************************************//*
 \brief      App Post Task
 \param[in]  Id of the application to be posted
 ************************************************************************/

void appPostTask(AppTaskIds_t id)
{
    ATOMIC_SECTION_ENTER
    appTaskFlags |= (1 << id);
    ATOMIC_SECTION_EXIT

    /* Also post a APP task to the system */
    SYSTEM_PostTask(APP_TASK_ID);
}


/*********************************************************************//*
 \brief      Application Task Handler
 ************************************************************************/

SYSTEM_TaskStatus_t APP_TaskHandler(void)
{

    if (appTaskFlags)
    {
        for (uint16_t taskId = 0; taskId < APP_TASKS_COUNT; taskId++)
        {
            if ((1 << taskId) & (appTaskFlags))
            {
                ATOMIC_SECTION_ENTER
                appTaskFlags &= ~(1 << taskId);
                ATOMIC_SECTION_EXIT

                appTaskHandlers[taskId]();

                /* Break here so that higher priority task executes next, if any */
                //break;  ^ No
            }
        }
    }

    if (appTaskFlags)
    {
        SYSTEM_PostTask(APP_TASK_ID);
    }
    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//*
 \brief      Set join parameters function
 \param[in]  activation type - notifies the activation type (OTAA/ABP)
 \return     LORAWAN_SUCCESS, if successfully set the join parameters
             LORAWAN_INVALID_PARAMETER, otherwise
 ************************************************************************/
StackRetStatus_t set_join_parameters(ActivationType_t activation_type)
{
    StackRetStatus_t status;

    printf("\n********************Join Parameters********************\n\r");

    if(ACTIVATION_BY_PERSONALIZATION == activation_type)
    {
        status = LORAWAN_SetAttr (DEV_ADDR, &demoDevAddr);
        if (LORAWAN_SUCCESS == status)
        {
            status = LORAWAN_SetAttr (APPS_KEY, demoAppsKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppSessionKey : ");
            print_array((uint8_t *)&demoAppsKey, sizeof(demoAppsKey));
            status = LORAWAN_SetAttr (NWKS_KEY, demoNwksKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nNwkSessionKey : ");
            print_array((uint8_t *)&demoNwksKey, sizeof(demoNwksKey));
        }

    }
    else
    {
        status = LORAWAN_SetAttr (DEV_EUI, demoDevEui);
        if (LORAWAN_SUCCESS == status)
        {
            printf("\nDevEUI : ");
            print_array((uint8_t *)&demoDevEui, sizeof(demoDevEui));
            status = LORAWAN_SetAttr (APP_EUI, demoAppEui);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppEUI : ");
            print_array((uint8_t *)&demoAppEui, sizeof(demoAppEui));
            status = LORAWAN_SetAttr (APP_KEY, demoAppKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppKey : ");
            print_array((uint8_t *)&demoAppKey, sizeof(demoAppKey));
        }
    }
    return status;
}

/*********************************************************************//*
 \brief      Function to Initialize the device type
 \param[in]  ed_class - notifies the device class (CLASS_A/CLASS_B/CLASS_C)
 \return     LORAWAN_SUCCESS, if successfully set the device class
             LORAWAN_INVALID_PARAMETER, otherwise
 ************************************************************************/
StackRetStatus_t set_device_type(EdClass_t ed_class)
{
    StackRetStatus_t status = LORAWAN_SUCCESS;

    status = LORAWAN_SetAttr(EDCLASS, &ed_class);

    if((LORAWAN_SUCCESS == status) && ((CLASS_C | CLASS_B) & ed_class) && (true == DEMO_APP_MCAST_ENABLE))
    {
        set_multicast_params();
    }

    return status;
}

/*********************************************************************//*
 \brief      Function to Initialize the Multicast parameters
 ************************************************************************/
void set_multicast_params (void)
{
    StackRetStatus_t status;

    printf("\n***************Multicast Parameters********************\n\r");

    status = LORAWAN_SetAttr(MCAST_APPS_KEY, &demoMcastAppsKey);
    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMcastAppSessionKey : ");
        print_array((uint8_t *)&demoMcastAppsKey, sizeof(demoMcastAppsKey));
        status = LORAWAN_SetAttr(MCAST_NWKS_KEY, &demoMcastNwksKey);
    }

    if(status == LORAWAN_SUCCESS)
    {
        printf("\nMcastNwkSessionKey : ");
        print_array((uint8_t *)&demoMcastNwksKey, sizeof(demoMcastNwksKey));
        status = LORAWAN_SetAttr(MCAST_GROUP_ADDR, &demoMcastDevAddr);
    }

    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMcastGroupAddr : 0x%lx\n\r", demoMcastDevAddr);
        status = LORAWAN_SetAttr(MCAST_ENABLE, &demoMcastEnable);
    }
    else
    {
        printf("\nMcastGroupAddrStatus : Failed\n\r");
    }

    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMulticastStatus : Enabled\n\r");
    }
    else
    {
        printf("\nMulticastStatus : Failed\n\r");
    }
}


/*********************************************************************//*
 \brief      Function to Print array of characters
 \param[in]  *array  - Pointer of the array to be printed
 \param[in]   length - Length of the array
 ************************************************************************/
void print_array (uint8_t *array, uint8_t length)
{
    printf("0x");
    for (uint8_t i =0; i < length; i++)
    {
        printf("%02x", *array);
        array++;
    }
    printf("\n\r");
}

/*********************************************************************//*
 \brief      Function to Print application configuration
 ************************************************************************/
void  print_application_config (void)
{
    EdClass_t edClass;
    printf("\n***************Application Configuration***************\n\r");
    LORAWAN_GetAttr(EDCLASS, NULL, &edClass);
    printf("\nDevType : ");

    if(edClass == CLASS_A)
    {
        printf("CLASS A\n\r");
    }
    else if(edClass == CLASS_C)
    {
        printf("CLASS C\n\r");
    }

    printf("\nActivationType : ");

    if(DEMO_APP_ACTIVATION_TYPE == OVER_THE_AIR_ACTIVATION)
    {
        printf("OTAA\n\r");
    }
    else if(DEMO_APP_ACTIVATION_TYPE == ACTIVATION_BY_PERSONALIZATION)
    {
        printf("ABP\n\r");
    }

    printf("\nTransmission Type - ");

    if(DEMO_APP_TRANSMISSION_TYPE == CONFIRMED)
    {
        printf("CONFIRMED\n\r");
    }
    else if(DEMO_APP_TRANSMISSION_TYPE == UNCONFIRMED)
    {
        printf("UNCONFIRMED\n\r");
    }

    printf("\nFPort - %d\n\r", DEMO_APP_FPORT);

    printf("\n*******************************************************\n\r");
}

/*********************************************************************//*
 \brief      Function to Print stack return status
 \param[in]  status - Status from the stack
 ************************************************************************/
void print_stack_status(StackRetStatus_t status)
{
    switch(status)
    {
        case LORAWAN_SUCCESS:
             printf("\nlorawan_success\n\r");
        break;

        case LORAWAN_BUSY:
             printf("\nlorawan_state : stack_Busy\n\r");
        break;

        case LORAWAN_NWK_NOT_JOINED:
            printf("\ndevice_not_joined_to_network\n\r");
        break;

        case LORAWAN_INVALID_PARAMETER:
            printf("\ninvalid_parameter\n\r");
        break;

        case LORAWAN_KEYS_NOT_INITIALIZED:
            printf("\nkeys_not_initialized\n\r");
        break;

        case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
            printf("\nsilent_immediately_active\n\r");
        break;

        case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
            printf("\nframecounter_error_rejoin_needed\n\r");
        break;

        case LORAWAN_INVALID_BUFFER_LENGTH:
            printf("\ninvalid_buffer_length\n\r");
        break;

        case LORAWAN_MAC_PAUSED:
            printf("\nMAC_paused\n\r");
        break;

        case LORAWAN_NO_CHANNELS_FOUND:
            printf("\nno_free_channels_found\n\r");
        break;

        case LORAWAN_INVALID_REQUEST:
            printf("\nrequest_invalid\n\r");
        break;
        case LORAWAN_NWK_JOIN_IN_PROGRESS:
            printf("\nprev_join_request_in_progress\n\r");
        break;
        default:
           printf("\nrequest_failed %d\n\r",status);
        break;
    }
}

/*********************************************************************//*
 \brief      Function to convert Celsius value to Fahrenheit
 \param[in]  cel_val   - Temperature value in Celsius
 \param[out] fauren_val- Temperature value in Fahrenheit
 ************************************************************************/
static float convert_celsius_to_fahrenheit(float celsius_val)
{
    float fauren_val;
    /* T(�F) = T(�C) � 9/5 + 32 */
    fauren_val = (((celsius_val * 9)/5) + 32);

    return fauren_val;

}

/*********************************************************************//*
 \brief      Reads the DEV EUI if it is flashed in EDBG MCU
 ************************************************************************/
static void dev_eui_read(void)
{
#if (EDBG_EUI_READ == 1)
	uint8_t invalidEDBGDevEui[8];
	uint8_t EDBGDevEUI[8];
	edbg_eui_read_eui64((uint8_t *)&EDBGDevEUI);
	memset(&invalidEDBGDevEui, 0xFF, sizeof(invalidEDBGDevEui));
	/* If EDBG doesnot have DEV EUI, the read value will be of all 0xFF, 
	   Set devEUI in conf_app.h in that case */
	if(0 != memcmp(&EDBGDevEUI, &invalidEDBGDevEui, sizeof(demoDevEui)))
	{
		/* Set EUI addr in EDBG if there */
		memcpy(demoDevEui, EDBGDevEUI, sizeof(demoDevEui));
	}
#endif
}


/* eof demo_app.c */
