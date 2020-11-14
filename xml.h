#pragma once
#ifndef __XML_H__
#define __XML_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XML_ON_ELEMENT_BEGIN, XML_ON_ATTR_NAME, XML_ON_ATTR_VALUE, XML_ON_CHARACTERS, XML_ON_ELEMENT_END
} xml_on_type_t;

typedef void (*xml_callback_t) (xml_on_type_t type, const char* at, size_t len, _Bool isContinue);

typedef enum {
    XML_DOC_BEGIN,
    XML_ELEMENT_BEGIN,
    XML_ATTRIBUTE_BEGIN,
    XML_ATTRIBUTE_END,
    XML_CHARACTERS_BEGIN,
    XML_CHARACTERS_END,
    XML_ELEMENT_END,
    XML_DOC_END
} xml_parser_state_t;

typedef struct {
    uint8_t parser_state;

    xml_callback_t on_callback;
} xml_parser_t;

extern void xml_parser_init(xml_parser_t* parser);

extern bool xml_update(xml_parser_t* parser, const char* src, size_t src_len);

extern bool xml_final(xml_parser_t* parser, const char* src, size_t src_len);

#ifdef __cplusplus
}
#endif

#endif // __XML_H__