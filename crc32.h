#pragma once
#ifndef __CRC32_H__
#define __CRC32_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t crc32(const void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif  // __CRC32_H__