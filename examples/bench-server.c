#include <assert.h>
#include "../src/fsocket.h"
#include "../src/utils/adlist.h"

fsocket_t *server;

void remove_closed_pipes(fsocket_pipe_t *p, void *data) {
	(void)data;

	if (p->type != FSOCKET_PIPE_DEAD)
		return;

	fsocket_t *socket = p->parent;

	fsocket_remove_pipe(p->parent, p);

	//fsocket_destroy(server);
	//server = NULL;
}

void ctx_routine(void *data) {
	fsocket_frame_t *frame;

	while(server != NULL && (frame = fsocket_pop_frame(server)) != NULL) {
		//printf("[server] Received: %.*s\n", frame->size, (char *)frame->data);
		fsocket_pipe_send(frame->pipe, "Wow such fsocket enabled socket! Welcome!", 41);
		fsocket_frame_destroy(frame);
	}

	// remove closed pipes
	if (server != NULL)
		fsocket_iterate_pipes(server, remove_closed_pipes, NULL);
}

int main(int argc, char *argv[]) {
	fsocket_ctx_t *ctx = fsocket_ctx_new();
	fsocket_pipe_t *pipe;
	listNode *lNode;
	int ret;

	server = fsocket_new(ctx);

	ret = fsocket_bind(server, "0.0.0.0", 2657);
	assert(ret == FSOCKET_OK);

	ctx->routine = ctx_routine;

	fsocket_ctx_run(ctx, 0);
	fsocket_ctx_destroy(ctx);

	return 0;
}