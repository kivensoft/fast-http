#ifndef __URL_ENCODE_H__
#define __URL_ENCODE_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t url_encode_length(const char *src, size_t src_len);

extern size_t url_component_encode_length(const char *src, size_t src_len);

extern size_t url_encode(const char *src, size_t src_len, char *dst, size_t dst_len);

extern size_t url_component_encode(const char *src, size_t src_len, char *dst, size_t dst_len);

extern size_t url_decode_length(const char* src, size_t src_len);

extern size_t url_decode(const char* src, size_t src_len, char* dst, size_t dst_len);

#ifdef __cplusplus
}
#endif

#endif  // __URL_ENCODE_H__