#include <assert.h>
#include "../src/fsocket.h"
#include "../src/utils/adlist.h"

fsocket_t *client;

void ctx_routine(void *data) {
	fsocket_frame_t *frame;

	// sonra istemciye gelenler
	while(client != NULL && (frame = fsocket_pop_frame(client)) != NULL) {
		printf("[client] Received: %.*s\n", frame->size, (char *)frame->data);
		printf("[client] calling frame destroy for: %p\n", frame->pipe);
		fsocket_frame_destroy(frame);
		printf("\nolm ben burada arıyorum bu niye beni dinlemiyor?\n\n");
		fsocket_destroy(client);
		client = NULL;
	}
}

int main(int argc, char *argv[]) {
	fsocket_ctx_t *ctx = fsocket_ctx_new();
	fsocket_pipe_t *pipe;
	listNode *lNode;
	int ret;

	client = fsocket_new(ctx);

	ret = fsocket_connect(client, "0.0.0.0", 2657);
	assert(ret == FSOCKET_OK);

	lNode = listIndex(client->pipes, 0);

	pipe = lNode->value;
	ret = fsocket_pipe_send(pipe, "Hello fsocket!", 14);

	ctx->routine = ctx_routine;

	fsocket_ctx_run(ctx, 0);
	fsocket_ctx_destroy(ctx);

	return 0;
}