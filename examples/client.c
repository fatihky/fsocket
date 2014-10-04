#include <stdio.h>
#include <fsocket/fsocket.h>

void on_connect(fsock_cli *c, void *arg)
{
    printf("client connected to %s:%d (fd: %d)\n", c->host, c->port, c->fd);
}

void on_data(fsock_cli *c, fstream_frame *f, void *arg)
{
    // make some things with data
    printf("received: %s\n", f->data);
}

void on_disconnect(fsock_cli *c, void *arg)
{
    printf("server connection closed\n");
}

int main(int argc, char *argv[])
{
    EV_P = ev_loop_new(0);

    fsock_cli *cli = fsock_cli_new(EV_A_ "127.0.0.1", 9123);
    fsock_cli_on_connect(cli, on_connect, NULL);
    fsock_cli_on_data(cli, on_data, NULL);
    fsock_cli_on_disconnect(cli, on_disconnect, NULL);

    fsock_send(cli, "ping", 4);

    ev_run(EV_A_ 0);
    return 0;
}