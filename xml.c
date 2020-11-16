#include "xml.h"

typedef enum {
    XML_ON_DOC_START = XML_ON_PARSER_ERROR + 1,
    XML_ON_TAG_START,
} _xml_parser_state_ex_t;

/** 跳过空格 */
static const char* _skip_space(const char* start, const char* end) {
    for (; start < end; ++start) {
        uint8_t c = *start;
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            break;
    }
    return start;
}

/** */
static const char* _locate_xml_begin(xml_parser_t* self, const char* start, const char* end) {
    start = _skip_space(start, end);
    if (start >= end)
        return start;
    
    char ch = *start++;
    if (ch != '<')
        return NULL;
    else
        self->parser_state = XML_ON_TAG_START;

    ch = *start++;
    if (ch)
}

void xml_parser_init(xml_parser_t* parser, xml_callback_t callback) {

}

void xml_parser(xml_parser_t* parser, const char* src, size_t src_len) {
    const char* src_end = src + src_len;
    while (src < src_end) {
        switch (parser->parser_state) {
            case XML_ON_ELEMENT_BEGIN:
                break;
            case XML_ON_ATTRIBUTE:
                break;
            case XML_ON_VALUE:
                break;
            case XML_ON_CHARACTERS:
                break;
            case XML_ON_ELEMENT_END:
                break;
            case XML_ON_DOC_START:
                /* code */
                break;
            case XML_ON_TAG_START:
                break;
            
            default:
                break;
        }
    }
}
