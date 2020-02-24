#ifndef PACKDATA_H
#define PACKDATA_H

#include <stdint.h>

void packi16(unsigned char *buf, uint16_t i);
void packi32(unsigned char *buf, uint32_t i);
void packi64(unsigned char *buf, uint64_t i);
int16_t unpacki16(unsigned char *buf);
uint16_t unpacku16(unsigned char *buf);
int32_t unpacki32(unsigned char *buf);
uint32_t unpacku32(unsigned char *buf);
int64_t unpacki64(unsigned char *buf);
uint64_t unpacku64(unsigned char *buf);
unsigned int pack(unsigned char *buf, char *format, ...);
void unpack(unsigned char *buf, char *format, ...);

#endif // PACKDATA_H
