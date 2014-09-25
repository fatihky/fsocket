#include "fsocket/fsocket.h"
#include "debug.h"

void on_data(fsock_conn *c, fstream_frame *f, void *arg)
{
<<<<<<< HEAD
    //log("client sent: %s", f->data);
=======
    log("client sent: %s", f->data);
>>>>>>> dda8e1a58047d3ecd69c9aca4fcd06237d8d9133
    fsock_send(c, "pong", 4);
}

void on_disconnect(fsock_conn *c, void *arg)
{
    log("client disconnected");
}

void on_conn(fsock_conn *c, void *arg)
{
    log("new client connected on %s:%d", c->ip, c->port);

    fsock_conn_on_data(c, on_data, NULL);
    fsock_conn_on_disconnect(c, on_disconnect, NULL);
}

int main()
{
    EV_P = ev_loop_new(0);

    fsock_srv *srv = fsock_srv_new(EV_A_ "127.0.0.1", 9123);
    fsock_srv_on_conn(srv, on_conn, NULL);

    log("server started on %s:%d (fd: %d)", srv->addr, srv->port, srv->fd);

    ev_run(EV_A_ 0);
    return 0;
}