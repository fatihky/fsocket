#ifndef _FSOCKET_INTERNAL_H
#define _FSOCKET_INTERNAL_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <assert.h>
#include <queue.h>
#include <ev.h>
#include "zmalloc.h"
#include "anet.h"
#include "sds.h"
#include "debug.h"
#include "light-lock.h"

#define FSOCK_TCP_INBUF (16 * 1024)
#define FSOCK_TCP_OBUF (32 * 1024)

extern ev_io *io__;

typedef enum
{
	FSOCK_PROT_START = 0,
	FSOCK_PROT_END
} fsock_prot_state;

typedef struct {
	EV_P;
	int fd;
	union {
		 struct {
				ev_io ioa; // accept event
		 };
		 struct {
			ev_io ior; // read event
			ev_io iow; // write event
			fsock_t *srv; // for connections
			uint32_t is_connected; // for clients
			uint32_t is_reading;
			uint32_t is_writing;
			uint32_t is_write_event_fired;
			light_lock_t o_lock; // output lock
		 };
	};

	char *addr;
	int port;

	// buffers
	sds in_buf;
	sds o_buf;

	// parser
	struct {
		size_t must_read;
		fsock_prot_state state;
		fsock_frame_t curr;
	} parser;

	// handlers
	fsock_conn_handler_t on_conn;
	fsock_conn_handler_t on_disconnect;
	fsock_frame_handler_t on_frame;

	// states
	int disconnecting;
} fsock_internal_t;

typedef struct {
	fsock_frame_t *frame;
	fsock_t *receiver;
	fsock_t *sender;
} fsock_frame_queue_item_t;

typedef struct {
	EV_P;
	pthread_t thread;
	ev_async async;
	ev_async in_async; // new connection handler
	ev_async o_async; // output handler
	ev_async close_async; // close handler
	queue_t *in_queue;
	queue_t *o_queue;
	queue_t *close_queue; // close queue
} fsock_thread;

typedef struct
{
	struct ev_loop *loop;
	int num_socks;
	ev_async awake;

	// queues
	queue_t *disconnect_queue;
	queue_t *frame_queue;

	// async handlers
	ev_async disconnect_async; // disconnect handler
	ev_async frame_async;
} fsock_main_thread;

// private global variables
extern int fsock_is_inited;
extern fsock_thread reader_thread;
extern fsock_main_thread main_thread;

fsock_t *fsock_new();

#define fsock_incr_sock_count() __sync_fetch_and_add(&main_thread.num_socks, 1)
#define fsock_decr_sock_count() if(__sync_fetch_and_sub(&main_thread.num_socks, 1) == 1) ev_async_stop(main_thread.loop, &main_thread.awake)

#endif