#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "info.h"
#include "obd.h"
#include "logger.h"

// works for my vehicle does not work for all

// 010d kph
// 0105 coolant temp
// 0104 engine load
// 0111 throttle %
// 0F01 intake temp
// 0110 mafs

// idk if these should just be in a header file and be inline?

int readVIN(int fd, char *vin, size_t len)
{
	// vin should be 17 chars minus the stupid trailing service bit
	// i should just make something that accounts for this somehow
	if (!vin || len < 18)
	{
		LOG_ERROR("readVIN: vin buffer not correct length\n");
		return -1;
	}

	unsigned char raw[17];
	// pid for vin response
	if (readPID(fd, "0902", raw, 17, 1000000) != 17)
	{
		LOG_ERROR("readVIN: PID 0902 failed\n");
		return -1;
	}

	memcpy(vin, raw, 17);
	vin[17] = '\0';
	
	return 0;
}

int readRPM(int fd, int *rpmOut)
{
	if (!rpmOut)
		return -1;

	unsigned char raw[2];

	// 0x010C engine rpm
	// 2 data bytes
	// convs = us (micro) to ms
	if (readPID(fd, "010C", raw, 2, 50000) != 2)
	{
		LOG_ERROR("readRPM: PID 010C failed\n");
		return -1;
	}
	// ((A*256)+B)/4
	// sae j1979 formula for rpm pasted from docs
	*rpmOut = (int)(((raw[0] << 8) | raw[1]) / 4);
	return 0;
}
