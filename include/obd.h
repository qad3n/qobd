#ifndef OBD_H
#define OBD_H

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h> 

int initOBD(int fd);
int readPID(int fd, const char *elmCmd, unsigned char *bytes, size_t needed, unsigned int timeoutUs);

#endif