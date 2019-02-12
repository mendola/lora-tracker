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
#include "enddevice_demo.h"
#include "sio2host.h"
#include "extint.h"
#include "conf_app.h"
#include "sw_timer.h"
#include "led.h"
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

static uint8_t on = LON;
static uint8_t off = LOFF;
static uint8_t toggle = LTOGGLE;

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif /* #if (_DEBUG_ == 1) */

/*********************************************************************//**
 \brief      Uninitializes app resources before going to low power mode
*************************************************************************/
#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void);
#endif

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
    sio2host_init();
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
	char payload[20] = {0};
	strcpy(payload, "Transmit #");
	itoa(++call_counter, payload+10,10);
	//strcpy(payload+10,itoa(++call_counter));
	
	RadioTransmitParam_t tx_packet;
	//payload[13] = "\r\n";
	tx_packet.bufferLen = 20;
	tx_packet.bufferPtr = (uint8_t*)payload;
	
	RadioError_t status = RADIO_Transmit(&tx_packet);
	printf("Payload: %s  Ret=%d\r\n", payload, status);
	SleepTimerStart(MS_TO_SLEEP_TICKS(5000), (void*)radio_tx_callback);
}


/*********************************************************************//**
\brief Callback function for the ending of Bidirectional communication of
       Application data
 *************************************************************************/
void demo_appdata_callback(void *appHandle, appCbParams_t *appdata){
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST;

	if (LORAWAN_EVT_RX_DATA_AVAILABLE == appdata->evt)
	{
		status = appdata->param.rxData.status;
		switch(status)
		{
			case LORAWAN_SUCCESS:
			{
				//demo_handle_evt_rx_data(appHandle, appdata);
				printf("\n\rLORAWAN_SUCCESS (normally calls demo_handle_evt_rx_data(appHandle, appdata);\n\r");
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("\n\rNO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			//m16946 - payload received in p2p
			case LORAWAN_RADIO_SUCCESS:
			{
				uint8_t dataLength = appdata->param.rxData.dataLength ;	// length of the packet received
				uint8_t *pData = appdata->param.rxData.pData ;
				if((dataLength > 0U) && (NULL != pData))
				{
					printf ("\nPayload Received. Length: %d\r\n", dataLength) ;
					for (int idx=0; idx<dataLength; idx++){
						printf("%d ", pData[idx]);
					}

					// Enter Radio Receive mode
					RadioReceiveParam_t radioReceiveParam ;
					uint32_t rxTimeout = 0 ;	// forever
					radioReceiveParam.action = RECEIVE_START ;
					radioReceiveParam.rxWindowSize = rxTimeout;
					if (RADIO_Receive(&radioReceiveParam) != 0)
					{
						printf("Radio failed to enter Receive mode\r\n") ;
					}
					//print_array(pData, dataLength) ;
					//printf("\r\n*************************\r\n");
				}
			}
			break ;
			default:
			printf("UNKNOWN ERROR\n\r");
			break;
		}
	}
	else if(LORAWAN_EVT_TRANSACTION_COMPLETE == appdata->evt)
	{
		switch(status = appdata->param.transCmpl.status)
		{
			case LORAWAN_SUCCESS:
			{
				printf("Transmission Success\r\n");
			}
			break;
			case LORAWAN_RADIO_SUCCESS:
			{
				printf("Transmission Success\r\n");
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_TX_TIMEOUT:
			printf("\nTx Timeout\n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("\n\rNO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			default:
			printf("\n\rUNKNOWN ERROR\n\r");
			break;

		}
		printf("\n\r*************************************************\n\r");
	}
}


/*********************************************************************//*
\brief Callback function for the ending of Activation procedure
 ************************************************************************/
void demo_joindata_callback(bool status)
{
    /* This is called every time the join process is finished */
    set_LED_data(LED_GREEN,&off);
    if(true == status)
    {
        uint32_t devAddress;
        bool mcastEnabled;

        printf("\nJoining Successful\n\r");
        LORAWAN_GetAttr(DEV_ADDR, NULL, &devAddress);
        LORAWAN_GetAttr(MCAST_ENABLE, NULL, &mcastEnabled);

        if (devAddress != DEMO_APP_MCAST_GROUP_ADDRESS)
        {
            printf("\nDevAddr: 0x%lx\n\r", devAddress);
        }
        else if ((devAddress == DEMO_APP_MCAST_GROUP_ADDRESS) && (true == mcastEnabled))
        {
            printf("\nAddress conflict between Device Address and Multicast group address\n\r");
        }
        print_application_config();
        set_LED_data(LED_GREEN,&on);
    }
    else
    {
        set_LED_data(LED_AMBER,&on);
        printf("\nJoining Denied\n\r");
    }
    printf("\n\r*******************************************************\n\r");
    //PDS_StoreAll();
	
	//appTaskState = JOIN_SEND_STATE;
    //appPostTask(DISPLAY_TASK_HANDLER);
}

void SetRadioSettings(void) {
	// Configure Radio Parameters
	// --------------------------
	// Bandwidth = BW_125KHZ
	// Channel frequency = FREQ_868100KHZ
	// Channel frequency deviation = 25000
	// CRC = enabled
	// Error Coding Rate = 4/5
	// IQ Inverted = disabled
	// LoRa Sync Word = 0x34
	// Modulation = LoRa
	// PA Boost = disabled (disabled for EU , enabled for NA)
	// Output Power = 1 (up to +14dBm for EU / up to +20dBm for NA)
	// Spreading Factor = SF7
	// Watchdog timeout = 60000

	// Bandwidth
	RadioLoRaBandWidth_t bw = BW_125KHZ ;
	RADIO_SetAttr(BANDWIDTH, &bw) ;
	printf("Configuring Radio Bandwidth: 125kHz\r\n") ;
	// Channel Frequency
	uint32_t freq = FREQ_868100KHZ ;
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
	int16_t sf = SF_7 ;
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
    RadioError_t ret = RADIO_GetAttr(CHANNEL_FREQUENCY, &frequency);
    if (ret != ERR_NONE) {
        printf("Failed to read CHANNEL_FREQUENCY\r\n");
        return ret;
    } else {
        printf("Frequency: %d Hz\r\n", frequency);
    }

    RadioModulation_t modulation;
    RadioError_t mod_ret = RADIO_GetAttr(MODULATION, &modulation);
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
    RadioError_t bw_ret = RADIO_GetAttr(BANDWIDTH, &bandwidth);
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
    sio2host_init();

    print_reset_causes();
#if (_DEBUG_ == 1)
    SYSTEM_AssertSubscribe(assertHandler);
#endif
    /* Initialize demo application */
    Stack_Init();

    SwTimerCreate(&demoTimerId);
    //SwTimerCreate(&lTimerId);
		
	SleepTimerInit();
    //mote_demo_init();

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

	// Enter Radio Receive mode
	RadioReceiveParam_t radioReceiveParam ;
	uint32_t rxTimeout = 0 ;	// forever
	radioReceiveParam.action = RECEIVE_START ;
	radioReceiveParam.rxWindowSize = rxTimeout;
	if (RADIO_Receive(&radioReceiveParam) == 0)
	{
		printf("Radio in Receive mode\r\n") ;
	}
	
	//SleepTimerStart(MS_TO_SLEEP_TICKS(1000), (void*)radio_tx_callback);
    while (1)
    {
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
    sio2host_deinit();
    /* Disable Transceiver SPI Module */
    HAL_RadioDeInit();
}
#endif
/**
 End of File
 */
