#ifndef GPS_H
#define GPS_H

#include <stdint.h>

/* Serial macros */
#define GPS_SERIAL_BUFFER_LENGTH 20

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

/* Function prototypes */
static GpsTaskState_t gpsTaskState;

void ConfigureGps(void);
void StartGpsTask(void);
void runGpsTask(void);

#endif /* GPS_H */