#ifndef GPS_H
#define GPS_H

#include <stdint.h>

/* NMEA macros */
#define NMEA_START_CHAR		'$'
#define NMEA_END_CHAR		'\n'
#define NMEA_BUFFER_LENGTH	100

/* UBX macros */
#define UBX_SYNC_1			0xB5
#define UBX_SYNC_2			0x62
#define UBX_BUFFER_LENGTH	100

/* Typedefs */
typedef enum _GpsTaskState_t {
	GPS_TASK_STATE_READY,
	GPS_TASK_STATE_NMEA_RX_IN_PROGRESS,
	GPS_TASK_STATE_UBX_RX_IN_PROGRESS,
	GPS_TASK_STATE_ERROR
} GpsTaskState_t;

typedef enum _NmeaMessageType_t {
	NMEA_MESSAGE_ADDRESS_GPRMC,
} NmeaMessageType_t;

/* Module Variables */
uint8_t nmea_buffer_[NMEA_BUFFER_LENGTH];
int8_t nmea_buffer_char_count_ = 0;

uint8_t ubx_buffer_[UBX_BUFFER_LENGTH];
int8_t ubx_buffer_char_count_ = 0;

/* Function prototypes */
static GpsTaskState_t gpsTaskState;

void ConfigureGps(void);
void StartGpsTask(void);
void runGpsTask(void);

#endif /* GPS_H */