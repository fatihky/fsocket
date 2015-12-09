#ifndef _FSOCKET_PIPE_H_
#define _FSOCKET_PIPE_H_

#include "types.h"

void fsocket_pipe_init(fsocket_pipe_t *self);
void fsocket_pipe_deinit(fsocket_pipe_t *self);

fsocket_pipe_t *fsocket_pipe_new();

void fsocket_pipe_incref(fsocket_pipe_t *p);
void fsocket_pipe_decref(fsocket_pipe_t *p);

int fsocket_pipe_send(fsocket_pipe_t *pipe, char *data, uint32_t size);
int fsocket_pipe_close(fsocket_pipe_t *pipe);

void fsocket_pipe_accept(EV_P_ ev_io *a, int revents);
void fsocket_pipe_init_io(fsocket_ctx_t *ctx, fsocket_pipe_t *pipe, int fd);

#endif