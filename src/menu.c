#include "menu.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// idk
extern int readRPM(int fd, int *rpm);
extern int readVIN(int fd, char *vin, size_t len);

// not really set up to add other menus properly but will do
// eventually when i am doing dtc reading and clearing

// i did not finish moving everything to terminal.h

// live data loop
static int showLiveData(int fd)
{
	int rpm = 0, lastRpm = -1;

	system("clear");

	hideCursor();
	moveCursor(1, 1);

	printf("LIVE DATA:\n");
	while (1)
	{
		if (readRPM(fd, &rpm) == 0 && rpm != lastRpm)
		{
			moveCursor(2, 1);
			printf("Engine RPM: %6d\n", rpm);
			lastRpm = rpm;
		}

		moveCursor(3, 1);

		printf("\nPress 'b' to go back, or 'q' to quit...");
		flushOut();

		// mfw no toLower
		char ch = getChar();
		if (ch == 'b' || ch == 'B')
		{
			showCursor();
			return 1;
		}
		if (ch == 'q' || ch == 'Q')
		{
			showCursor();
			return 0;
		}

		usleep(40); // needs some amount of delay or cpu load will be 100%
	}
}

// menu entry point, call from main.c
void drawMenu(int fd)
{
	char vin[18];
	if (readVIN(fd, vin, sizeof(vin)) < 0)
		snprintf(vin, sizeof(vin), "empty");

	// set random terminal seed
	srand((unsigned int)time(NULL));
	setRawMode();
	hideCursor();
	
	while (1)
	{
		showCursor();

		printf("\nVIN: %s\n", vin);
		printf("[1] Live data\n");
		//printf("[2] Show pending DTCs\n");
		//printf("[3] Show permanent DTCs\n");
		//printf("[4] Clear DTCs\n");
		printf("\nChoose an option or 'q' to quit: ");

		flushOut();

		// blocking get char
		int ch = getchar();
		if (ch == 'q' || ch == 'Q' || ch == '5')
			break;

		int menuStatus = 1;

		if (ch == '1')
			menuStatus = showLiveData(fd);
		//else if (ch == '2')
			//menuStatus = whatever(fd);
		if (!menuStatus)
			break;
	}

	showCursor();
	restoreTerm();
	system("clear");
	moveCursor(1, 1);
	flushOut();
}
