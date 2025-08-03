#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

int cfgUSB(int fd);
int initUSB(const char *path);
ssize_t sendElmCmd(int fd, const char *cmd, char *resp, size_t max, useconds_t delayUs, bool debug);

#endif // USB_H
