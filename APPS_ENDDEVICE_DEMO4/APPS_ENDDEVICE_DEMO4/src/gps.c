#include "gps.h"
#include "compiler.h"
#include "string.h"

extern int gps_uart_getchar_nowait(void);
extern void appPostGpsTask(void);
extern void gps_uart_copy_data(uint8_t* destination, int8_t maxlen);
extern bool gpsUartHasData(void);

static void gpsRxCharStateReady(const char rx_char);
static void gpsRxCharStateNmeaRxInProgress(const char rx_char);
static void gpsRxCharStateUbxRxInProgress(const char rx_char);


/* Module Variables */
uint8_t serial_buffer_[GPS_SERIAL_BUFFER_LENGTH];
int8_t serial_buffer_char_count_ = 0;

uint8_t nmea_buffer_[NMEA_BUFFER_LENGTH];
int8_t nmea_buffer_char_count_ = 0;

uint8_t ubx_buffer_[UBX_BUFFER_LENGTH];
int8_t ubx_buffer_char_count_ = 0;

/* Send appropriate messages to GPS to configure settings */
void ConfigureGps(void) {
    
}

void StartGpsTask(void) {
    gpsTaskState = 	GPS_TASK_STATE_READY;
    gps_uart_request_rx();
    appPostGpsTask();
}

void runGpsTask(void) {
    char rx_char;

    if(gpsUartHasData()){
        gps_uart_copy_data(serial_buffer_, GPS_SERIAL_BUFFER_LENGTH);

        for (int i = 0; i<serial_buffer_char_count_; i++) {
            rx_char = (char)serial_buffer_[i];
            
            switch(gpsTaskState) {
                case GPS_TASK_STATE_READY:
                    gpsRxCharStateReady(rx_char);
                    break;
                case GPS_TASK_STATE_NMEA_RX_IN_PROGRESS:
                    gpsRxCharStateNmeaRxInProgress(rx_char);
                    break;
                case GPS_TASK_STATE_UBX_RX_IN_PROGRESS:
                    gpsRxCharStateUbxRxInProgress(rx_char);
                    break;
                default:
                    printf("Error: invalid gpsTaskState\r\n");
                    break;
            }
        }

        serial_buffer_char_count_ = 0;
        gps_uart_request_rx();
    }


    appPostGpsTask();
}

static readNmeaMessageType(char* buffer, int8_t length) {
    if (length >= 6 && strncmp((char*)buffer + 1, "GPRMC", 5) == 0) {
        printf("Received GPRMC messge\r\n");

    } else if (length >= 5 && strncmp((char*)buffer + 1, "PUBX", 4 == 0)) {
        printf("Received PUBX message\r\n");

    }
}

static uint8_t nmeaRollingXorChecksum(uint8_t* buffer, int8_t length) {
    uint8_t result = 0;
    for (int i = 0; i < length; i++){
        result ^= buffer[i];
    }
    return result;
}

/* Buffer starts after $ */
static void HandleNmeaPacketGPRMC(uint8_t* buffer, int8_t length) {
    
}

static int16_t ascii_pair_to_hex(uint8_t char1, uint8_t char0) {
    int16_t result = 0;
    if ('A' <= char1 <= 'F') {
        result += (int16_t)(char1 - 'A') * 10;
    } else if ('0' <= char1 <= '9') {
        result += (int16_t)(char1 - '0') * 10;
    } else {
        return -1;
    }

    if ('A' <= char1 <= 'F') {
        result += (int16_t)(char0 - 'A') * 10;
    } else if ('0' <= char0 <= '9') {
        result += (int16_t)(char0 - '0') * 10;
    } else {
        return -1;
    }
}

