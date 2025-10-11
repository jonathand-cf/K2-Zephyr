#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32_ieee(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
