#include "fsocket.h"
#include "fs_internal.h"
#include "io_handlers.h"

// int fsock_is_inited = false;
// fsock_thread reader_thread;
int fsock_is_inited = false;
fsock_thread reader_thread;
ev_io *io__ = NULL;

fsock_main_thread main_thread = {
	.loop = NULL,
	.num_socks = 0
};

// main_thread.loop = NULL;
// main_thread.num_socks = 0;

static void fsock_initialize();

// reader thread async watchers
static void reader_thread_in_async_cb(EV_P_ struct ev_io *a, int revents)
{
	(void)a;
	(void)revents;

	fsock_t *sock = (fsock_t *)queue_pop_left(reader_thread.in_queue);
	while(sock != NULL)
	{
		fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
		ev_io *io = &_prv->ior;
		io->data = (void *)sock;

		ev_io_init(io, io_read, _prv->fd, EV_READ);
		io->next = (struct ev_watcher_list *)io;
		if(__sync_bool_compare_and_swap(&_prv->is_reading, 0, 1))
		{
			ev_io_start(EV_A_ io);
		}
		sock = (fsock_t *)queue_pop_left(reader_thread.in_queue);
	}
}

static void reader_thread_o_async_cb(EV_P_ struct ev_io *a, int revents)
{
	(void)a;
	(void)revents;

	fsock_t *sock = (fsock_t *)queue_pop_left(reader_thread.o_queue);
	while(sock != NULL)
	{
		fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
		ev_io *io = &_prv->iow;
		if(__sync_bool_compare_and_swap(&_prv->is_writing, 0, 1))
		{
			ev_io_start(loop, io);
		}
		sock = (fsock_t *)queue_pop_left(reader_thread.o_queue);
	}
}

static void reader_thread_close_async_cb(EV_P_ struct ev_async *a, int revents)
{
	(void)a;
	(void)revents;
	fsock_t *sock = (fsock_t *)queue_pop_left(reader_thread.close_queue);
	while(sock != NULL)
	{
		fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
		if(_prv->is_writing)
			ev_io_stop(EV_A_ &_prv->iow);

		if(_prv->is_reading)
			ev_io_stop(EV_A_ &_prv->ior);

		close(_prv->fd);

		// add connection to reader thread's queue
		queue_push_right(main_thread.disconnect_queue, (void *)sock);
		// fire reader thread
		ev_async_send(main_thread.loop, &main_thread.disconnect_async);

		sock = (fsock_t *)queue_pop_left(reader_thread.close_queue);
	}
}

static void main_thread_disconnect_async_cb(EV_P_ struct ev_io *a, int revents)
{
	(void)a;
	(void)revents;
	(void)loop;

	fsock_t *sock = (fsock_t *)queue_pop_left(main_thread.disconnect_queue);
	while(sock != NULL)
	{
		fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
		if(sock->type == FSOCK_CONN)
		{
			fsock_internal_t *srv_prv = (fsock_internal_t *)_prv->srv->_prv;
			if(srv_prv->on_disconnect)
				srv_prv->on_disconnect(_prv->srv, sock);
		} else if(_prv->on_disconnect)
			_prv->on_disconnect(sock, NULL);

		// free memory
		zfree(_prv->addr);
		zfree(_prv);
		zfree(sock);
		fsock_decr_sock_count();
		sock = (fsock_t *)queue_pop_left(main_thread.disconnect_queue);
	}
}

static void main_thread_frame_async_cb(EV_P_ struct ev_io *a, int revents)
{
	(void)loop;
	(void)a;
	(void)revents;
	uint32_t i;
	uint32_t count = queue_count(main_thread.frame_queue);
	// FSOCK_LOG("there are %d elements in 'in_queue'", count);
	if(count == 0)
		return;

	for(i = 0; i < count; i++)
	{
		fsock_frame_queue_item_t *q_item = (fsock_frame_queue_item_t *)queue_pop_left(main_thread.frame_queue);
		fsock_t *sock = q_item->receiver;
		fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
		if(_prv->on_frame)
			_prv->on_frame(q_item->receiver, q_item->sender, q_item->frame);
		zfree(q_item);
	}
}

static void reader_thread_async_cb(struct ev_loop *loop, struct ev_io *a, int revents)
{
	(void)loop;
	(void)a;
	(void)revents;
	// FSOCK_LOG("reader_thread_async_cb: async event fired");
}

static void async_noop(struct ev_loop *loop, struct ev_io *a, int revents)
{
	(void)loop;
	(void)a;
	(void)revents;
	// FSOCK_LOG("reader_thread_async_cb: async event fired");
}

// reader thread
static void *fsock_reader_thread(void *arg)
{
	(void)arg;
	// FSOCK_LOG("fsock_reader_thread");
	// start loop
	uint32_t count = queue_count(reader_thread.o_queue);
	if(count > 0)
		ev_async_start(reader_thread.loop, &reader_thread.o_async);
	ev_run(reader_thread.loop, 0);
	return NULL;
}

