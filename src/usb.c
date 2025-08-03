#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <string.h>

#include "math.h"
#include "logger.h"
#include "usb.h"

int initUSB(const char *path)
{
	// no tty controller open
	int fd = open(path, O_RDWR | O_NOCTTY);
	if (fd < 0) 
		return -1;
		
	// configure baud
	if (cfgUSB(fd))
	{ 
		close(fd);
		return -1;
	}

	return fd;
}

// 38 400 baud
// 8n1 raw

// usb port things
int cfgUSB(int fd)
{
	// termios struct will hold current settings
	struct termios cfg;

	// fetch current attributes
	LOG_DEBUG("initializing usb");
	if (tcgetattr(fd, &cfg))
	{
		LOG_ERROR("failed to get usb attributes");
		return -1;
	}

	// this sets baud to 38400(hz) its not hz but its bascially hz shut up
	cfsetispeed(&cfg, B38400);
	cfsetospeed(&cfg, B38400);

	// enable local device (whatever is on usb0) & ignore modem control
	cfg.c_cflag |= CLOCAL | CREAD;

	// clear size/parity/stop/control flags then pick 8 data bits
	cfg.c_cflag &= ~((tcflag_t)(CSIZE | PARENB | CSTOPB | CRTSCTS));
	cfg.c_cflag |= CS8;

	// disables software flow control
	cfg.c_iflag &= ~(tcflag_t)(IXON | IXOFF | IXANY);

	// raw input no cannon no echo no signals
	cfg.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ECHOE | ISIG);

	// disable post processing
	cfg.c_oflag &= ~(tcflag_t)OPOST;

	// return asap
	cfg.c_cc[VMIN] = 0; // 0 bytes minimum
	cfg.c_cc[VTIME] = 10; // 1.0 s timeout

	// push modified attributes immediately
	LOG_DEBUG("setting baud rate");
	if (tcsetattr(fd, TCSANOW, &cfg))
	{
		LOG_ERROR("failed to set baud rate");
		return -1;
	}

	LOG_SUCCESS("usb baud fixed to 38400");
	return 0;
}

// written in accordance to elm docs
ssize_t sendElmCmd(int fd, const char *cmd, char *resp, size_t max, useconds_t delayUs, bool debug)
{
	// calc length of cmd without carriage return
	size_t cmdLen = strlen(cmd);
	// alloc local buffer for cmd cr
	// 128 bytes is usually fine
	char outBuf[128];

	// format cmd into outBuf and append line break
	// makes it easier to parse ourput dont have to do it every time lol
	snprintf(outBuf, sizeof(outBuf), "%s\r", cmd);

	if (debug)
		LOG_ELM("TX ASCII: %s", cmd);

	// write cmd cr to elm
	if (write(fd, outBuf, cmdLen + 1) < 0)
	{
		LOG_ERROR("failed to write command to elm");
		return -1;
	}

	// wait for the device...
	usleep(delayUs);

	ssize_t total = 0; // total bytes read so far
	int idleLoops = 0; // count of consecutive read timeouts

	// read until buffer almost full or > cr seen
	while (total + 1 < (ssize_t)max)
	{
		// read one byte into resp at offset total
		ssize_t r = read(fd, resp + total, 1);
		
		// bad data shouldnt happen function safety
		if (r < 0)
		{
			LOG_ERROR("elm communication failed");
			return -1;
		}

		// no data
		if (r == 0)
		{
			// if too many idles assume eor
			if (++idleLoops > 20)
				break;

			// not sure how long this should be
			usleep(50000);
			continue;
		}

		idleLoops = 0; // reset idle counter when data recieved
		total += r; // this should be 1

		// if prompt > received that is eor the elm is done
		if (resp[total - 1] == '>')
			break;
	}

	// clamp total to max-1 to reserve for nul
	if (total >= (ssize_t)max)
		total = (ssize_t)max - 1;

	// null-terminate response
	resp[total] = '\0';

	if (debug)
	{
		// ensure non-negative cast for size_t so that the data is not
		// lost when printing
		size_t dataLen = (total > 0) ? (size_t)total : 0;

		char ascii[512];
		toASCII(resp, dataLen, ascii, sizeof(ascii));
		LOG_ELM("RX ASCII: %s", ascii);

		char hex[1024];
		toHEX((const unsigned char*)resp, dataLen, hex, sizeof(hex));
		LOG_ELM("RX HEX: %s", hex);
	}

	return total;
}
