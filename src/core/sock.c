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
  self->idxlocal = -1;
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
  if (self->want_efd == 1) {
    nn_efd_signal (&self->efd);
    self->want_efd = 0;
  }
  fsock_queue_push (&self->events, &event->item);
  fsock_mutex_unlock (&self->sync);
  return 0;
}

void fsock_sock_accept_handler (EV_P_ ev_io *a, int revents) {
  printf ("accept new connection\n");
  char err[255];
  char ip[255];
  int port = 0;
  int fd = anetTcpAccept(err, a->fd, ip, 255, &port);
  if (fd < 0)
    return;

  /*
    Sadece bu iş parçacığı bu sokete yeni bağlantı ekleyip bağlantıları
    çıkarabileceği için için yeni bağlantıları sokete eklerken mutex
    kullanmıyorum. İlerde kullanırım. Çalışsın bir.
  */
  struct fsock_sock *sock = frm_cont (a, struct fsock_sock, rio);
  struct fsock_sock *root = sock->owner;
  struct fsock_sock *conn = malloc (sizeof (struct fsock_sock));
  struct fsock_thread *thr;
  int index;
  int indexlocal;

  if (conn == NULL)
    return;

  /*  we check socket's owner's type here because this socket is a bind socket
      that attached to*/
  assert (sock->owner->type == FSOCK_SOCK_BASE);
  fsock_sock_init (conn, FSOCK_SOCK_IN);
  conn->owner = sock->owner;
  fsock_mutex_lock (&sock->sync);
  index = fsock_parr_insert (&sock->conns, conn);
  indexlocal = index;
  fsock_mutex_unlock (&sock->sync);
  if (index < 0) {
    free (conn);
    return;
  }
  fsock_mutex_lock(&root->sync);
  index = fsock_parr_insert (&root->conns, conn);
  fsock_mutex_unlock(&root->sync);
  if (index < 0) {
    printf ("can not add conection to the root  socket's connections array.");
    return;
  }
  fsock_glock_lock();
  thr = f_choose_thr();
  fsock_glock_unlock();
  conn->thr = thr;
  conn->fd = fd;
  conn->idx = index;
  conn->idxlocal = indexlocal;
  fsock_thread_start_connection (thr, conn);
  // notify socket about this event or do not like zeromq
  printf ("notify socket about this event or do not like zeromq {socket: %d|%d}\n", sock->idx, sock->uniq);
}

static void fsock_sock_read_handler (EV_P_ ev_io *r, int revents) {
  struct fsock_sock *conn = frm_cont (r, struct fsock_sock, rio);
  assert (conn->fd == r->fd);

  struct frm_cbuf *cbuf = frm_cbuf_new (1400);

  if (cbuf == NULL)
    return;

  ssize_t nread = read (r->fd, cbuf->buf, 1400);

  if (nread == 0)
    goto stop;

  if (nread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    goto clean;

  int rc = frm_parser_parse (&conn->parser, cbuf, nread);

  if (rc != 0) {
    printf ("parse error. rc: %d\n", rc);
    goto stop;
  }

  while (!frm_list_empty (&conn->parser.in_frames)) {
    struct frm_list_item *li = frm_list_begin (&conn->parser.in_frames);

    if (!li)
      break;

    // remove frame from the list
    frm_list_erase (&conn->parser.in_frames, li);

    struct frm_frame *fr = frm_cont (li, struct frm_frame, item);

    // frm_frame_destroy (fr);
    fsock_sock_queue_event (conn->owner, FSOCK_EVENT_NEW_FRAME, fr, -1);
  }

  goto clean;

stop:
  printf ("bağlantı koptu.\n");
  ev_io_stop (EV_A_ r);
clean:
  frm_cbuf_unref (cbuf);
}

static void fsock_sock_write_handler (EV_P_ ev_io *w, int revents) {
  struct fsock_sock *conn = frm_cont (w, struct fsock_sock, wio);
  assert (conn->fd == w->fd);

  if (frm_list_empty (&conn->ol.list)) {
    printf ("All frames are written.\n");
    goto stop;
  }

  struct iovec iovs[512];
  int retiovcnt = -1;
  ssize_t tow = frm_out_frame_list_get_iovs (&conn->ol, iovs, 512, &retiovcnt);
  ssize_t nw = writev (w->fd, iovs, retiovcnt);

  if (nw < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return;

  if (nw <= 0) {
    printf ("nw: %zd\n", nw);
    goto stop;
  }

  frm_out_frame_list_written (&conn->ol, nw);
  return;

stop:
  ev_io_stop (EV_A_ w);
  fsock_mutex_lock (&conn->sync);
  conn->writing = 0;
  fsock_mutex_unlock (&conn->sync);
}