static void fsock_initialize()
{
	if(fsock_is_inited == true)
		return;
	fsock_is_inited = true;

	// initialize reader thread's variables
	reader_thread.loop = ev_loop_new(0);
	reader_thread.in_queue = queue_create();
	reader_thread.o_queue = queue_create();
	reader_thread.close_queue = queue_create();
	// initialize disconnect queue
	main_thread.disconnect_queue = queue_create();
	// initialize frame queue
	main_thread.frame_queue = queue_create();

	// initialize async watchers
	ev_async *async = &reader_thread.async;
	ev_async_init(async, reader_thread_async_cb);
	ev_async_start(reader_thread.loop, async);

	async = &reader_thread.in_async;
	ev_async_init(async, reader_thread_in_async_cb);
	ev_async_start(reader_thread.loop, async);

	async = &reader_thread.o_async;
	ev_async_init(async, reader_thread_o_async_cb);
	ev_async_start(reader_thread.loop, async);

	async = &reader_thread.close_async;
	ev_async_init(async, reader_thread_close_async_cb);
	ev_async_start(reader_thread.loop, async);

	// start disconnect handler
	async = &main_thread.disconnect_async;
	ev_async_init(async, main_thread_disconnect_async_cb);
	ev_async_start(main_thread.loop, async);

	// start frame handler
	async = &main_thread.frame_async;
	ev_async_init(async, main_thread_frame_async_cb);
	ev_async_start(main_thread.loop, async);

	// start awake async handler
	async = &main_thread.awake;
	ev_async_init(async, async_noop);
	ev_async_start(main_thread.loop, async);

	// start thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&reader_thread.thread, &attr, fsock_reader_thread, NULL);
}

fsock_t *fsock_new()
{
	fsock_initialize();
	fsock_t *sock = zmalloc(sizeof(fsock_t));
	fsock_internal_t *_prv = zmalloc(sizeof(fsock_internal_t));
	memset(sock, '\0', sizeof(fsock_t));
	memset(_prv, '\0', sizeof(fsock_internal_t));
	sock->_prv = _prv;
	_prv->in_buf = sdsempty();
	_prv->o_buf = sdsempty();
	return sock;
}

fsock_t *fsock_bind(EV_P_ char *addr, int port)
{
	if(main_thread.loop == NULL)
		main_thread.loop = loop;
	else {
		assert(main_thread.loop == loop);
	}

	fsock_t *sock = fsock_new();
	fsock_internal_t *_prv = sock->_prv;
	sock->type = FSOCK_SERVER;

	char err[256] = "\0";
	int fd = anetTcpServer(err, port, addr, 1024);

	_prv->loop = loop;
	_prv->fd = fd;

	// initialize accept event
	ev_io *io = &_prv->ioa;
	io->data = (void *)sock;

	// start accept event
	ev_io_init(io, io_accept, fd, EV_READ);
	ev_io_start(EV_A_ io); // accept new connections from main event loop

	// increment sockets count
	fsock_incr_sock_count();

	return sock;
}

fsock_t *fsock_connect(EV_P_ char *addr, int port)
{
	if(main_thread.loop == NULL)
		main_thread.loop = loop;
	else {
		assert(main_thread.loop == loop);
	}

	fsock_t *sock = fsock_new();
	fsock_internal_t *_prv = sock->_prv;
	sock->type = FSOCK_CLI;

	char err[256] = "\0";
	int fd = anetTcpNonBlockConnect(err, addr, port);

	_prv->loop = loop;
	_prv->fd = fd;
	_prv->addr = zstrdup(addr);
	_prv->port = port;
	_prv->o_lock.val = 1;
	_prv->is_connected = 0;
	_prv->is_write_event_fired = 0;
	_prv->is_reading = 0;
	_prv->is_writing = 0;

	// initialize write event
	ev_io *io = &_prv->iow;
	io->data = (void *)sock;
	ev_io_init(io, io_write, fd, EV_WRITE);

	// add connection to reader thread's queue
	queue_push_right(reader_thread.in_queue, (void *)sock);
	// fire reader thread
	ev_async_send(reader_thread.loop, &reader_thread.in_async);

	// increment sockets count
	fsock_incr_sock_count();
	return sock;
}

void fsock_destroy(fsock_t *sock)
{
	// add connection to reader thread's queue
	queue_push_right(reader_thread.close_queue, (void *)sock);
	// fire reader thread
	ev_async_send(reader_thread.loop, &reader_thread.close_async);
}

void fsock_send(fsock_t *sock, void *data, uint32_t size)
{
	fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;

	// encode data
	uint32_t len = size;
	size_t avail = sdsavail(_prv->o_buf);
	size_t raw_len = size + 4;
	
	light_lock(&_prv->o_lock);
	if(avail < raw_len)
	{
		_prv->o_buf = sdsMakeRoomFor(_prv->o_buf, raw_len);
	}
	_prv->o_buf = sdscatlen(_prv->o_buf, &len, 4);
	_prv->o_buf = sdscatlen(_prv->o_buf, data, len);
	light_unlock(&_prv->o_lock);

	if(__sync_bool_compare_and_swap(&_prv->is_write_event_fired, 0, 1))
	{
		queue_push_right(reader_thread.o_queue, (void *)sock);
		ev_async_send(reader_thread.loop, &reader_thread.o_async);
	}
}

/*
 * set callbacks
 */
void fsock_on_conn(fsock_t *self, fsock_conn_handler_t handler)
{
	fsock_internal_t *_prv = (fsock_internal_t *)self->_prv;
	_prv->on_conn = handler;
}

void fsock_on_frame(fsock_t *self, fsock_frame_handler_t handler)
{
	fsock_internal_t *_prv = (fsock_internal_t *)self->_prv;
	_prv->on_frame = handler;
}

void fsock_on_disconnect(fsock_t *self, fsock_conn_handler_t handler)
{
	fsock_internal_t *_prv = (fsock_internal_t *)self->_prv;
	_prv->on_disconnect = handler;
}