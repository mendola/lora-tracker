#include "gps.h"
#include "compiler.h"
#include "string.h"

extern int gps_uart_getchar_nowait(void);
extern void appPostGpsTask(void);
extern void gps_uart_copy_data(uint8_t* destination, int8_t maxlen);
extern bool gpsUartHasData(void);
extern status_code_genare_t gps_uart_blocking_read(uint16_t *const rx_data);

void gps_uart_request_rx(void);

static void gpsRxCharStateReady(const char rx_char);
static void gpsRxCharStateNmeaRxInProgress(const char rx_char);
static void gpsRxCharStateUbxRxInProgress(const char rx_char);


/* Module Variables */
volatile uint8_t most_recent_gprmc_message_condensed_[CONDENSED_GPRMC_MSG_BUFFER_LENGTH];
volatile int8_t most_recent_gprmc_message_length_ = 0;
volatile bool has_valid_gprmc_message = false;

uint8_t serial_buffer_[GPS_SERIAL_BUFFER_LENGTH];
int8_t serial_buffer_char_count_ = 0;

volatile uint8_t nmea_buffer_[NMEA_BUFFER_LENGTH];
volatile int8_t nmea_buffer_char_count_ = 0;

uint8_t ubx_buffer_[UBX_BUFFER_LENGTH];
int8_t ubx_buffer_char_count_ = 0;

bool gps_has_fix(void) {
	return true; //has_valid_gprmc_message;
}

int16_t getMostRecentCondensedRmcPacket(char* dest, uint8_t max_len) {
	uint16_t length = 0;
	if (most_recent_gprmc_message_length_ > 0){	
		if (most_recent_gprmc_message_length_ >= max_len) {
			length = max_len;
		} else {
			length = most_recent_gprmc_message_length_;
		}
		memcpy((void*)dest, most_recent_gprmc_message_condensed_, length);
		memset((void*)most_recent_gprmc_message_condensed_, 0, sizeof(most_recent_gprmc_message_condensed_));
		most_recent_gprmc_message_length_ = 0;
	}
	return length;
}

void printRxBuffer(void) {
	for(int i = 0; i < GPS_SERIAL_BUFFER_LENGTH; i++) {
		printf("%c", serial_buffer_[i]);
	}
	printf("\r\n");
}

/* Send appropriate messages to GPS to configure settings */
void ConfigureGps(void) {
    
}

void StartGpsTask(void) {
    gpsTaskState = 	GPS_TASK_STATE_READY;
    gps_uart_request_rx();
    appPostGpsTask();
}

void printBuffer(uint8_t* buffer, int8_t length) {
	for (int i=0; i<length; i++) {
		printf("%c", buffer[i]);
	}
	printf("\r\n");
}

