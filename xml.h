#pragma once
#ifndef __XML_H__
#define __XML_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum xml_on_type_t {
    XML_ON_ELEMENT_BEGIN, XML_ON_ATTR_NAME, XML_ON_ATTR_VALUE, XML_ON_CHARACTERS, XML_ON_ELEMENT_END
} xml_on_type_t;

typedef void (*xml_callback_t) (xml_on_type_t type, const char* at, size_t len, _Bool isContinue);

typedef enum {
    XML_ON_DOCTYPE,
    XML_ON_ELEMENT_BEGIN,
    XML_ON_ATTRIBUTE,
    XML_ON_VALUE,
    XML_ON_CHARACTERS,
    XML_ON_ELEMENT_END,
    XML_ON_PARSER_ERROR
} xml_parser_state_t;

typedef struct {
    uint8_t parser_state;

    xml_callback_t on_callback;
} xml_parser_t;

extern void xml_parser_init(xml_parser_t* parser, xml_callback_t callback);

extern void xml_parser(xml_parser_t* parser, const char* src, size_t src_len);

#ifdef __cplusplus
}
#endif

#endif // __XML_H__