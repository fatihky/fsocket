#include "fsocket.h"
#include "debug.h"

void on_data(fsock_t *self, fsock_t *conn, fsock_frame_t *frame)
{
    fsock_send(conn, "pong", 4);
}

void on_disconnect(fsock_t *self, fsock_t *conn)
{
    FSOCK_LOG("client disconnected");
}

void on_conn(fsock_t *self, fsock_t *conn)
{
    FSOCK_LOG("new client connected");
}

int main()
{
    EV_P = ev_loop_new(0);

    fsock_t *srv = fsock_bind(EV_A_ "127.0.0.1", 9123);
    fsock_on_conn(srv, on_conn);
    fsock_on_frame(srv, on_data);
    fsock_on_disconnect(srv, on_disconnect);

    FSOCK_LOG("server started");

    ev_run(EV_A_ 0);

    printf("ev_run bitmiş\n");

    return 0;
}