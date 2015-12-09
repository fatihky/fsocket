#ifndef _FSOCKET_STREAM_H_
#define _FSOCKET_STREAM_H_

#include "utils/libhl/queue.h"
#include "types.h"

fsocket_stream_t *fsocket_stream_new(void *data);
void fsocket_stream_destroy(fsocket_stream_t *self);
void fsocket_stream_init(fsocket_stream_t *self);
void fsocket_stream_deinit(fsocket_stream_t *self);
void fsocket_stream_append_frame(fsocket_stream_t *self, fsocket_frame_t *frame);
char *fsocket_stream_get_append_ptr(fsocket_stream_t *self, int *growth);
void fsocket_stream_extend(fsocket_stream_t *self, int bytes);
struct iovec *fsocket_stream_get_out_vectors(fsocket_stream_t *self, uint32_t max_size, int *count);
void fsocket_stream_clear_out_progress(fsocket_stream_t *self, size_t written);

#endif