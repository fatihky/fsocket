#include <assert.h>
#include "../src/fsocket.h"
#include "../src/utils/adlist.h"
#include "../src/utils/util.h"

fsocket_t *client;
int received;
long long start;

#define TOTAL 3000000

void ctx_routine(void *data) {
	fsocket_frame_t *frame;

	while(client != NULL && (frame = fsocket_pop_frame(client)) != NULL) {

	}
}

void on_frame(fsocket_t *self, fsocket_frame_t *frame)
{
	fsocket_frame_destroy(frame);

	received++;

	if (received == TOTAL) {
		long long end = mstime();
		long long duration = end - start;
		printf("Bench completed ========================\n");
		printf("Total messages sent(and received): %d\n", TOTAL);
		printf("Duration %lld\n", duration);
		printf("Throughput %lld ops/sec\n", (TOTAL * 1000ULL) / duration);
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
	client->on_frame = on_frame;

	ret = fsocket_connect(client, "0.0.0.0", 2657);
	assert(ret == FSOCKET_OK);

	lNode = listIndex(client->pipes, 0);

	pipe = lNode->value;

	int i = 0;
	received = 0;

	for (; i < TOTAL; i++) {
		ret = fsocket_pipe_send(pipe, "Hello fsocket!", 14);
	}

	ctx->routine = ctx_routine;

	start = mstime();

	fsocket_ctx_run(ctx, 0);
	fsocket_ctx_destroy(ctx);

	return 0;
}