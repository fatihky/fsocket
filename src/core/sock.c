#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <framer/cont.h>
#include "../utils/queue.h"
#include "sock.h"
#include "../fsock.h"

static void task_routine (struct fsock_thread *thr, struct fsock_task *task) {
  struct fsock_sock *sock;
  switch (task->type) {
    case FSOCK_START_READ: {
      printf ("FSOCK_START_READ\n");
    } break;
    case FSOCK_STOP_READ: {
      printf ("FSOCK_STOP_READ\n");
    } break;
    case FSOCK_START_WRITE: {
      printf ("start writing.\n");
      sock = frm_cont (task, struct fsock_sock, t_start_write);
      ev_io_start (thr->loop, &sock->wio);
    } break;
    case FSOCK_STOP_WRITE: {
      printf ("FSOCK_STOP_WRITE\n");
    } break;
    case FSOCK_CLOSE: {
      printf ("FSOCK_CLOSE\n");
    } break;
    default:
      printf ("unknown task type passed: %d\n", task->type);
      assert (0);
  }
}

void fsock_sock_init (struct fsock_sock *self, int type) {
  self->idx = -1; /*  used for debugging */
  self->type = type;
  self->fd = -1;
  self->owner = NULL;
  self->thr = NULL;
  self->want_efd = 0;
  self->uniq = rand();
  self->reading = 0;
  self->writing = 0;
  nn_efd_init (&self->efd);
  frm_parser_init (&self->parser);
  frm_out_frame_list_init (&self->ol);
  fsock_parr_init (&self->conns, 10);
  fsock_parr_init (&self->binds, 10);
  fsock_queue_init (&self->events);
  fsock_mutex_init (&self->sync);
  fsock_task_init (&self->t_start_read, FSOCK_START_READ, task_routine, NULL);
  fsock_task_init (&self->t_stop_read, FSOCK_STOP_READ, task_routine, NULL);
  fsock_task_init (&self->t_start_write, FSOCK_START_WRITE, task_routine, NULL);
  fsock_task_init (&self->t_stop_write, FSOCK_STOP_WRITE, task_routine, NULL);
  fsock_task_init (&self->t_close, FSOCK_CLOSE, task_routine, NULL);
}

int fsock_sock_queue_event (struct fsock_sock *self, int type,
    struct frm_frame *fr, int conn) {
  struct fsock_event *event = malloc (sizeof (struct fsock_event));
  if (!event)
    return ENOMEM;
  assert (self->type == FSOCK_SOCK_BASE);
  event->type = type;
  switch (type) {
    case FSOCK_EVENT_NEW_CONN:
      event->conn = conn; break;
    case FSOCK_EVENT_NEW_FRAME:
      event->frame = fr; break;
    default:
      printf ("[fsock_sock_queue_event] unknown event type: %d\n", type);
      free (event);
      assert (0);
      return -1;
  }
  fsock_queue_item_init (&event->item);
  fsock_mutex_lock (&self->sync);
  //printf ("new event added to sock: %d|%d [fsock_queue_empty: %d]\n", self->idx, self->uniq, fsock_queue_empty (&self->events));
  if (self->want_efd == 1) {
    nn_efd_signal (&self->efd);
    self->want_efd = 0;
    //printf ("efd'ti tetikledim.\n");
  } //else
    //printf ("kimse efd'yi beklemiyor.\n");
  fsock_queue_push (&self->events, &event->item);
  fsock_mutex_unlock (&self->sync);
  return 0;
}
