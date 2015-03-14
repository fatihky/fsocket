# fsocket

### framed sockets library

It's very young project. With fsocket, you can easily communicate over tcp.

### api
fsocket has a very simple api. You can see at [fsocket.h](https://github.com/fatihky/fsocket/blob/master/include/fsocket.h)

### Dependencies:
libev (for installiatation in ubuntu: `sudo apt-get install libev-dev`)<br/>
[libhl](https://github.com/xant/libhl)<br/>
pthread<br/>

For installiatation:
```sh
make
sudo make install
```

Run `make test` to compile examples.
If you want to uninstall just run `sudo make uninstall`.

### What makes it useful?
First, it has a simple binary protocol. That is just length prefixed stream.<br/>
`[4 bytes length] [n bytes data] ...`

So you just think based on frames. All buffering jobs done in back.


#### Example server:
> compile with `gcc server.c -o server -lfsocket -lev -lhl -lpthread`

```c
#include <stdio.h>
#include <fsocket.h>

void on_conn(fsock_t *self, fsock_t *conn);
void on_disconnect(fsock_t *self, fsock_t *conn);
void on_frame(fsock_t *self, fsock_t *conn, fsock_frame_t *frame);

int main(int argc, char *argv[])
{
  EV_P = ev_loop_new(0);
  fsock_t *server = fsock_bind(EV_A_ "127.0.0.1", 9123);
  fsock_on_conn(server, on_conn);
  fsock_on_disconnect(server, on_disconnect);
  fsock_on_frame(server, on_frame);

  ev_run(EV_A_ 0);
  return 0;
}

void on_conn(fsock_t *self, fsock_t *conn)
{
  printf("new conn\n");
}

void on_disconnect(fsock_t *self, fsock_t *conn)
{
	printf("user disconnected\n");
}

void on_frame(fsock_t *self, fsock_t *conn, fsock_frame_t *frame)
{
  fsock_send(conn, (void *)"pong", 4);
}
```

#### Example client:
> compile with `gcc client.c -o client -lfsocket -lev -lhl -lpthread`

```c
#include <stdio.h>
#include <fsocket.h>

void on_conn(fsock_t *self, fsock_t *conn);
void on_disconnect(fsock_t *self, fsock_t *conn);
void on_frame(fsock_t *self, fsock_t *conn, fsock_frame_t *frame);

int main(int argc, char *argv[])
{
  EV_P = ev_loop_new(0);
  fsock_t *client = fsock_connect(EV_A_ "127.0.0.1", 9123);
  fsock_on_conn(client, on_conn);
  fsock_on_disconnect(client, on_disconnect);
  fsock_on_frame(client, on_frame);

  fsock_send(client, (void *)"ping", 4);

  ev_run(EV_A_ 0);
  return 0;
}

void on_conn(fsock_t *self, fsock_t *conn)
{
  printf("connected to the server\n");
}

void on_disconnect(fsock_t *self, fsock_t *conn)
{
  printf("server connection closed\n");
}

void on_frame(fsock_t *self, fsock_t *conn, fsock_frame_t *frame)
{
  printf("new frame: %s\n", (char *)frame->data);
}
```

### Bindings
* node.js:
    * [fsocket.js](https://github.com/fatihky/fsocket.js)

### Licence: `no`
![](https://cloud.githubusercontent.com/assets/4169772/4864685/466ebb02-611e-11e4-829c-8dbca79aa7ec.jpg)
