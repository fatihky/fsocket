#ifndef _FSOCKET_FRAME_H_
#define _FSOCKET_FRAME_H_

#include "types.h"

// currently we only send 4 bytes length
#define FSOCKET_FRAME_HEADERS_SIZE 4
#define FSOCKET_FRAME_HEADERS_START(frame) (&frame->size)
// TODO: yarım yazılanların boyutları az hesaplanacak
#define FSOCKET_FRAME_TOTAL_SIZE(frame) (FSOCKET_FRAME_HEADERS_SIZE + frame->size)

fsocket_frame_t *fsocket_frame_new(uint32_t size, char *data);
void fsocket_frame_init(fsocket_frame_t *frame);
void fsocket_frame_deinit(fsocket_frame_t *self);
void fsocket_frame_destroy(fsocket_frame_t *self);

#endif