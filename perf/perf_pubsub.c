#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../src/fsock.h"
#include "../src/utils/stopwatch.h"
#include "../src/utils/efd.h"

struct frm_frame *hello_frame = NULL;

struct {
  int msg_cnt;
  int cli_cnt;
  struct nn_efd *efd;
} g_data = {0, 0, NULL};

void *srvthr (void *arg) {
  (void)arg;
  struct nn_efd *efd = g_data.efd;
  int srv = fsock_socket ("server");
  int bnd = fsock_bind (srv, "0.0.0.0", 9652);
  struct fsock_event *ev;

  // wait for clients to connect and send hello frame
  for (int i = 0; i < g_data.cli_cnt; i++)
    ev = fsock_get_event(srv, 0);

  nn_efd_signal (efd);

  for (int i = 0; i < g_data.msg_cnt; i++) {
    fsock_sendc (srv, FSOCK_SND_ALL, 0, 0, hello_frame, 0, 0);
  }

  sleep (10);
}

void *clithrxx (void *arg) {
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

/*
*/

void *clithr (void *arg) {
  int cnt = g_data.msg_cnt;
  int cli = fsock_socket ("client");
  int conn = fsock_connect (cli, "0.0.0.0", 9652);
  // say hello to the server
  fsock_send (cli, 0, hello_frame, 0);
  for (int i = 0; i < cnt; i++) {
    struct fsock_event *event = fsock_get_event (cli, 0);
    if (!event) {
      i--; /*  handle timeouts */
      continue;
    }
    fsock_event_destroy (event);
  }
}

int main(int argc, char *argv[]) {
  struct nn_efd efd;
  int cnt = 10 * 1000000;
  int cli_cnt = 4;
  nn_efd_init (&efd);
  hello_frame = malloc(sizeof(struct frm_frame));
  frm_frame_init (hello_frame);
  frm_frame_set_data (hello_frame, "fatih", 5);
  g_data.efd = &efd;
  g_data.msg_cnt = cnt;
  g_data.cli_cnt = cli_cnt;

  pthread_t *cli_thrs = malloc (cli_cnt * sizeof (pthread_t));

  pthread_t thrs[1];
  pthread_create(&thrs[0], NULL, srvthr, &efd);

  for (int i = 0; i < cli_cnt; i++) {
    usleep(10000);
    pthread_create(&cli_thrs[i], NULL, clithr, NULL);
  }

  struct nn_stopwatch stopwatch;
  nn_efd_wait (&efd, -1);
  nn_efd_unsignal (&efd);
  nn_stopwatch_init (&stopwatch);

  for (int i = 0; i < cli_cnt; i++)
    pthread_join (cli_thrs[i], NULL);

  uint64_t us = nn_stopwatch_term (&stopwatch);
  printf ("us: %llu\n", (unsigned long long)us);

  pthread_join (thrs[0], NULL);
  sleep(1000);
  return 0;
}
