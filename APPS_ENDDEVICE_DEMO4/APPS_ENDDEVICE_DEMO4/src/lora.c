#include "lora.h"
#include "application_tasks.h"
#include "gps.h"

#define MAX_LORA_PAYLOAD_LENGTH 50
#define COMMAND_PACKET_LENGTH 5
#define LOCATION_PACKET_MIN_LENGTH 1

#define CMD_GO_TO_SLEEP 0x11
#define CMD_LOCALIZE 0x22

uint16_t dest_address = 0xABCD;
uint16_t ping_period = 10;
uint16_t cmd_sleep_time = 10000;

char g_payload[MAX_LORA_PAYLOAD_LENGTH] = {0};
char rx_data[MAX_LORA_PAYLOAD_LENGTH] = {0};
uint8_t rx_dataLength = 0;

uint16_t application_listen_timeout_ = 3;  // in seconds


bool receiver_listening_ = false;
bool actual_message_received_ = false;
bool receiver_timed_out_ = false;

bool transmit_success_ = false;
bool transmitter_sending_ = false;

void set_ping_period(uint16_t ping_period) {
    application_listen_timeout_ = ping_period;
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

void lora_init(void) {
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
}


AppTaskState_t handle_received_packet(void) {
	actual_message_received_ = false;
    printf("Message Received: ");
    for (int i = 0; i < rx_dataLength; i++ ){
        printf("%c", rx_data[i]);
    }
    printf("\r\n");
    return APP_STATE_AWAITING_UART_CMD;
}


void lora_send_location(void) {
	static int call_counter = 0;
	//char payload[20] = {0};
	int16_t payload_length = getMostRecentCondensedRmcPacket(g_payload, MAX_LORA_PAYLOAD_LENGTH);


	//strcpy(payload+10,itoa(++call_counter));
	
	RadioTransmitParam_t tx_packet;
	if (payload_length <= 0) {
		strcpy(g_payload, "No gps fix :(");
		//itoa(++call_counter, g_payload+26,10);
		tx_packet.bufferLen = 13;
		tx_packet.bufferPtr = (uint8_t*)g_payload;
	} else {
		//strcpy(g_payload, "Team ALINA's LoRa packet #");
		//itoa(++call_counter, g_payload+26,10);
		tx_packet.bufferLen = payload_length;
		tx_packet.bufferPtr = (uint8_t*)g_payload;
	}	
	RadioError_t status = RADIO_Transmit(&tx_packet);  //TODO move to a task (not inside callback)
	//printf("Payload: %s  Ret=%d\r\n", g_payload, status);
	//SleepTimerStart(MS_TO_SLEEP_TICKS(5000), (void*)lora_send_location);
}

AppTaskState_t send_lora_localize_cmd(void) {
    AppTaskState_t next_state = APP_STATE_UNKNOWN;
    uint8_t tx_buffer[7];
    
    tx_buffer[0] = dest_address >> 8;
    tx_buffer[1] = dest_address & 0xFF;
    tx_buffer[2] = CMD_LOCALIZE;
    tx_buffer[3] = ping_period >> 8;
    tx_buffer[4] = ping_period & 0xFF;
    tx_buffer[5] = '\r';
    tx_buffer[6] = '\n';

	RadioTransmitParam_t tx_packet;
        
    memcpy((void*)g_payload, tx_buffer, 7);
    //itoa(++call_counter, g_payload+26,10);
    tx_packet.bufferLen = 7;
    tx_packet.bufferPtr = (uint8_t*)g_payload;
	
	if(!transmitter_sending_) {
		if (transmit_success_){
			transmit_success_ = false;
			next_state = APP_STATE_LORA_LISTENING;
		} else {
			next_state = APP_STATE_SEND_LORA_LOCALIZE_CMD;
			RadioError_t status = RADIO_Transmit(&tx_packet);
			//printf("Payload: %s  Ret=%d\r\n", g_payload, status);
			if (status == ERR_NONE) {
				transmitter_sending_ = true;
				next_state = APP_STATE_SEND_LORA_LOCALIZE_CMD;
			}
		}
	}
	
    return next_state;
}

AppTaskState_t send_lora_sleep_cmd(void) {
    AppTaskState_t next_state = APP_STATE_UNKNOWN;
    uint8_t tx_buffer[7];
    
    tx_buffer[0] = dest_address >> 8;
    tx_buffer[1] = dest_address & 0xFF;
    tx_buffer[2] = CMD_GO_TO_SLEEP;
    tx_buffer[3] = cmd_sleep_time >> 8;
    tx_buffer[4] = cmd_sleep_time & 0xFF;
    tx_buffer[5] = '\r';
    tx_buffer[6] = '\n';

	RadioTransmitParam_t tx_packet;
        
    memcpy((void*)g_payload, tx_buffer, 7);
    //itoa(++call_counter, g_payload+26,10);
    tx_packet.bufferLen = 7;
    tx_packet.bufferPtr = (uint8_t*)g_payload;

	if(!transmitter_sending_) {
		if (transmit_success_){
			transmit_success_ = false;
			next_state = APP_STATE_LORA_LISTENING;
		} else {
			next_state = APP_STATE_SEND_LORA_SLEEP_CMD;
			RadioError_t status = RADIO_Transmit(&tx_packet);
			//printf("Payload: %s  Ret=%d\r\n", g_payload, status);
			if (status == ERR_NONE) {
				transmitter_sending_ = true;
				next_state = APP_STATE_SEND_LORA_SLEEP_CMD;
			}			
		}
	}

    return next_state;
}

AppTaskState_t lora_listen(void) {
    AppTaskState_t next_state = APP_STATE_UNKNOWN;
	if (!receiver_listening_) {
		if (actual_message_received_) {
			printf("Message received.\r\n");
			next_state = handle_received_packet();
		} else {
			if (receiver_timed_out_) {
				receiver_timed_out_ = false;
				printf("Receiver timed out.");
				next_state = APP_STATE_AWAITING_UART_CMD;
			} else {
				printf("Putting radio in Receive mode...\r\n");
				RadioReceiveParam_t radioReceiveParam;
				radioReceiveParam.action = RECEIVE_START;
				radioReceiveParam.rxWindowSize = 30 * application_listen_timeout_; // Convert to s
				receiver_listening_ = true;
				rx_dataLength = 0;
				if (RADIO_Receive(&radioReceiveParam) != 0) {
					receiver_listening_ = false;
					printf("Radio failed to enter Receive mode\r\n") ;
				}		
			}
		}
	}
    return next_state;
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
	//appPostTask(HEARTBEAT_TASK_HANDLER) ;
}



/*********************************************************************//*
 \brief      Function that processes the Rx data
 \param[in]  data - Rx data payload
 \param[in]  dataLen - The number of Rx bytes
 ************************************************************************/
static void demo_handle_evt_rx_data(void *appHandle, appCbParams_t *appdata)
{
    uint8_t *pData = appdata->param.rxData.pData;
    uint8_t dataLength = appdata->param.rxData.dataLength;
    uint32_t devAddress = appdata->param.rxData.devAddr;

    //Successful transmission
    if((dataLength > 0U) && (NULL != pData))
    {
        printf("*** Received DL Data ***\n\r");
        printf("\nFrame Received at port %d\n\r",pData[0]);
        printf("\nFrame Length - %d\n\r",dataLength);
        printf("\nAddress - 0x%lx\n\r", devAddress);
        printf ("\nPayload: ");
        for (uint8_t i =0; i<dataLength - 1; i++)
        {
            printf("%x",pData[i+1]);
        }
        printf("\r\n*************************\r\n");
    }
    else
    {
        printf("Received ACK for Confirmed data\r\n");
    }
}

/*********************************************************************//**
\brief Callback function for the ending of Bidirectional communication of
       Application data
 *************************************************************************/
void demo_appdata_callback(void *appHandle, appCbParams_t *appdata)
{
    StackRetStatus_t status = LORAWAN_INVALID_REQUEST;

    if (LORAWAN_EVT_RX_DATA_AVAILABLE == appdata->evt)
    {
        status = appdata->param.rxData.status;
        switch(status)
        {
            case LORAWAN_SUCCESS:
            {
                demo_handle_evt_rx_data(appHandle, appdata);
            }
            break;
            case LORAWAN_RADIO_NO_DATA:
            {
                printf("\n\rRADIO_NO_DATA \n\r");
				receiver_timed_out_ = true;
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
			case LORAWAN_RADIO_SUCCESS:
			{
				uint8_t dataLength = appdata->param.rxData.dataLength ;	// length of the packet received
				uint8_t *pData = appdata->param.rxData.pData ;
				if((dataLength > 0U) && (NULL != pData)) {
                    if (dataLength >= MAX_LORA_PAYLOAD_LENGTH) {
                        dataLength = MAX_LORA_PAYLOAD_LENGTH;
                    }
					//printf ("\r\nPayload of length %d received: %s", dataLength, (char*)pData) ;
                    memcpy((void*)rx_data, pData, dataLength);
					if (receiver_listening_) {		
						actual_message_received_ = true;
					}
					if (transmitter_sending_) {
						transmit_success_ = true;
					}
                    rx_dataLength = dataLength;
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
                printf("LORAWAN_SUCCESS\r\n");
				transmit_success_ = true;
            }
            break;
            case LORAWAN_RADIO_SUCCESS:
            {
                printf("LORAWAN_RADIO_SUCCESS\r\n");
				transmit_success_ = true;
            }
            break;
            case LORAWAN_RADIO_NO_DATA:
            {
                printf("\n\rRADIO_NO_DATA \n\r");
				receiver_timed_out_ = true;
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
    
    receiver_listening_ = false;
	transmitter_sending_ = false;
}

