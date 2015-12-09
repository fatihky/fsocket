#include "utils/zmalloc.h"
#include "pipe.h"
#include "stream.h"
#include "fsocket.h"
#include "frame.h"

#define FSOCKET_TCP_INBUF (64 * 1024)

// uncomment this if you want to see logs about reference counts
//#define REF_DEBUG

#ifdef FSOCKET_PIPE_DEBUG
#	define debug(...) printf(__VA_ARGS__)
#else
#	define debug(...)
#endif

#ifdef REF_DEBUG
#	define ref_debug debug
#else
#	define ref_debug(...)
#endif

static void fsocket_pipe_write(EV_P_ ev_io *io, int revents);
static void fsocket_pipe_read(EV_P_ ev_io *io, int revents);

void fsocket_pipe_init(fsocket_pipe_t *pipe) {
	fsocket_stream_init(&pipe->stream);
	pipe->type = FSOCKET_PIPE_ACTIVE;
	pipe->ref_count = 1;
	pipe->stream.pipe = pipe;
	pipe->fd = -1;
	pipe->ctx = NULL;
}

void fsocket_pipe_deinit(fsocket_pipe_t *self) {
	self->type = FSOCKET_PIPE_DEAD;
	fsocket_stream_deinit(&self->stream);

	if (self->fd != -1) {
		close(self->fd);
	}
}

fsocket_pipe_t *fsocket_pipe_new() {
	fsocket_pipe_t *p = c_new(fsocket_pipe_t);

	fsocket_pipe_init(p);

	return p;
}

void fsocket_pipe_incref(fsocket_pipe_t *p) {
	p->ref_count += 1;
	ref_debug("[pipe incref] ref_count: %d (pipe: %p)\n", p->ref_count, p);
}

void fsocket_pipe_decref(fsocket_pipe_t *p) {
	assert(p->ref_count > 0);
	int is_bind = p->parent->pipe == p;

	p->ref_count -= 1;
	ref_debug("[pipe decref] ref_count: %d (pipe: %p, is_bind: %d)\n", p->ref_count, p, is_bind);

	if (p->ref_count > 0)
		return;

	if (is_bind)
		p->parent->pipe = NULL;

	fsocket_pipe_deinit(p);
	fsocket_ctx_dec_active(p->parent->ctx, p);
	if (is_bind)
		zfree(p);
	else
		zfree(p);
}

int fsocket_pipe_close(fsocket_pipe_t *pipe) {
	int is_bind = pipe->parent->pipe == pipe;

	if (pipe->type != FSOCKET_PIPE_DEAD) {
		debug("pipe->type != FSOCKET_PIPE_DEAD\nstopping io for this pipe. because we are great!\n");

		close(pipe->fd);

		if (is_bind)
			fsocket_ctx_stop_io(pipe->parent->ctx, &pipe->parent->io_read);
		else {
			if (pipe->io_states.is_reading) {
				debug("okumayı da durdurdum\n");
				pipe->io_states.is_reading = 0;
		 		fsocket_ctx_stop_io(pipe->parent->ctx, &pipe->io_read);
			} else {
				debug("okuma kendi durmuş yav. hll süpr dvm\n");
			}
		}
	
		// if this is bind pipe, pipe->parent->pipe equals to pipe
		// so we should not try to stop write events of bind pipes
	 	if (!is_bind) {
	 		if (pipe->io_states.is_writing) {
	 			debug("yazmayı da durdurdum\n");
	 			pipe->io_states.is_writing = 0;
			 	fsocket_ctx_stop_io(pipe->parent->ctx, &pipe->io_write);
	 		} else {
				debug("yazma kendi durmuş yav. hll süpr dvm\n");
			}
	 	}
	}

	pipe->type = FSOCKET_PIPE_DEAD;
	debug("[pipe.c close] calling decref for: %p\n", pipe);
	fsocket_pipe_decref(pipe);

	return FSOCKET_OK;
}

int fsocket_pipe_send(fsocket_pipe_t *pipe, char *data, uint32_t size) {
	if (pipe->type == FSOCKET_PIPE_DEAD)
		return FSOCKET_ERR_PIPE_DEAD;

	fsocket_frame_t *frame = fsocket_frame_new(size, data);

	queue_push_right(pipe->stream.out_frames, frame);

	if (!pipe->io_states.is_writing) {
		pipe->io_states.is_writing = 1;
		fsocket_ctx_start_io(pipe->parent->ctx, &pipe->io_write);
	}

	return FSOCKET_OK;
}

