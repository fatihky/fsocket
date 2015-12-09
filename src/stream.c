#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <queue.h>
#include <ev.h>
#include <unistd.h>
#include "utils/debug.h"
#include "utils/zmalloc.h"
#include "utils/cbuffer.h"
#include "utils/buffer.h"
#include "utils/zmalloc.h"
#include "types.h"
#include "stream.h"
#include "frame.h"

#define FSOCKET_TCP_INBUF (64 * 1024)
#define FSOCKET_MAX_OUT_FRAMES 10

fsocket_stream_t *fsocket_stream_new(void *data) {
	fsocket_stream_t *stream = malloc(sizeof(fsocket_stream_t));
	if (stream != NULL)
		return NULL;

	fsocket_stream_init(stream);

	return stream;
}

void fsocket_stream_init(fsocket_stream_t *self) {
	self->pipe = NULL;
	self->in_buffer_cursor = 0;
	self->in_buffer = nitro_buffer_new();
	self->out_frames = queue_create();
	self->out_progress = queue_create();
	self->in_frames = queue_create();
	fsocket_parser_init(&self->parser);
}

void fsocket_stream_deinit(fsocket_stream_t *self) {
	nitro_buffer_destroy(self->in_buffer);
	queue_destroy(self->out_frames);
	queue_destroy(self->out_progress);
	queue_destroy(self->in_frames);
	fsocket_parser_deinit(&self->parser);
}

void fsocket_stream_destroy(fsocket_stream_t *self) {
	fsocket_stream_deinit(self);
	free(self);
}

char *fsocket_stream_get_append_ptr(fsocket_stream_t *self, int *growth) {
	return nitro_buffer_prepare(self->in_buffer, growth);
}

void fsocket_stream_extend(fsocket_stream_t *self, int bytes) {
	nitro_buffer_extend(self->in_buffer, bytes);
}

void fsocket_stream_append_frame(fsocket_stream_t *self, fsocket_frame_t *frame) {
	queue_push_right(self->out_frames, frame);
}

struct iovec *fsocket_stream_get_out_vectors(fsocket_stream_t *self, uint32_t max_size, int *count) {
	uint32_t total_size = 0;
	// TODO: add partial frames first
	int index = 0;
	fsocket_frame_t *frame;
	int vector_count = FSOCKET_MAX_OUT_FRAMES;
	struct iovec *vectors = zmalloc(vector_count * sizeof(struct iovec));

	if (vectors == NULL)
		return NULL;

	memset(vectors, '\0', vector_count * sizeof(struct iovec));

	frame = queue_pop_left(self->out_frames);

	do {
		queue_push_right(self->out_progress, frame);

		vectors[index].iov_len = FSOCKET_FRAME_HEADERS_SIZE;
		vectors[index].iov_base = FSOCKET_FRAME_HEADERS_START(frame);
		total_size += vectors[index].iov_len;

		// TODO: tam gönderilmeyenlerde ne kadar gönderildiğini yaz gözüm
		// sonra ne kadar gönderilmiş diye bakar nereden itibaren göndermen gerektiğine karar verirsin
		// Not: Parça parça gönderme tamamlandığında bu yorum kendi kendini imha
		// edecektir
		vectors[index + 1].iov_len = frame->size - frame->cursor;
		vectors[index + 1].iov_base = frame->data + frame->cursor;
		total_size += vectors[index + 1].iov_len;
		//printf("frame->cursor: %u\n", frame->cursor);

		frame = queue_pop_left(self->out_frames);

		index += 2;
	} while(frame && index < vector_count && total_size < max_size);

	if (frame)
		queue_push_left(self->out_frames, frame);

	*count = index;

	//printf("total_size: %u\n", total_size);

	return vectors;
}

// bütün frame'!eri kontrol et, tamamı gönderilemeyenleri yeniden
// gönderilecekler listesine al
void fsocket_stream_clear_out_progress(fsocket_stream_t *self, size_t written) {
	size_t remaining = written;
	uint32_t done = 0;

	fsocket_frame_t *frame;

	frame = queue_pop_left(self->out_progress);

	while(frame && !done) {
		uint32_t size = FSOCKET_FRAME_TOTAL_SIZE(frame);

		if (remaining < size) {
			frame->cursor += remaining;
			queue_push_left(self->out_frames, (void *)frame);
			break;
		}

		remaining -= size;

		fsocket_frame_destroy(frame);

		if (remaining == 0)
			done = 1;
		else
			frame = queue_pop_left(self->out_progress);
	}
}