#include "fsocket.h"
#include "pipe.h"
#include "utils/zmalloc.h"
#include "utils/anet.h"
#include "utils/adlist.h"

fsocket_t *fsocket_new(fsocket_ctx_t *ctx) {
	fsocket_t *s = c_new(fsocket_t);

	s->ctx = ctx;
	s->pipe = NULL;
	s->pipes = listCreate();
	s->in_frames = queue_create();

	return s;
}

void fsocket_destroy(fsocket_t *s) {
	if (s->pipe) {
		printf("[fsocket.c:%d] calling close for: %p\n", __LINE__, s->pipe);
		fsocket_pipe_close(s->pipe);
	}

	listNode *node;
	listIter *it = listGetIterator(s->pipes, 0);

	while((node = listNext(it)) != NULL) {
		printf("[fsocket.c:%d] calling decref for: %p\n", __LINE__, node->value);
		fsocket_pipe_close((fsocket_pipe_t *)node->value);
	}

	listReleaseIterator(it);
	listRelease(s->pipes);
	queue_destroy(s->in_frames);
	zfree(s);
}

int fsocket_bind(fsocket_t *socket, char *addr, int port) {
	fsocket_ctx_t *ctx = socket->ctx;
	socket->pipe = fsocket_pipe_new();
	socket->pipe->parent = socket;
	//printf("[server] in_frames: %p\n", socket->in_frames);
	printf("[server] pipe: %p\n", socket->pipe);

	char err[256] = "\0";
	int fd;

	fd = anetTcpServer(err, port, addr, 256);

	if (fd < 0)
		return FSOCKET_ERR_ERRNO;

	socket->pipe->fd = fd;
	socket->pipe->ctx = ctx;

	ev_io *a = &socket->io_read;
	a->data = socket;
	ev_io_init(a, fsocket_pipe_accept, fd, EV_READ);

	fsocket_ctx_start_io(ctx, a);

	fsocket_ctx_inc_active(ctx, socket->pipe);

	return FSOCKET_OK;
}

int fsocket_connect(fsocket_t *socket, char *addr, int port) {
	fsocket_ctx_t *ctx = socket->ctx;

	char err[256] = "\0";
	int fd;

	fd = anetTcpNonBlockConnect(err, addr, port);

	if (fd <= 0)
		return FSOCKET_ERR_ERRNO;

	fsocket_pipe_t *new_pipe = fsocket_pipe_new();

	// setup pipe
	new_pipe->fd = fd;
	new_pipe->parent = socket;
	new_pipe->ctx = ctx;
	new_pipe->io_states.is_reading = 1;

	// setup i/o
	fsocket_pipe_init_io(ctx, new_pipe, fd);

	// add to socket's pipes
	socket->pipes = listAddNodeTail(socket->pipes, (void *)new_pipe);

	// for read event
	printf("[fsocket.c:%d] *not* calling incref for: %p\n", __LINE__, new_pipe);
	//fsocket_pipe_incref(new_pipe);

	printf("[fsocket.c:%d] pipe: %p\n", __LINE__, new_pipe);
	fsocket_ctx_inc_active(ctx, new_pipe);

	return FSOCKET_OK;
}

void fsocket_remove_pipe(fsocket_t *s, fsocket_pipe_t *p) {
	listNode *node = listSearchKey(s->pipes, (void *)p);

	if (!node)
		return;

	listDelNode(s->pipes, node);
	printf("[fsocket.c remove pipe] calling close for: %p\n", p);
	fsocket_pipe_close(p);
}

fsocket_frame_t *fsocket_pop_frame(fsocket_t *socket) {
	return queue_pop_left(socket->in_frames);
}

void fsocket_iterate_pipes(fsocket_t *s,
	void (cb)(fsocket_pipe_t *p, void *data), void *data) {

	listIter *it = listGetIterator(s->pipes, 0);
	listNode *n;

	while((n = listNext(it)) != NULL)
		cb(n->value, data);

	listReleaseIterator(it);
}