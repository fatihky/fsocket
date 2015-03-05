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