#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <string.h>     /* String handling */
#include <errno.h>
#include "fsocket.h"
#include "anet.h"
<<<<<<< HEAD
<<<<<<< HEAD
#include "../debug.h"
=======

#define FSOCK_TCP_INBUF (16 * 1024)
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======

#define FSOCK_TCP_INBUF (16 * 1024)
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133

static fsock_conn *fsock_conn_new(fsock_srv *s, int fd, char *ip, int port);

static void _srv_frame_handler(fstream_frame *f, void *arg)
{
    fsock_conn *c = (fsock_conn *)arg;
    if(c->on_data)
        c->on_data(c, f, c->on_data_arg);
}

static void _cli_frame_handler(fstream_frame *f, void *arg)
{
    fsock_cli *c = (fsock_cli *)arg;
    if(c->on_data)
        c->on_data(c, f, c->on_data_arg);
}

static void _srv_read_cb(EV_P_ struct ev_io *r, int revents)
{
    fsock_conn *c = (fsock_conn *)r->data;

    int sz = FSOCK_TCP_INBUF;
	int bytes = 0;

<<<<<<< HEAD
<<<<<<< HEAD
    if(sz < fstream_must_read(c->stream))
        sz = fstream_must_read(c->stream);

=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
	char *p = fstream_prepare_to_read(c->stream, sz);

	bytes = read(c->fd, p, sz);

    fstream_data_readed(c->stream, bytes);

    if (bytes == 0)
    {
        // connection closed
        fsock_conn_destroy(c);
        return;
    } else if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no data
            // continue to wait for recv
<<<<<<< HEAD
<<<<<<< HEAD
            //log("no data, continue to wait for recv");
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
            return;
        } else {
            // error at server recv
            fsock_conn_destroy(c);
            return;
        }
    }

<<<<<<< HEAD
<<<<<<< HEAD
    //log("%d bytes received", bytes);
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
    fstream_decode_input(c->stream, _srv_frame_handler, c);

	_fsock_add_write(c);
}

static void _srv_write_cb(EV_P_ struct ev_io *w, int revents)
{
    fsock_conn *c = (fsock_conn *)w->data;

	if(fstream_output_size(c->stream) == 0)
	{
		// if all data transfered to client, stop write event
		_fsock_del_write(c);
		return;
	}

	int bytes, len;

    char *p = fstream_prepare_to_write(c->stream, &len);
    if(p == NULL)
    {
        _fsock_del_write(c);
        return;
    }

    bytes = write(c->fd, p, len);

	if (bytes < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
			(errno == EINTR)) {
			/* do nothing, try again. */
<<<<<<< HEAD
<<<<<<< HEAD
			//log("do nothing, try again");
		} else {
			// TODO: what I must do in here?
			fsock_conn_destroy(c);
			return;
		}
	} else if(bytes > 0) {
	    //log("%d bytes written", bytes);
	    fstream_data_written(c->stream, bytes);
		if (fstream_output_size(c->stream) == 0) {
=======
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
		} else {
			// TODO: what I must do in here?
			return;
		}
	} else if(bytes > 0) {
	    fstream_data_written(c->stream, bytes);
		if (bytes == fstream_output_size(c->stream)) {
<<<<<<< HEAD
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
			_fsock_del_write(c); // and, stop write event of course
		} else {
		}
	}
}


static int __cli_handle_connect(fsock_cli *c)
{
    c->flags |= FSOCK_CONNECTED; // mark as connected
    if(c->on_connect)
        c->on_connect(c, c->on_connect_arg);
    return FSOCK_OK;
}