static bool verify_nmea_checksum_passes(uint8_t* buffer, int8_t length) {
    int i = 0;
    uint8_t checksum_value;
    while(i < length){  // Length minus the 2 charagers after "*" representing hex checksum value
        if (buffer[i] == "*") {
            if (i < length - 2) {
                int8_t hexchar1 = buffer[i+1]; // ASCII to digit
                int8_t hexchar0 = buffer[i+2];
                checksum_value = ascii_pair_to_hex(hexchar1, hexchar0);
                if (checksum_value >= 0) {
                    return checksum_value == nmeaRollingXorChecksum(buffer, i);
                } else {
                    return false;
                }
            }
        }
        ++i;
    }
}


static void HandleCompleteNmeaPacket(void) {
    printf("Complete NMEA packet: ");
    for(int i = 0; i<nmea_buffer_char_count_; i++){
        printf("%c ", nmea_buffer_[i]);
    }
    printf("\r\n");


    if(!verify_nmea_checksum_passes(nmea_buffer_ + 1, nmea_buffer_char_count_ - 1)){
        printf("NMEA Checksum Failed.\r\n");
        return;
    }

    if (nmea_buffer_char_count_ >= 6 && strncmp((char*)nmea_buffer_ + 1, "GPRMC", 5) == 0) {
        printf("Received GPRMC messge\r\n");
        HandleNmeaPacketGPRMC(nmea_buffer_ + 1, nmea_buffer_char_count_ - 1);

    } else if (nmea_buffer_char_count_ >= 5 && strncmp((char*)nmea_buffer_ + 1, "PUBX", 4 == 0)) {
        printf("Received PUBX message\r\n");
    } else {
        printf("Invalid NMEA message type");
    }

    nmea_buffer_char_count_ = 0;
    gpsTaskState = GPS_TASK_STATE_READY;
}

static void gpsRxCharStateReady(const char rx_char) {
    switch (rx_char) {
        case NMEA_START_CHAR:
            nmea_buffer_char_count_ = 1;
            nmea_buffer_[0] = NMEA_START_CHAR;
            gpsTaskState = GPS_TASK_STATE_NMEA_RX_IN_PROGRESS;
            break;
        case UBX_SYNC_1:
            ubx_buffer_char_count_ = 1;
            ubx_buffer_[0] = UBX_SYNC_1;
            gpsTaskState = GPS_TASK_STATE_UBX_RX_IN_PROGRESS;
            break;
        default:
            printf("Received non-start gps char in state ready: %c\r\n", rx_char);
            break;
    }
}

static uint8_t ValidNmeaCharacter(uint8_t c) {
  return ('a' <= c && c <= 'z') ||
          ('A' <= c && c <= 'Z') ||
          ('0' <= c && c <= '9') ||
          c == '$' ||
          c == ',' ||
          c == '.' ||
          c == '-' ||
          c == '/' ||
          c == '*' ||
          c == '\r' ||
          c == NMEA_END_CHAR;
}

static void gpsRxCharStateNmeaRxInProgress(const char rx_char) {
    /* Make sure char is valid */
    if (!ValidNmeaCharacter(rx_char)) {
        printf("Received invalid NMEA character: %c\r\n", rx_char);
        gpsTaskState = GPS_TASK_STATE_READY;
        nmea_buffer_char_count_ = 0;
        return;
    }

    /* Make sure buffer isn't full (if so something went wrong) */
    if (nmea_buffer_char_count_ >= NMEA_BUFFER_LENGTH) {
        gpsTaskState = GPS_TASK_STATE_READY;
        nmea_buffer_char_count_ = 0;
        return;     
    }

    nmea_buffer_[nmea_buffer_char_count_] = rx_char;
    ++nmea_buffer_char_count_;
    
    if (rx_char == NMEA_END_CHAR) {
        HandleCompleteNmeaPacket();
    }
}

/* Implement if needed */
static void gpsRxCharStateUbxRxInProgress(const char rx_char) {
    printf("In state GPS_TASK_STATE_UBX_RX_IN_PROGRESS, received: %x\r\n", rx_char);
    return;
}
