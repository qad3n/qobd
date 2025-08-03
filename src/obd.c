#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#include "obd.h"
#include "usb.h"
#include "colors.h"
#include "logger.h"

// i do not entirely know what i am doing
int readPID(int serialFd, const char *cmd, unsigned char *outBuf, size_t outLen, unsigned int timeoutUs)
{
	// reasonable buffer size for ascii reply
	char reply[512];

	// send command and store reply
	if (sendElmCmd(serialFd, cmd, reply, sizeof(reply), timeoutUs, false) <= 0)
		return -1;

	// every hex token parsed from reply will land here
	unsigned int hexVals[512];
	size_t numVals = 0;

	// split reply on spaces or newlines then convert each token to a number
	// hex for easier brain parsing
	char *token = strtok(reply, " \r\n");
	while (token != NULL && numVals < sizeof hexVals / sizeof hexVals[0])
	{
		// elm prefix
		if (strchr(token, ':'))
		{
			token = strtok(NULL, " \r\n");
			continue;
		}

		// tmp
		unsigned int value;

		// only accept if sscanf returns 1 field
		// should be something like
		// 7E
		// 0x7E
		if (sscanf(token, "%x", &value) == 1)
			hexVals[numVals++] = value;

		// advance to next token
		token = strtok(NULL, " \r\n");
	}

	// instruction set says at least two header bytes and one payload byte
	// this is just function safety
	if (numVals < 3)
		return -1;

	// get mode and pid from the command string
	unsigned int mode = 0, pid = 0;
	if (sscanf(cmd, "%2x%2x", &mode, &pid) != 2)
		return -1;

	// obd2 spec a pos response echoes the service with bit 6 set so add 0x40
	// tested
	// 0x01 = 0x41
	// 0x09 = 0x49

	// expected response header bytes
	const unsigned int hdrMode = (mode + 0x40U) & 0xFFU; // 0x40 0100 0000b
	const unsigned int hdrPid = pid; // second byte is just whatever the pid is

	// wtf
	// service 0x09 replies carry an extra record index byte
	// // // does the ecu log these records and can i reference them by the index? idk
	const bool mode09 = (mode == 0x09);

	// scan the token list until either pattern hdrMode hdrPid
	// locate the first header match
	size_t idx = 0;
	while (idx + 1 < numVals && !(hexVals[idx] == hdrMode && hexVals[idx + 1] == hdrPid))
		++idx;
	if (idx + 1 >= numVals) // no matching frame
		return -1;

	// advance past header bytes
	// for mode 09 service also skip header
	idx += 2;
	if (mode09 && idx < numVals)
		++idx;

	// shovel payload bytes into caller buffer, collapsing any repeated headers
	// copy data bytes into caller buffer
	size_t copied = 0;
	while (idx < numVals && copied < outLen)
	{
		// some adapters echo the header at the start of every 7-byte can fragment
		// this caches and skip repeat header buffer
		// faster elm response times when requesting big / multiple buffers

		// the elm states this and suggests that this is performed on its responses
		// account for trailing service byte
		if (hexVals[idx] == hdrMode && idx + 1 < numVals && hexVals[idx + 1] == hdrPid)
		{
			idx += 2;
			if (mode09 && idx < numVals)
				++idx;

			continue;
		}

		// copy byte to output
		outBuf[copied++] = (unsigned char)hexVals[idx++];
	}

	// caller only wants success if exactly outLen bytes
	// otherwise data is wrong
	// remove trailing byte
	return (copied == outLen) ? (int)copied : -1;
}