void fsocket_pipe_accept(EV_P_ ev_io *a, int revents) {
	fsocket_t *socket = (fsocket_t *)a->data;
	fsocket_pipe_t *pipe = socket->pipe;
	fsocket_ctx_t *ctx = socket->ctx;

	int port;
	char ip[256];

	//debug("Handle new connection\n");

	int fd = anetTcpAccept(NULL, pipe->fd, ip, 256, &port);

	if (fd < 0)
		return;

	// setup fd
	anetNonBlock(NULL, fd);
	anetEnableTcpNoDelay(NULL, fd);

	fsocket_pipe_t *new_pipe = fsocket_pipe_new();

	// setup pipe
	new_pipe->fd = fd;
	new_pipe->parent = socket;
	new_pipe->ctx = ctx;
	new_pipe->io_states.is_reading = 1;

	// setup i/o
	fsocket_pipe_init_io(ctx, new_pipe, fd);

	// increase pipe's reference count for read event
	debug("[pipe accept] calling incref for: %p\n", new_pipe);
	fsocket_pipe_incref(new_pipe);

	// add to socket's pipes
	socket->pipes = listAddNodeTail(socket->pipes, (void *)new_pipe);

	debug("[connection] pipe: %p\n", new_pipe);
	fsocket_ctx_inc_active(ctx, new_pipe);

	//debug("New connection from: %s:%d\n", ip, port);
}

void fsocket_pipe_init_io(fsocket_ctx_t *ctx, fsocket_pipe_t *pipe, int fd) {
	ev_io *io = &pipe->io_write;
	io->data = (void *)pipe;
	ev_io_init(io, fsocket_pipe_write, fd, EV_WRITE);

	io = &pipe->io_read;
	io->data = (void *)pipe;
	ev_io_init(io, fsocket_pipe_read, fd, EV_READ);
	fsocket_ctx_start_io(ctx, io);
}

static void fsocket_pipe_read(EV_P_ ev_io *io, int revents)
{
	(void)revents;

	fsocket_pipe_t *pipe = (fsocket_pipe_t *)io->data;
	debug("fsocket_pipe_read: %p\n", pipe);
	fsocket_t *parent = pipe->parent;
	fsocket_ctx_t *ctx = pipe->ctx;

	if (pipe->type == FSOCKET_PIPE_DEAD) {
		return;
	}

  int sz = FSOCKET_TCP_INBUF;
  char *append_ptr = fsocket_stream_get_append_ptr(&pipe->stream, &sz);

  int r = read(pipe->fd, append_ptr, sz);

  if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
  	// read again later
		return;
  }

  if (r <= 0) {
  	debug("[pipe.c read] calling close for: %p\n", pipe);
  	fsocket_pipe_close(pipe);
  	//ev_io_stop(EV_A_ io);
	return;
  }

  fsocket_stream_extend(&pipe->stream, r);
  debug("Readed: %d\n", r);
  fsocket_parser_parse(&pipe->stream);
}

static void fsocket_pipe_write(EV_P_ ev_io *io, int revents)
{
	(void)revents;

	fsocket_pipe_t *pipe = (fsocket_pipe_t *)io->data;
	fsocket_t *parent = pipe->parent;
	fsocket_ctx_t *ctx = parent->ctx;

	if (pipe->type == FSOCKET_PIPE_DEAD)
		return;

	if (queue_count(pipe->stream.out_frames) == 0) {
		pipe->io_states.is_writing = 0;
		fsocket_ctx_stop_io(ctx, &pipe->io_write);
		debug("queue_count(pipe->stream.out_frames) == 0\n");
		return;
	}

	int count = 0;
	struct iovec *vectors = fsocket_stream_get_out_vectors(&pipe->stream, 64 * 1024, &count);
	ssize_t w = writev(pipe->fd, vectors, count);

	fsocket_stream_clear_out_progress(&pipe->stream, w);

	zfree(vectors);
	debug("written: %zu\nvector count: %d\n", w, count);
}
