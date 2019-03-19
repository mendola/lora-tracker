#include "gps.h"

extern int gps_uart_getchar_nowait(void);

typedef enum _GpsTaskState_t {
	GPS_TASK_STATE_READY,
	GPS_TASK_STATE_NMEA_RX_IN_PROGRESS,
	GPS_TASK_STATE_UBX_RX_IN_PROGRESS
} GpsTaskState_t;

void runGpsTask(void) {
    int byte;
    char rx_char;
    while (byte = gps_uart_getchar_nowait() >= 0) {
        rx_char = (char)byte;
        
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
}

static void HandleCompleteNmeaPacket(void) {
    printf("Complete NMEA packet: ");
    for(int i = 0; i<nmea_buffer_char_count_; i++){
        printf("%c ", nmea_buffer_[i]);
    }
    printf("\r\n");
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
