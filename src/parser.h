#ifndef _FSOCKET_PARSER_H_
#define _FSOCKET_PARSER_H_

#include "types.h"

void fsocket_parser_init(fsocket_parser_t *parser);
void fsocket_parser_deinit(fsocket_parser_t *self);
void fsocket_parser_parse(fsocket_stream_t *self);

#endif