void runGpsTask(void) {
    uint16_t rx_data;
	char rx_char;
    if(gpsUartHasData()) {
		//printf("Gps Has Data: ");
		//printRxBuffer();
        gps_uart_copy_data(serial_buffer_, GPS_SERIAL_BUFFER_LENGTH);
		gps_uart_request_rx();

        for (int i = 0; i<GPS_SERIAL_BUFFER_LENGTH; i++) {
            rx_char = (char)serial_buffer_[i];
		//	status_code_genare_t status = gps_uart_blocking_read(rx_data);
		//	printf("gps_uart_blocking_read() returned: %d\r\n", status);
		//	if(status == 0) {
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
		//}
        //serial_buffer_char_count_ = 0;
    }


    appPostGpsTask();
}


static uint8_t nmeaRollingXorChecksum(uint8_t* buffer, int8_t length) {
    uint8_t result = 0;
    for (int i = 0; i < length; i++){
        result ^= buffer[i];
    }
    return result;
}

static bool skipField(uint8_t **source, int8_t *sourceSize) {
	if (source == NULL || *source == NULL || sourceSize == NULL)
		return -1;
		
	while (*sourceSize >= 1) {
		uint8_t currChar = (uint8_t)(*source)[0];
	//	printf("Curr char: %c\r\n",currChar);
		if (currChar == (uint8_t)',' || currChar == (uint8_t)'*') {
			--(*sourceSize);
			++(*source);
	//		printf(" -- Return 0. source[0] is now %c\r\n",**source);
			return 0;
		} else {
			//printf("%c (%d) is not %d\r\n",(char)currChar,currChar,(uint8_t)",");
			--(*sourceSize);
			++(*source);	
		}
	}
	return -1;
}

static bool copyField(uint8_t **dest, uint8_t **source, int8_t *destCapacity, int8_t *sourceSize) {
	if (dest == NULL || *dest == NULL || source == NULL || *source == NULL || destCapacity == NULL || sourceSize == NULL)
		return -1;

	int i = 0;
	while (i < *sourceSize && i < *destCapacity) {
		if ((*source)[i] == ',' || (*source)[i] == '*') {
			memcpy((void*)*dest, *source, i + 1);
			*sourceSize = *sourceSize - i - 1;
			*destCapacity = *destCapacity - i - 1;
			(*dest) = (*dest) + i + 1;
			(*source) = (*source) + i + 1;
			return 0;
		} else {
			++i;
		}
	}
	return -1;
}

static uint8_t char2digit(const char character) {
	if (character < '0') {
		return 0;
		} else if (character > '9') {
		return 0;
		} else {
		return character - '0';
	}
}

static int getIsNorth(const char* buffer, int8_t stringlength){
	if (stringlength < 1)
	return 0;

	if (*buffer == 'S') {
		return 0;
		} else {
		return 1;
	}
}

static int getIsDataOk(char* buffer, int8_t stringlength){
	if (stringlength < 1)
		return 0;

	if (*buffer == 'A') {
		return 1;
	} else {
		return 0;
	}
}

static int getIsEast(const char* buffer, int8_t stringlength){
	if (stringlength < 1)
	return 0;

	if (*buffer == 'W') {
		return 0;
	} else {
		return 1;
	}
}

static uint32_t getLatitudeMagnitude(const char** buffer, int8_t stringlength) {
	char* remaining_string = *buffer;
	char nmea_latitude_buf[15] = {0};
	uint8_t degrees = 0;
	uint32_t milliminutes = 0; // minutes * 1/1000
	

	uint8_t nmea_latitude_buf_len = 0;
	while (nmea_latitude_buf_len < stringlength && remaining_string[nmea_latitude_buf_len] != ',') {
		//printf("%d | %c\r\n", remaining_string[nmea_latitude_buf_len], ',');
		nmea_latitude_buf[nmea_latitude_buf_len] = (char)remaining_string[nmea_latitude_buf_len];
		++nmea_latitude_buf_len;
	}

	//printf("Strinlength: %d\r\n", stringlength);
	int8_t period_index = 0;
	while (period_index < nmea_latitude_buf_len && remaining_string[period_index] != '.') {
		++period_index;
	}

	if (period_index < 4 || nmea_latitude_buf_len < period_index + 2) {
		printf("Period index: %d\r\n", period_index);
		return 0xFFFFFFFF;
	}
	
	int16_t multiplier = 10000;
	milliminutes = milliminutes + multiplier * char2digit(nmea_latitude_buf[period_index - 2]);
	multiplier /= 10;
	milliminutes = milliminutes + 1000 * char2digit(nmea_latitude_buf[period_index - 1]);
	multiplier /= 10;
	for (int i = 0; i < nmea_latitude_buf_len - period_index - 1 && multiplier >= 1; i++) {
		milliminutes += multiplier * char2digit(nmea_latitude_buf[period_index + 1 + i]);
		multiplier /= 10;
	}

	multiplier = 1;
	period_index -= 3; // Get to the degreess
	while (period_index >= 0) {
		degrees += multiplier * char2digit(nmea_latitude_buf[period_index]);
		period_index--;
		multiplier *= 10;
	}
	milliminutes += degrees * 60 * 1000; // Convert degrees to milliminutes
	printf("Milliminutes: %d\r\n",milliminutes);
	return milliminutes;
}

/* Buffer starts after $ */
static void HandleNmeaPacketGPRMC(const uint8_t* buffer, int8_t length) {
	uint8_t *dest = most_recent_gprmc_message_condensed_;
	int8_t dest_capacity_left = CONDENSED_GPRMC_MSG_BUFFER_LENGTH;
	uint8_t * source = buffer;
	int8_t sourceSize = length;
	
	has_valid_gprmc_message = false;
	most_recent_gprmc_message_length_ = 0;
	memset((void*)most_recent_gprmc_message_condensed_, 0, sizeof(most_recent_gprmc_message_condensed_));
	
	/* Copy the fields we care about */
	// Field 0: GPRMC
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 1: time UTC
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 2: status
	if (!getIsDataOk(source, sourceSize)){
		has_valid_gprmc_message = false;
		return -1;
	}
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 3: latitude
	uint32_t lat_mag_milliminutes = getLatitudeMagnitude(&source, sourceSize); // Lat
	if (lat_mag_milliminutes == 0xFFFFFFFF) {
		printf("Error\r\n");
		has_valid_gprmc_message = false;
		return -1;
	}
	if (!getIsNorth(source, sourceSize)){
		lat_mag_milliminutes += 0x80000000;  // Make negative
	}
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 4: North/South Indicator
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 5: Longitude
	uint32_t longitude_mag_milliminutes = getLatitudeMagnitude(&source, sourceSize); // Lat
	if (longitude_mag_milliminutes == 0xFFFFFFFF) {
		printf("Error\r\n");
		has_valid_gprmc_message = false;
		return -1;
	}
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 6: East/West indicator
	if (!getIsEast(source, sourceSize)){
		longitude_mag_milliminutes += 0x80000000;  // Make negative
	}
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 7: Speed over ground
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 8: Course over Ground
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 9: Date
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 10: Magnetic Variation
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}
	// Field 11: Magnetic Variation East/West indicator
	if(skipField(&source, &sourceSize)) {
		has_valid_gprmc_message = false;
		return -1;
	}

	most_recent_gprmc_message_condensed_[0] = '$';
	memcpy(most_recent_gprmc_message_condensed_ + 1, &lat_mag_milliminutes, sizeof(lat_mag_milliminutes));
	memcpy(most_recent_gprmc_message_condensed_ + 5, &longitude_mag_milliminutes, sizeof(longitude_mag_milliminutes));
	most_recent_gprmc_message_condensed_[9] = '\r';
	most_recent_gprmc_message_condensed_[10] = '\n';
	most_recent_gprmc_message_length_ = CONDENSED_GPRMC_MSG_BUFFER_LENGTH;
	has_valid_gprmc_message = true;
	printf(">");
	for (int i = 0; i < CONDENSED_GPRMC_MSG_BUFFER_LENGTH; i++){
		printf("%c", (uint8_t)most_recent_gprmc_message_condensed_[i]);
	}
	//printf("%s\r\n", most_recent_gprmc_message_condensed_);
	return 0;
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
	//    printf("Complete NMEA packet: ");
	//    for(int i = 0; i<nmea_buffer_char_count_; i++){
	//        printf("%c", nmea_buffer_[i]);
	//    }
	//	printf("\r\n");
	if(!verify_nmea_checksum_passes(nmea_buffer_ + 1, nmea_buffer_char_count_ - 1)){
		//printf("\t -- Checksum Failed.\r\n");
		return;
		} else {
		//printf("\t-- Checksum Passed.\r\n");
	}

	if (nmea_buffer_char_count_ >= 6 && (strncmp((char*)nmea_buffer_ + 1, "GPRMC", 5) == 0 || strncmp((char*)nmea_buffer_ + 1, "GNRMC", 5) == 0) ) {
		HandleNmeaPacketGPRMC(nmea_buffer_ + 1, nmea_buffer_char_count_ - 1);

		} else if (nmea_buffer_char_count_ >= 5 && strncmp((char*)nmea_buffer_ + 1, "PUBX", 4 == 0)) {
		//printf("\tMessage Type: PUBX \r\n");
		} else {
		//printf("\tMessage Type: Invalid \r\n");
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
		// printf("Received non-start gps char in state ready: %c\r\n", rx_char);
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
		//printf("Received invalid NMEA character: %c\r\n", rx_char);
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
