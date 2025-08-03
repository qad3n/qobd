#ifndef INFO_H
#define INFO_H

#include <stddef.h>

int readVIN(int fd, char *vin, size_t len);
int readRPM(int fd, int *rpmOut);

#endif
