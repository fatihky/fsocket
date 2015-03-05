#include "fsocket.h"
#include "fs_internal.h"
#include "io_handlers.h"

void io_accept(EV_P_ struct ev_io *a, int revents)
{
	(void)revents;
	fsock_t *self = (fsock_t *)a->data;
	fsock_internal_t *srv_prv = (fsock_internal_t *)self->_prv;
	int port;
	char ip[256];
	int fd = anetTcpAccept(NULL, a->fd, ip, 256, &port);
	anetNonBlock(NULL, fd);
	anetEnableTcpNoDelay(NULL, fd);

	fsock_t *conn = fsock_new();
	fsock_internal_t *_prv = conn->_prv;
	conn->type = FSOCK_CONN;
	_prv->loop = loop; // main loop
	_prv->fd = fd;
	_prv->addr = zstrdup(ip);
	_prv->port = port;
	_prv->srv = self;
	_prv->is_connected = 1;
	_prv->is_write_event_fired = 0;
	_prv->o_lock.val = 1;
	_prv->is_reading = 0;
	_prv->is_writing = 0;

	if(srv_prv->on_conn)
		srv_prv->on_conn(self, conn);

	// initialize write event
	ev_io *io = &_prv->iow;
	io->data = (void *)conn;
	ev_io_init(io, io_write, fd, EV_WRITE);
	io__ = io;

	// increment sockets count
	fsock_incr_sock_count();

	// add connection to reader thread's queue
	queue_push_right(reader_thread.in_queue, (void *)conn);
	// fire reader thread
	ev_async_send(reader_thread.loop, &reader_thread.in_async);
}

void io_read(EV_P_ struct ev_io *r, int revents)
{
	(void)loop;
	(void)revents;
	fsock_t *sock = (fsock_t *)r->data;
	fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;

	if(sock->type == FSOCK_CLI)
	{
		if(_prv->is_connected == 0)
		{
			if(_prv->on_conn)
				_prv->on_conn(sock, NULL);
			_prv->is_connected = 1;
		}
	}

	uint32_t sz = FSOCK_TCP_INBUF;
	int bytes = 0;

	if(sz < _prv->parser.must_read)
		sz = _prv->parser.must_read;

	_prv->in_buf = sdsMakeRoomFor(_prv->in_buf, sz);

	char *start = _prv->in_buf + sdslen(_prv->in_buf);
	bytes = read(_prv->fd, start, sz);

	if(bytes == 0) {
		// connection closed
		if(__sync_bool_compare_and_swap(&_prv->is_connected, 1, 0))
		{
			fsock_destroy(sock);
		}
		return;
	} else if(bytes == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// no data received. wait for new data
			return;
		} else {
			// connection closed
			if(__sync_bool_compare_and_swap(&_prv->is_connected, 1, 0))
			{
				// add connection to reader thread's queue
				queue_push_right(main_thread.disconnect_queue, (void *)sock);
				// fire reader thread
				ev_async_send(main_thread.loop, &main_thread.disconnect_async);
			}
			return;
		}
	}

	// parse incoming data
	if(_prv->parser.must_read > 0 && (uint32_t)bytes <= _prv->parser.must_read)
		_prv->parser.must_read -= bytes;

	sdsIncrLen(_prv->in_buf, bytes);

	// parse incoming data
	size_t not_parsed = sdslen(_prv->in_buf);

	if(not_parsed < _prv->parser.must_read)
		return; // wait for more data

	sds p = _prv->in_buf;

	for(;;)
	{
		fsock_frame_t *f = &_prv->parser.curr;
		switch(_prv->parser.state)
		{
			case FSOCK_PROT_START:
			{
				if(not_parsed < 4) // wait for more data
				{
					return; // FSTREAM_OK;
				}
				memcpy(&f->size, p, 4);
				p += 4; // move pointer for 4 bytes
				not_parsed -= 4;
				_prv->parser.state = FSOCK_PROT_END;
			}
			case FSOCK_PROT_END:
			{
				if(not_parsed < f->size)
				{
					_prv->parser.must_read = f->size - not_parsed;
					sdsrange(_prv->in_buf, sdslen(_prv->in_buf) - not_parsed, -1);
					return;
				}
				f->data = p;

				// send this frame to main thread
				fsock_frame_t *copy = zmalloc(sizeof(fsock_frame_t));
				copy->data = f->data;
				copy->size = f->size;

				fsock_frame_queue_item_t *q_item = zmalloc(sizeof(fsock_frame_queue_item_t));
				q_item->frame = copy;
				if(sock->type == FSOCK_CONN)
				{
					q_item->receiver = _prv->srv;
					q_item->sender = sock;
				} else {
					q_item->receiver = sock;
					q_item->sender = NULL;
				}

				queue_push_right(main_thread.frame_queue, (void *)q_item);

				ev_async_send(main_thread.loop, &main_thread.frame_async);

				p += f->size;
				not_parsed -= f->size;
				_prv->parser.state = FSOCK_PROT_START; // return to start

				_prv->parser.must_read = 0;
				sdsrange(_prv->in_buf, sdslen(_prv->in_buf) - not_parsed, -1);

				p = _prv->in_buf;
			}
			default: break;
		}

		if(not_parsed == 0 || _prv->parser.must_read)
			break;
	}

}

void io_write(EV_P_ struct ev_io *w, int revents)
{
	(void)revents;
	fsock_t *sock = (fsock_t *)w->data;
	fsock_internal_t *_prv = (fsock_internal_t *)sock->_prv;
	light_lock(&_prv->o_lock);
	size_t o_len = sdslen(_prv->o_buf);
	if(o_len == 0)
	{
		if(__sync_bool_compare_and_swap(&_prv->is_writing, 1, 0))
		{
			ev_io_stop(EV_A_ w);
		}
		return;
	}

	int bytes, len = o_len;
	if(len > FSOCK_TCP_OBUF)
		len = FSOCK_TCP_OBUF;

	bytes = write(_prv->fd, (char *)_prv->o_buf, len);

	if (bytes < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
			(errno == EINTR)) {
			/* do nothing, try again. */
		} else {
			queue_push_right(main_thread.disconnect_queue, (void *)sock);
			// fire reader thread
			ev_async_send(main_thread.loop, &main_thread.disconnect_async);
		}
	} else if(bytes > 0) {
		if(bytes == (int)sdslen(_prv->o_buf))
			sdsclear(_prv->o_buf);
		else
			sdsrange(_prv->o_buf, 0, bytes);

		if(sdslen(_prv->o_buf) == 0) {
			if(__sync_bool_compare_and_swap(&_prv->is_writing, 1, 0))
			{
				ev_io_stop(EV_A_ w);
			}
		} else {
			// do nothing
		}
	} else if(bytes == 0) {
		// return;
	}
	light_unlock(&_prv->o_lock);
	__sync_bool_compare_and_swap(&_prv->is_write_event_fired, 1, 0);
}