#include <framer/framer.h>
#include "../utils/thread.h"
#include "../utils/efd.h"

#pragma once

#define FSOCK_SOCK_BASE 1
#define FSOCK_SOCK_BIND 2
#define FSOCK_SOCK_IN   3 /*  incoming connection */
#define FSOCK_SOCK_OUT  4 /*  outgoing connection */

#define FSOCK_START_READ 1
#define FSOCK_STOP_READ 2
#define FSOCK_START_WRITE 3
#define FSOCK_STOP_WRITE 4
#define FSOCK_CLOSE 5

struct fsock_sock {
  int type;
  int fd;
  int uniq;
  ev_io rio;
  ev_io wio;
  int writing;
  int reading;
  int want_efd;
  struct nn_efd efd;
  struct fsock_queue events;
  struct fsock_sock *owner;
  struct fsock_thread *thr;
  struct fsock_parr conns;
  struct fsock_parr binds;
  struct fsock_mutex sync;
  struct fsock_task t_start_read;
  struct fsock_task t_stop_read;
  struct fsock_task t_start_write;
  struct fsock_task t_stop_write;
  struct fsock_task t_close;
  struct frm_out_frame_list ol;
  struct frm_parser parser;
  /* debugging */
  int idx; // socket-wide index
  int idxlocal; // sub-socket(bind)-wide index
};

void fsock_sock_init (struct fsock_sock *self, int type);
int fsock_sock_queue_event (struct fsock_sock *self, int type,
  struct frm_frame *fr, int conn);

/*  i/o handlers */
void fsock_sock_accept_handler (EV_P_ ev_io *a, int revents);
void fsock_sock_read_handler (EV_P_ ev_io *r, int revents);
void fsock_sock_write_handler (EV_P_ ev_io *w, int revents);