// i have an idea on how to set this up better
// and also maybe detect year make and model
// https://www.alldatasheet.com/datasheet-pdf/pdf/197403/ELM/ELM327.html
int initOBD(int fd)
{
	// usually 256 is fine
	char respBuf[256];
	ssize_t len; 

	// flush old serial data
	LOG_DEBUG("flushed");
	tcflush(fd, TCIOFLUSH);

	LOG_DEBUG("attempting to communicate with obd2 reciever");

	// reset elm
	LOG_DEBUG("ELM RESET COMMAND");
	len = sendElmCmd(fd, "ATZ", respBuf, sizeof(respBuf), 500000, true);
	if (len <= 0)
	{
		LOG_ERROR("elm reset (ATZ) failed: no response");
		return -1;
	}

	// disable echo
	LOG_DEBUG("ELM DISABLE ECHO COMMAND");
	len = sendElmCmd(fd, "ATE0", respBuf, sizeof(respBuf), 100000, true);
	if (len <= 0)
	{
		LOG_ERROR("disabling echo (ATE0) failed: no response");
		return -1;
	}

	// i didnt see a difference with and without these calls
	// so i only disable echo for now
	//ATS0 // disables spaces
	//ATL0 // disables lines

	// auto detect obd devices
	// you can manually specify the protocol if this fails for some reason
	// pyobd has it attempt from a list if auto fails i should do that too
	LOG_DEBUG("OBD AUTO PROTOCOL DETECT COMMAND");
	len = sendElmCmd(fd, "AT SP 0", respBuf, sizeof(respBuf), 100000, true);
	if (len <= 0)
	{
		LOG_ERROR("setting protocol to automatic (AT SP 0) failed: no response");
		return -1;
	}
	if (strstr(respBuf, "UNABLE TO CONNECT")) // sometimes it just doesnt idk
	{
		LOG_WARNING("obd2 device unreachable on first protocol attempt... retrying");

		// retry with longer buffer wait time the short respone works fine on my car
		// but fails for the newer models and needs the longer buffer time
		len = sendElmCmd(fd, "AT SP 0", respBuf, sizeof(respBuf), 5000000, true);

		if (len <= 0)
		{
			LOG_ERROR("second protocol attempt (AT SP 0) failed: no response");
			return -1;
		}
	}

	// force lower protocol response timeout for faster buffer response times
	LOG_DEBUG("ELM RESPONSE RATE COMMAND");
	len = sendElmCmd(fd, "AT ST 0A", respBuf, sizeof respBuf, 100000, true);
	if (len <= 0) 
	{
		LOG_ERROR("failed to force protocol response timeout");
		return -1;
	}

	// mode 01 PID 00
	// probe attempt on service mode pid
	// proper way to connect to ecu? i think?
	// pids wont print unless this responds valid, assuming so

	// okay so if u first get in ur car and only plug it in there is no response
	// if u do accessory and get it to load then open ur door and take the key out
	// and return the program the ecu will still respond
	// i got this to happening without having to open my door too
	// if u wait long enough it wont respond anymore until u put the key back in again
	// annoying when debugging
	LOG_DEBUG("ELM ECU SERVICE PROBE COMMAND");
	len = sendElmCmd(fd, "0100", respBuf, sizeof(respBuf), 200000, true);
	if (len <= 0) 
	{
		// no bytes at all, i couldnt get this to happen, catching just in case
		LOG_ERROR("ecu did not respond at all?");
		return -1;
	}

	if (!strstr(respBuf, "41 00"))
	{
		if (strstr(respBuf, "UNABLE TO CONNECT"))
		{
			LOG_WARNING("ecu did not respond to service probe attempt\nvehicle must at least be in accessory mode");
			return -1;
		}

		LOG_WARNING("ecu responded. unexpected service pid response.");
		return -1;
	}
	else
		LOG_SUCCESS("acu active / ecu supports pids 01 through 20");

	// what I should really do is just have a silent check for the battery
	// and find a way to properly detect the ignition state
	// probably easier with push start cars, my key cylinder is ancient
	// all of this code below kinda just testing different ways i could do that

	// finds vehicle battery and check voltage
	// elm usually gets 0.1v from usb port so anything more probably means the key
	// is at least in ignition state 0
	LOG_DEBUG("ELM BATTERY COMMAND");
	len = sendElmCmd(fd, "AT RV", respBuf, sizeof(respBuf), 100000, true);
	if (len <= 0)
	{
		LOG_ERROR("reading vehicle voltage (AT RV) failed: no response");
		return -1;
	}

	float voltage = strtof(respBuf, NULL);
	if (voltage <= 1.0f)
	{
		LOG_ERROR("vehicle battery not detected or health is too low: %.2fv detected\n", voltage);
		return -1;
	}

	return 0;
}
