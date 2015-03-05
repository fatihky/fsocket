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