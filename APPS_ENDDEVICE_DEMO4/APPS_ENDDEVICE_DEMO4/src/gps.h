#ifndef GPS_H
#define GPS_H

#include <stdint.h>

/* NMEA macros */
#define NMEA_START_CHAR '$'
#define NMEA_END_CHAR = '\n'
#define NMEA_BUFFER_LENGTH 100

/* UBX macros */
#define UBX_SYNC_1  0xB5
#define UBX_SYNC_2  0x62

uint8_t nmea_buffer_[NMEA_BUFFER_LENGTH];
int8_t nmea_buffer_char_count_ = 0;

static GpsTaskState_t gpsTaskState;

void runGpsTask(void);

#endif /* GPS_H */