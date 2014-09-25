#ifndef __FSOCKET_H
#define __FSOCKET_H

#include <stddef.h>
#include <ev.h>

#include "fstream.h"
#include "zmalloc.h"

/*
    Async connect/disconnect functionality related functions, constants are copied from hiredis.
    Please look at (https://github.com/antirez/redis/blob/unstable/deps/hiredis/async.c) for licence information
*/

// i/o utilities
#define _fsock_add_read(c) if(!c->is_reading) { ev_io_start(c->loop, &c->ior); c->is_reading = 1; }
#define _fsock_add_write(c) if(!c->is_writing) { ev_io_start(c->loop, &c->iow); c->is_writing = 1; }
#define _fsock_del_read(c) if(c->is_reading) { ev_io_stop(c->loop, &c->ior); c->is_reading = 0; }
#define _fsock_del_write(c) if(c->is_writing) { ev_io_stop(c->loop, &c->iow); c->is_writing = 0; }

#define FSOCK_OK 0
#define FSOCK_ERR -1

#define FSOCK_CONNECTED 0x2

typedef struct fsock_srv fsock_srv;
typedef struct fsock_cli fsock_cli;
typedef struct rmy_conn fsock_conn;

struct rmy_conn {
    EV_P;
    int fd;
    ev_io ior;
    ev_io iow;
    unsigned int is_reading:1;
    unsigned int is_writing:1;
    fstream *stream;

    fsock_srv *srv;
    char *ip;
    int port;

    void *data;
    void (*on_data)(fsock_conn *c, fstream_frame *f, void *arg);
    void (*on_disconnect)(fsock_conn *c, void *arg);
    void *on_data_arg;
    void *on_disconnect_arg;
};

struct fsock_cli {
    fsock_conn *conn;
    EV_P;
    int fd;
    int flags;
    char *host;
    int port;
    ev_io ior;
    ev_io iow;
    ev_async async;
    unsigned int is_reading:1;
    unsigned int is_writing:1;
    fstream *stream;

    void (*on_connect)(fsock_cli *c, void *arg);
    void (*on_data)(fsock_cli *c, fstream_frame *f, void *arg);
    void (*on_disconnect)(fsock_cli *c, void *arg);
    void *on_connect_arg;
    void *on_data_arg;
    void *on_disconnect_arg;
};

struct fsock_srv {
    EV_P;
    int fd;
    char *addr;
    int port;
    ev_io ioa;

    void (*on_conn)(fsock_conn *conn, void *arg);
    void *on_conn_arg;
};

#define fsock_send(sock, data, len) \
    fstream_encode_data(sock->stream, data, len); \
    _fsock_add_write(sock)

fsock_srv *fsock_srv_new(EV_P_ char *addr, int port);
void fsock_srv_on_conn(fsock_srv *s, void (*on_conn)(fsock_conn *conn, void *arg), void *arg);

void fsock_conn_destroy(fsock_conn *c);
void fsock_conn_on_data(fsock_conn *c, void (*on_data)(fsock_conn *c, fstream_frame *f, void *arg), void *arg);
void fsock_conn_on_disconnect(fsock_conn *c, void (*on_disconnect)(fsock_conn *c, void *arg), void *arg);

fsock_cli *fsock_cli_new(EV_P_ char *host, int port);
void fsock_cli_destroy(fsock_cli *c);
void fsock_cli_on_connect(fsock_cli *c, void (*fn)(fsock_cli *c, void *data), void *arg);
void fsock_cli_on_data(fsock_cli *c, void (*on_data)(fsock_cli *c, fstream_frame *f, void *arg), void *arg);
void fsock_cli_on_disconnect(fsock_cli *c, void (*on_disconnect)(fsock_cli *c, void *arg), void *arg);

#endif