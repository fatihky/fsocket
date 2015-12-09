#include <assert.h>
#include "../src/fsocket.h"
#include "../src/utils/adlist.h"

fsocket_t *server;
fsocket_t *client;

void remove_closed_pipes(fsocket_pipe_t *p, void *data) {
	(void)data;

	if (p->type != FSOCKET_PIPE_DEAD)
		return;

	fsocket_t *socket = p->parent;

	printf("[basic:%d] calling remove for: %p\n", __LINE__, p);
	fsocket_remove_pipe(p->parent, p);

	//printf("[basic:%d] *not* calling close for: %p\n", __LINE__, socket->pipe);
	//fsocket_pipe_close(socket->pipe);

	printf("[basic.c:%d] destroying %d %d\n", __LINE__,
		server->pipe->fd, server->io_read.fd);

	fsocket_destroy(server);
	server = NULL;
	printf("server is *not* still running\n");
}

void ctx_routine(void *data) {
	fsocket_frame_t *frame;
	// önce sunucuya gelen mesajlar.....
	while(server != NULL && (frame = fsocket_pop_frame(server)) != NULL) {
		printf("[server] Received: %.*s\n", frame->size, (char *)frame->data);
		fsocket_pipe_send(frame->pipe, "Wow such fsocket enabled socket! Welcome!", 41);
		fsocket_frame_destroy(frame);
	}

	// sonra istemciye gelenler
	while(client != NULL && (frame = fsocket_pop_frame(client)) != NULL) {
		printf("[client] Received: %.*s\n", frame->size, (char *)frame->data);
		// bağlantı kopmalı burada. bakalım nasıl kopacak?!....
		printf("[basic] fsocket_frame_destroy çalıştırılıyor: %p\n", frame->pipe);
		fsocket_frame_destroy(frame);
		fsocket_destroy(client);
		client = NULL;
	}

	// kapanan bağlantıları sil
	if (server != NULL)
		fsocket_iterate_pipes(server, remove_closed_pipes, NULL);

	// şimdi bağlantı kopma uyarısı
	// gerisi rock'n roll
}

int main(int argc, char *argv[]) {
	fsocket_ctx_t *ctx = fsocket_ctx_new();
	fsocket_pipe_t *pipe;
	listNode *lNode;
	int ret;

	server = fsocket_new(ctx);
	client = fsocket_new(ctx);

	ret = fsocket_bind(server, "0.0.0.0", 2657);
	assert(ret == FSOCKET_OK);

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