#include <stdarg.h>
#include <stdio.h>

#include "logger.h"

static const char *colors[] =
{
	[LOG_DEFAULT] = DEFAULT,
	[LOG_ERROR] = ERROR,
	[LOG_WARNING] = WARNING,
	[LOG_SUCCESS] = SUCCESS,
	[LOG_ELM] = ELM_RESP,
	[LOG_DEBUG] = DEBUG
};

// buggy; sometimes does not revert to default color
void logPrint(log_level_t level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	printf("%s", colors[level]);
	vprintf(fmt, args);
	va_end(args);

	printf("%s\n", RESET);
}
