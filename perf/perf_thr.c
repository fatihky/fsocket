#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/fsock.h"
#include "../src/utils/stopwatch.h"

int main(int argc, char *argv[]) {
  int srv = fsock_socket ("server");
  int cli = fsock_socket ("client");
  int srv2 = fsock_socket ("server2");
  int bnd = fsock_bind (srv, "0.0.0.0", 9652);
  int conn = fsock_connect (cli, "0.0.0.0", 9652);
  //int conn2 = fsock_connect (cli, "0.0.0.0", 9652);
  printf ("srv: %d %d %d\n", srv, cli, srv2);
  printf ("srv: %d | cli: %d\n{bnd: %d} {conn: %d} {conn2: %%d}\n", fsock_rand(srv), fsock_rand(cli), bnd, conn/*, conn2*/);
  for (int i = 0; i < 3; i++) {
    struct fsock_event *event = fsock_get_event (srv, 0);
    if (!event) {
      i--;
      continue;
    }
    printf ("yeni frame geldi...\n");
  }
  // struct frm_frame *fr
  struct frm_frame *fr = malloc(sizeof(struct frm_frame));
  frm_frame_init (fr);
  frm_frame_set_data (fr, "fatih", 5);
  for (int i = 0; i < 10 * 1000000; i++) {
    fsock_send (srv, 0, fr, 0);
  }
  struct nn_stopwatch stopwatch;
  nn_stopwatch_init (&stopwatch);
  for (int i = 0; i < 10 * 1000000; i++) {
    struct fsock_event *event = fsock_get_event (cli, 0);
    if (!event) {
      i--;
      continue;
    }
    if (i == 0)
      printf ("[cli] yeni frame geldi...\n");
    if (i == 9999999)
      printf ("tüm mesajlar geldi....\n");
  }
  uint64_t ms = nn_stopwatch_term (&stopwatch);
  printf ("ms: %llu\n", (unsigned long long)ms);
  sleep(1000);
  return 0;
}