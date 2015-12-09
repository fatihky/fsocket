#include "frame.h"

fsocket_frame_t *fsocket_frame_new(uint32_t size, char *buffer) {
	fsocket_frame_t *frame = malloc(sizeof(fsocket_frame_t));
	if (frame == NULL)
		return NULL;

	fsocket_frame_init(frame);

	frame->data = malloc(size);

	if (frame->data == NULL) {
		free(frame);
		return NULL;
	}

	memcpy(frame->data, buffer, size);

	frame->type = FSOCKET_FRAME_FULL;
	frame->size = size;

	return frame;
}

void fsocket_frame_init(fsocket_frame_t *frame) {
	frame->type = FSOCKET_FRAME_EMPTY;
	frame->data = NULL;
	frame->size = 0;
	frame->cursor = 0;
	frame->pipe = NULL;
}

void fsocket_frame_deinit(fsocket_frame_t *self) {
	if (self->data != NULL)
		free(self->data);
	if (self->pipe) {
		printf("[frame.c] calling decref for: %p\n", self->pipe);
		fsocket_pipe_decref(self->pipe);
	}
	self->size = 0;
}

void fsocket_frame_destroy(fsocket_frame_t *self) {
	fsocket_frame_deinit(self);
	free(self);
}