void _cli_read_cb(EV_P_ struct ev_io *r, int revents)
{
<<<<<<< HEAD
<<<<<<< HEAD
=======
    // log("triggered");
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
    // log("triggered");
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
    fsock_cli *c = (fsock_cli *)r->data;

    // I copied this from hiredis
    if (!(c->flags & FSOCK_CONNECTED))
    {
        /* Abort connect was not successful. */
        if (__cli_handle_connect(c) != FSOCK_OK)
            return;
        /* Try again later when the context is still not connected. */
        if (!(c->flags & FSOCK_CONNECTED))
            return;
    }

    int sz = FSOCK_TCP_INBUF;
	int bytes = 0;

<<<<<<< HEAD
<<<<<<< HEAD
    if(sz < fstream_must_read(c->stream))
        sz = fstream_must_read(c->stream);

=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
	char *p = fstream_prepare_to_read(c->stream, sz);

	bytes = read(c->fd, p, sz);

    fstream_data_readed(c->stream, bytes);

    if (bytes == 0)
    {
        // connection closed
        fsock_cli_destroy(c);
        return;
    } else if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no data
            // continue to wait for recv
            return;
        } else {
            // error at server recv
            fsock_cli_destroy(c);
            return;
        }
    }

    fstream_decode_input(c->stream, _cli_frame_handler, c);

	_fsock_add_write(c);
}

void _cli_write_cb(EV_P_ struct ev_io *w, int revents)
{
    fsock_cli *c = (fsock_cli *)w->data;

	if(fstream_output_size(c->stream) == 0)
	{
		// if all data transfered to client, stop write event
		_fsock_del_write(c);
		return;
	}

	int bytes, len;

    char *p = fstream_prepare_to_write(c->stream, &len);
    if(p == NULL)
    {
        _fsock_del_write(c);
        return;
    }

    bytes = write(c->fd, p, len);

	if (bytes < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
			(errno == EINTR)) {
			/* do nothing, try again. */
		} else {
			// TODO: what I must do in here?
<<<<<<< HEAD
<<<<<<< HEAD
			fsock_cli_destroy(c);
			return;
		}
	} else if(bytes > 0) {
	    //log("%d bytes written", bytes);
	    fstream_data_written(c->stream, bytes);
		if (fstream_output_size(c->stream) == 0) {
		    //log("all data is written");
			_fsock_del_write(c); // and, stop write event of course
		} else {
		    //log("bytes: %d otunput size: %zu", bytes, fstream_output_size(c->stream)); 
            // maybe resize input buffer
		}
	} else if(bytes == 0) {
	    //log("bytes == 0");
=======
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
			return;
		}
	} else if(bytes > 0) {
	    fstream_data_written(c->stream, bytes);
		if (bytes == fstream_output_size(c->stream)) {
			_fsock_del_write(c); // and, stop write event of course
		} else {
            // maybe resize input buffer
		}
<<<<<<< HEAD
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
=======
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
	}
}

static void _srv_accept_cb(EV_P_ struct ev_io *a, int revents)
{
    fsock_srv *s = (fsock_srv *)a->data;
	int port;
	char ip[256];
	int fd = anetTcpAccept(NULL, a->fd, ip, 256, &port);
	anetNonBlock(NULL, fd);
	anetEnableTcpNoDelay(NULL, fd);

    fsock_conn *c = fsock_conn_new(s, fd, ip, port);

    if(s->on_conn)
        s->on_conn(c, s->on_conn_arg);

    // register event handler to server loop
	_fsock_add_read(c);
}

/*
 * Client
 */

void _cli_async_cb (struct ev_loop *loop, struct ev_io *a, int revents)
{
    printf("%s:%d: triggered", __func__, __LINE__);
}

fsock_cli *fsock_cli_new(EV_P_ char *host, int port)
{
    char err[256] = "\0";
    int fd = anetTcpNonBlockConnect(err, host, port);
    fsock_cli *c = zmalloc(sizeof(fsock_cli));
    c->loop = EV_A;
    c->flags = 0;
    c->fd = fd;
    c->host = zstrdup(host);
    c->port = port;
    ev_io *io = &c->ior; 
    ev_io_init(io, _cli_read_cb, fd, EV_READ);
    io = &c->iow;
    ev_io_init(io, _cli_write_cb, fd, EV_WRITE);
    c->ior.data = c;
    c->iow.data = c;
    c->is_reading = 0;
    c->is_writing = 0;
    c->stream = fstream_new();
    c->on_connect = NULL;
    c->on_data = NULL;
    c->on_disconnect = NULL;
    c->on_connect_arg = NULL;
    c->on_data_arg = NULL;
    c->on_disconnect_arg = NULL;
    ev_async *async = &c->async;
    ev_async_init(async, _cli_async_cb);
    ev_async_start(EV_A_ async);
    _fsock_add_read(c);
    return c;
}

