#ifndef _FSOCKET_H
#define _FSOCKET_H

#include <string.h>
#include <stdint.h>
#include <ev.h>

typedef enum {
	FSOCK_SERVER,
	FSOCK_CONN,
	FSOCK_CLI
} fsock_type_t;

typedef struct {
	void *data;
	uint32_t size;
} fsock_frame_t;

typedef struct fsock_st {
	fsock_type_t type;
	void		*data; // unused by fsocket library
	void		*_prv; // private fields
} fsock_t;

// handler types
typedef void (*fsock_conn_handler_t)(fsock_t *self, fsock_t *conn);
typedef void (*fsock_frame_handler_t)(fsock_t *self, fsock_t *conn, fsock_frame_t *frame);

fsock_t *fsock_bind(EV_P_ char *addr, int port);
fsock_t *fsock_connect(EV_P_ char *addr, int port);
void fsock_destroy(fsock_t *self);

void fsock_send(fsock_t *sock, void *data, uint32_t size);
void fsock_on_conn(fsock_t *self, fsock_conn_handler_t handler);
void fsock_on_frame(fsock_t *self, fsock_frame_handler_t handler);
void fsock_on_disconnect(fsock_t *self, fsock_conn_handler_t handler);

#endif