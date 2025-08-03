#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stddef.h>

#include "colors.h"

typedef enum
{
	LOG_DEFAULT,
	LOG_ERROR,
	LOG_WARNING,
	LOG_SUCCESS,
	LOG_ELM,
	LOG_DEBUG
} log_level_t;

void logPrint(log_level_t level, const char *fmt, ...);

#define LOG_ERROR(...) logPrint(LOG_ERROR, __VA_ARGS__)
#define LOG_WARNING(...) logPrint(LOG_WARNING, __VA_ARGS__)
#define LOG_SUCCESS(...) logPrint(LOG_SUCCESS, __VA_ARGS__)
#define LOG_ELM(...) logPrint(LOG_ELM, __VA_ARGS__)
#define LOG_DEBUG(...) logPrint(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)logPrint(LOG_DEFAULT, __VA_ARGS__)

#endif