void fsock_cli_destroy(fsock_cli *c)
{
    if(c->on_disconnect)
        c->on_disconnect(c, c->on_disconnect_arg);

    _fsock_del_read(c);
    _fsock_del_write(c);
    ev_async *async = &c->async;
    ev_async_stop(c->loop, async);
    
    close(c->fd);

    fstream_free(c->stream);
    zfree(c->host);
    zfree(c);
}

void fsock_cli_on_connect(fsock_cli *c, void (*on_connect)(fsock_cli *c, void *data), void *arg)
{
    c->on_connect = on_connect;
    c->on_connect_arg = arg;
}

void fsock_cli_on_data(fsock_cli *c, void (*on_data)(fsock_cli *c, fstream_frame *f, void *arg), void *arg)
{
    c->on_data = on_data;
    c->on_data_arg = arg;
}

void fsock_cli_on_disconnect(fsock_cli *c, void (*on_disconnect)(fsock_cli *c, void *arg), void *arg)
{
    c->on_disconnect = on_disconnect;
    c->on_disconnect_arg = arg;
}

/*
 * Connection
 */

fsock_conn *fsock_conn_new(fsock_srv *s, int fd, char *ip, int port)
{
    fsock_conn *c = zmalloc(sizeof(fsock_conn));
    c->loop = s->loop;
    c->srv = s;
    c->fd = fd;
    c->ip = zstrdup(ip);
    c->port = port;
    c->data = NULL;
    ev_io *io = &c->ior;
    ev_io_init(io, _srv_read_cb, fd, EV_READ);
    io = &c->iow;
    ev_io_init(io, _srv_write_cb, fd, EV_WRITE);
    c->ior.data = c;
    c->iow.data = c;
    c->is_reading = 0;
    c->is_writing = 0;
    c->stream = fstream_new();

    c->on_data = NULL;
    c->on_disconnect = NULL;
    c->on_data_arg = NULL;
    c->on_disconnect_arg = NULL;
    return c;
}

void fsock_conn_destroy(fsock_conn *c)
{
    if(c->on_disconnect)
        c->on_disconnect(c, c->on_disconnect_arg);

    _fsock_del_read(c);
    _fsock_del_write(c);
    
    close(c->fd);

    fstream_free(c->stream);
    zfree(c->ip);
    zfree(c);
}

void fsock_conn_on_data(fsock_conn *c, void (*on_data)(fsock_conn *c, fstream_frame *f, void *arg), void *arg)
{
    c->on_data = on_data;
    c->on_data_arg = arg;
}

void fsock_conn_on_disconnect(fsock_conn *c, void (*on_disconnect)(fsock_conn *c, void *arg), void *arg)
{
    c->on_disconnect = on_disconnect;
    c->on_disconnect_arg = arg;
}

fsock_srv *fsock_srv_new(EV_P_ char *addr, int port)
{
    fsock_srv *s = zmalloc(sizeof(fsock_srv));

    char err[256] = "\0";
    int fd = anetTcpServer(err, 9123, "127.0.0.1", 1024);

    s->loop = EV_A;
    s->fd = fd;
    s->addr = zstrdup(addr);
    s->port = port;
    s->on_conn = NULL;

    s->on_conn_arg = NULL;

    ev_io *io = &s->ioa;
    io->data = s;

	ev_io_init(io, _srv_accept_cb, fd, EV_READ);
	ev_io_start(EV_A_ io);

	return s;
}

void fsock_srv_on_conn(fsock_srv *s, void (*on_conn)(fsock_conn *conn, void *arg), void *arg)
{
    s->on_conn = on_conn;
    s->on_conn_arg = arg;
}