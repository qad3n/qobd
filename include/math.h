#ifndef QOBD_MATH_H
#define QOBD_MATH_H

#include <stddef.h>
#include <string.h>

// stolen and reformatted

static inline char *toASCII(const char *src, size_t srcLen, char *dst, size_t dstSize)
{
	if (!src || !dst || dstSize == 0)
		return dst;

	size_t len = (srcLen < dstSize - 1) ? srcLen : (dstSize - 1);
	memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

inline char *toHEX(const unsigned char *bytes, size_t byteLen, char *hexBuf, size_t hexBufSize)
{
	if (bytes == NULL || hexBuf == NULL || hexBufSize == 0)
		return hexBuf;

	size_t requiredLen = byteLen * 3 + 1;

	if (hexBufSize < requiredLen)
	{
		byteLen = (hexBufSize - 1) / 3;
		if (byteLen == 0)
		{
			hexBuf[0] = '\0';
			return hexBuf;
		}
	}

	size_t currentPos = 0;
	for (size_t i = 0; i < byteLen; i++)
	{
		int w = sprintf(&hexBuf[currentPos], "%02X ", bytes[i]);
		
		if (w < 0)
			break;
			
		currentPos += (size_t)w;
	}

	if (currentPos > 0 && hexBuf[currentPos - 1] == ' ')
		currentPos--;

	hexBuf[currentPos] = '\0';

	return hexBuf;
}

#endif // QOBD_MATH_H
