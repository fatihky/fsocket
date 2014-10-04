fsocket
=======

framed sockets library

It's very young project. With fsocket, you can easily communicate over tcp.

Dependencies:
libev (for installiatation in ubuntu: `sudo apt-get install libev-dev`)

For installiatation:
```sh
make
sudo make install
```

If you want to uninstall just run `sudo make uninstall`

Example server:
```c

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

```

Example client:
> compile with `gcc client.c -o client -lfsocket -lev`

```c

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

```