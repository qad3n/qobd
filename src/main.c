#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "usb.h"
#include "obd.h"
#include "menu.h"
#include "logger.h"

int main(void)
{
	// open serial to elm327 usb
	// file descriptor
	int fd = initUSB("/dev/ttyUSB0");
	if (fd < 0)
	{
		LOG_ERROR("failed to find elm obd2 device");
		return 1;
	}

	//initialize elm & obd2 protocols
	if (initOBD(fd))
	{
		LOG_ERROR("adapter/vehicle not ready");
		close(fd);
		return 1;
	}

	// start the program
	drawMenu(fd);

	// close and exit properly when program is closed
	close(fd);
	return 0;
}
