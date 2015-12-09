#ifndef _FSOCKET_CTX_H_
#define _FSOCKET_CTX_H_

#include "types.h"

fsocket_ctx_t *fsocket_ctx_new();
void fsocket_ctx_destroy(fsocket_ctx_t *ctx);
void fsocket_ctx_start_io(fsocket_ctx_t *ctx, ev_io *io);
void fsocket_ctx_stop_io(fsocket_ctx_t *ctx, ev_io *io);
void fsocket_ctx_run(fsocket_ctx_t *ctx, int run_once);
void fsocket_ctx_inc_active(fsocket_ctx_t *ctx, fsocket_pipe_t *p);
void fsocket_ctx_dec_active(fsocket_ctx_t *ctx, fsocket_pipe_t *p);
void posix_print_stack_trace();

#endif