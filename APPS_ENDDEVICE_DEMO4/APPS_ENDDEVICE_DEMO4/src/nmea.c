#include <stdio.h>
#include <stdint.h>
#include <string.h>
                            //(d)ddmm.mmmm
#define TEST_GPRMC_STRING "225446,A,4916.45,N,12311.12,E,"

uint8_t mini_gprmc_msg[] = TEST_GPRMC_STRING;

static int skipField(uint8_t **source, int8_t *sourceSize) {
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

static int copyField(uint8_t **dest, uint8_t **source, int8_t *destCapacity, int8_t *sourceSize) {
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

static int getIsNorth(const char** buffer, int8_t stringlength){
    if (stringlength < 1)
        return 0;

    if (*(*buffer) == 'S') {
        return 0;
    } else {
        return 1;
    }
}

static int getIsEast(const char** buffer, int8_t stringlength){
    if (stringlength < 1)
        return 0;

    if (*(*buffer) == 'W') {
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

    printf("Strinlength: %d\r\n", stringlength);
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

    return milliminutes;
}

int main() {
    int8_t sourcesize = strlen(mini_gprmc_msg);
    uint8_t * buf = mini_gprmc_msg;
    printf("%s\r\n", buf);
    skipField(&buf, &sourcesize); //UTC
    printf("%s\r\n", buf);
    skipField(&buf, &sourcesize); //Status
    printf("%s\r\n", buf);
    uint32_t lat_mag_milliminutes = getLatitudeMagnitude(&buf, sourcesize); // Lat
    skipField(&buf, &sourcesize); //Lat
    if (!getIsNorth(&buf, sourcesize)){
        lat_mag_milliminutes += 0x80000000;  // Make negative
        printf("Not north.\r\n");
    }
    printf("%s -- lat: %lu mm\r\n", buf, lat_mag_milliminutes);
    skipField(&buf, &sourcesize); // N/S
    printf("%s\r\n", buf);
    uint32_t long_mag_milliminutes = getLatitudeMagnitude(&buf, sourcesize); // Long
    skipField(&buf, &sourcesize); //Long
    if (!getIsEast(&buf, sourcesize)) {
        long_mag_milliminutes += 0x80000000;  // make negative
        printf("Not East\r\n");
    }
    printf("%s -- long: %lu mm\r\n", buf, long_mag_milliminutes);
    printf("%s\r\n", buf);
    return 0;
}
