#ifndef _FSOCKET_H_
#define _FSOCKET_H_

#include "types.h"
#include "ctx.h"

#define FSOCKET_OK 0
#define FSOCKET_ERR_ERRNO 1 // look errno for error details
#define FSOCKET_ERR_PIPE_DEAD 2 // pipe already closed

fsocket_t *fsocket_new(fsocket_ctx_t *ctx);
void fsocket_destroy(fsocket_t *s);
int fsocket_bind(fsocket_t *s, char *addr, int port);
int fsocket_connect(fsocket_t *s, char *addr, int port);

fsocket_frame_t *fsocket_pop_frame(fsocket_t *socket);
void fsocket_iterate_pipes(fsocket_t *s,
	void (cb)(fsocket_pipe_t *p, void *data), void *data);
void fsocket_remove_pipe(fsocket_t *s, fsocket_pipe_t *p);

#endif