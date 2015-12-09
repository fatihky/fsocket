#ifndef _FSOCKET_TYPES_H_
#define _FSOCKET_TYPES_H_

#include "utils/libhl/queue.h"
#include "utils/adlist.h"
#include "utils/buffer.h"
#include "utils/cbuffer.h"

typedef struct fsocket_t fsocket_t;
typedef struct fsocket_ctx_t fsocket_ctx_t;
typedef struct fsocket_pipe_s fsocket_pipe_t;

// frame

typedef enum {
	FSOCKET_FRAME_EMPTY,	 // empty frame
	FSOCKET_FRAME_PARTIAL, // we have some piece of the frame
	FSOCKET_FRAME_FULL		 // we have all of this frame
} fsocket_frame_type_t;

typedef struct {
	// FSOCKET_HEADERS_SIZE region start
	uint32_t size;
	// FSOCKET_HEADERS_SIZE region end
	fsocket_frame_type_t type;
	void *data;

	uint32_t cursor;
	fsocket_pipe_t *pipe;
} fsocket_frame_t;

// parser

typedef enum {
	FSOCK_PROT_START = 0,
	FSOCK_PROT_END
} fsock_prot_state;

typedef struct fsocket_parser_t {
	size_t must_read;
	fsock_prot_state state;
	fsocket_frame_t frame;
} fsocket_parser_t;

// stream

typedef struct {
	queue_t *out_frames; // frames will be sent
	queue_t *out_progress; // frames are sending now
	queue_t *in_frames; // received frames
	fsocket_parser_t parser;
	nitro_buffer_t *in_buffer;
	uint64_t in_buffer_cursor; // bunu silmem lazım hacı
	fsocket_pipe_t *pipe;
} fsocket_stream_t;

// pipe

typedef enum {
	FSOCKET_PIPE_ACTIVE,
	FSOCKET_PIPE_DEAD
} fsocket_pipe_type_t;

struct fsocket_pipe_s {
	int fd;
	int ref_count;

	fsocket_ctx_t *ctx;
	struct fsocket_t *parent;
	fsocket_stream_t stream;
	fsocket_pipe_type_t type;

	ev_io io_read;
	ev_io io_write;

	struct {
		uint8_t is_connected: 1;
		uint8_t is_writing:1;
		uint8_t is_reading:1;
	} io_states;
};

// fsocket

struct fsocket_t {
	list *pipes;
	fsocket_pipe_t *pipe;
	fsocket_ctx_t *ctx;
	queue_t *in_frames;
	ev_io io_read;
};

struct fsocket_ctx_t {
	EV_P;
	ev_async async;
	void (*routine)(void *data);
	int activecnt;
	list *pipes;
};

#endif