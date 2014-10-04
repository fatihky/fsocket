#include <stdio.h>
#include <fsocket/fsocket.h>

void on_data(fsock_conn *c, fstream_frame *f, void *arg)
{
    fsock_send(c, "pong", 4);
}

void on_disconnect(fsock_conn *c, void *arg)
{
    printf("client disconnected\n");
}

void on_conn(fsock_conn *c, void *arg)
{
    printf("new client connected on %s:%d\n", c->ip, c->port);

    fsock_conn_on_data(c, on_data, NULL);
    fsock_conn_on_disconnect(c, on_disconnect, NULL);
}

int main()
{
    EV_P = ev_loop_new(0);

    fsock_srv *srv = fsock_srv_new(EV_A_ "127.0.0.1", 9123);
    fsock_srv_on_conn(srv, on_conn, NULL);

    printf("server started on %s:%d (fd: %d)\n", srv->addr, srv->port, srv->fd);

    ev_run(EV_A_ 0);
    return 0;
}