#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../src/fsock.h"
#include "../src/utils/stopwatch.h"

void *srvthr (void *arg) {
  (void)arg;
  int srv = fsock_socket ("server");
  int bnd = fsock_bind (srv, "0.0.0.0", 9652);
  struct frm_frame *fr = malloc(sizeof(struct frm_frame));
  frm_frame_init (fr);
  frm_frame_set_data (fr, "fatih", 5);
  usleep(10000);
  for (int i = 0; i < 10 * 1000000; i++) {
    fsock_send (srv, 0, fr, 0);
  }
  sleep (10);
}

void *clithr (void *arg) {
  (void)arg;
  int cli = fsock_socket ("client");
  int conn = fsock_connect (cli, "0.0.0.0", 9652);
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
  sleep (10);
}

int main(int argc, char *argv[]) {
  pthread_t thrs[2];
  pthread_create(&thrs[0], NULL, srvthr, NULL);
  pthread_create(&thrs[1], NULL, clithr, NULL);
  pthread_join (thrs[0], NULL);
  pthread_join (thrs[1], NULL);
  sleep(1000);
  return 0;
}
