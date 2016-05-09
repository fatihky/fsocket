#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <framer/cont.h>
#include "../core/global.h"
#include "../utils/queue.h"
#include "../utils/anet.h"
#include "../utils/glock.h"
#include "sock.h"
#include "../fsock.h"

#define FSOCK_SOCK_HWM_DEFAULT 1000
#define FSOCK_SOCK_HWM_FULL -1

static void task_routine (struct fsock_thread *thr, struct fsock_task *task) {
  struct fsock_sock *sock;
  switch (task->type) {
    case FSOCK_START_READ: {
      //printf ("FSOCK_START_READ\n");
      sock = frm_cont (task, struct fsock_sock, t_start_read);
      sock->reading = 1;
      ev_io_start (thr->loop, &sock->rio);
    } break;
    case FSOCK_STOP_READ: {
      //printf ("FSOCK_STOP_READ\n");
      sock = frm_cont (task, struct fsock_sock, t_stop_read);
      ev_io_stop (thr->loop, &sock->rio);
    } break;
    case FSOCK_START_WRITE: {
      // printf ("FSOCK_START_WRITE\n");
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
  self->flags = 0;
  self->fd = -1;
  self->owner = NULL;
  self->thr = NULL;
  self->want_efd = 0;
  self->uniq = rand();
  self->reading = 0;
  self->writing = 0;
  self->sndhwm = FSOCK_SOCK_HWM_DEFAULT;
  self->rcvhwm = FSOCK_SOCK_HWM_DEFAULT;
  self->sndqsz = 0;
  self->rcvqsz = 0;
  self->want_wcond = 0;
  pthread_cond_init(&self->wcond, NULL);
  nn_efd_init (&self->efd);
  frm_parser_init (&self->parser);
  frm_out_frame_list_init (&self->ol);
  fsock_parr_init (&self->binds, 10);
  fsock_parr_init (&self->conns, 10);
  fsock_queue_init (&self->events);
  fsock_mutex_init (&self->sync);
  fsock_task_init (&self->t_start_read, FSOCK_START_READ, task_routine, NULL);
  fsock_task_init (&self->t_stop_read, FSOCK_STOP_READ, task_routine, NULL);
  fsock_task_init (&self->t_start_write, FSOCK_START_WRITE, task_routine, NULL);
  fsock_task_init (&self->t_stop_write, FSOCK_STOP_WRITE, task_routine, NULL);
  fsock_task_init (&self->t_close, FSOCK_CLOSE, task_routine, NULL);
}


static void fsock_sock_conn_parr_term (struct fsock_sock *self,
    struct fsock_parr *parr) {
  int i = -1;
  void *ptr = fsock_parr_begin (parr, &i);

  for (; ptr != fsock_parr_end (parr); ptr = fsock_parr_next (parr, &i)) {
    struct fsock_sock *conn = (struct fsock_sock *)parr->elems[i];
    if (conn->type == FSOCK_SOCK_IN)
      fsock_parr_clear (&self->conns, conn->idx);
    fsock_sock_term (conn);
    free (conn);
    fsock_parr_clear (parr, i);
  }
}

static void fsock_sock_bind_parr_term (struct fsock_sock *self,
  struct fsock_parr *parr) {
  int i = -1;
  void *ptr = fsock_parr_begin (parr, &i);

  for (; ptr != fsock_parr_end (parr); ptr = fsock_parr_next (parr, &i)) {
    struct fsock_sock *bnd = (struct fsock_sock *)parr->elems[i];
    fsock_sock_conn_parr_term (self, &bnd->conns);
    fsock_sock_term (bnd);
    free (bnd);
    fsock_parr_clear (parr, i);
  }
}

void fsock_sock_term (struct fsock_sock *self) {
  if (self->fd != -1)
    close (self->fd);
  nn_efd_term (&self->efd);
  frm_parser_term (&self->parser);
  frm_out_frame_list_term (&self->ol);
  fsock_sock_bind_parr_term (self, &self->binds);
  fsock_sock_conn_parr_term (self, &self->conns);
  fsock_parr_term (&self->binds);
  fsock_parr_term (&self->conns);
  fsock_queue_term (&self->events);
  fsock_mutex_term (&self->sync);
  fsock_task_term (&self->t_start_read);
  fsock_task_term (&self->t_stop_read);
  fsock_task_term (&self->t_start_write);
  fsock_task_term (&self->t_stop_write);
  fsock_task_term (&self->t_close);
}

int fsock_queue_event (struct fsock_queue *queue, int type,
    struct frm_frame *fr, int conn) {
  struct fsock_event *event = malloc (sizeof (struct fsock_event));
  if (!event)
    return ENOMEM;
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
  fsock_queue_push (queue, &event->item);
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

static int fsock_sock_read_routinecxc (EV_P_ ev_io *r) {
  struct fsock_sock *conn = frm_cont (r, struct fsock_sock, rio);
  assert (conn->fd == r->fd);

  struct frm_cbuf *cbuf = frm_cbuf_new (1400);
  int rc = 0;

  if (cbuf == NULL)
    return EAGAIN;

  ssize_t nread = read (r->fd, cbuf->buf, 1400);

  if (nread == 0)
    goto stop;

  if (nread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    rc = EAGAIN;
    goto clean;
  }

  rc = frm_parser_parse (&conn->parser, cbuf, nread);

  if (rc != 0) {
    printf ("parse error. rc: %d\n", rc);
    goto stop;
  }

  struct fsock_queue queue = {NULL, NULL};
  int cnt;

  while (!frm_list_empty (&conn->parser.in_frames)) {
    struct frm_list_item *li = frm_list_begin (&conn->parser.in_frames);

    if (!li)
      break;

    // remove frame from the list
    frm_list_erase (&conn->parser.in_frames, li);

    struct frm_frame *fr = frm_cont (li, struct frm_frame, item);

    // frm_frame_destroy (fr);
    //fsock_sock_queue_event (conn->owner, FSOCK_EVENT_NEW_FRAME, fr, -1);
    int rc = fsock_queue_event (&queue, FSOCK_EVENT_NEW_FRAME, fr, -1);
    if (rc != 0) break;
    cnt++;
  }

  struct fsock_sock *owner = conn->owner;
  fsock_mutex_lock (&conn->owner->sync);
  owner->rcvqsz += cnt;
  //if (owner->sndqsz >= owner->sndhwm)
  //  ev_io_stop (EV_P_ r);
  if (cnt > 0) {
    if (owner->events.tail) {
      owner->events.tail->next = queue.head;
      owner->events.tail = queue.tail;
    }
    else if (owner->events.head) {
      owner->events.head->next = queue.head;
      owner->events.tail = queue.tail;
    } else {
      owner->events.head = queue.head;
      owner->events.tail = queue.tail;
    }
    if (owner->want_efd == 1) {
      nn_efd_signal (&owner->efd);
      owner->want_efd = 0;
    }
  }
  // let's do some high water mark magick
  if (owner->rcvhwm > 0 && owner->rcvhwm < owner->rcvqsz) {
    owner->reading = FSOCK_SOCK_STOP_OP;
    fsock_workers_lock();
    fsock_sock_bulk_schedule_unsafe(owner, t_stop_read, t_start_read);
    fsock_workers_unlock();
    fsock_workers_signal();
    //printf ("stoppped reads due to hwm\n");
  }
  fsock_mutex_unlock (&owner->sync);

  goto clean;

stop:
  printf ("bağlantı koptu.\n");
  ev_io_stop (EV_A_ r);
  ev_io_stop (EV_A_ &conn->wio);
  fsock_mutex_lock (&conn->sync);
  if (!(conn->flags & FSOCK_SOCK_ZOMBIE)) {
    conn->flags |= FSOCK_SOCK_ZOMBIE;
    printf ("conn->flags: %d FSOCK_SOCK_ZOMBIE: %d\n", conn->flags,
      FSOCK_SOCK_ZOMBIE);
  }
  fsock_mutex_unlock (&conn->sync);
  rc = EAGAIN;
clean:
  frm_cbuf_unref (cbuf);
  printf ("rc: %d\n", rc);
  return rc;
}

void fsock_sock_read_handleryyy (EV_P_ ev_io *r, int revents) {
  int rc;
  do {
    rc = fsock_sock_read_routinecxc (EV_A_ r);
    printf ("rc: %d\n", rc);
  }
  while (rc == 0);
}

void fsock_sock_read_handler (EV_P_ ev_io *r, int revents) {
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

  struct fsock_queue queue = {NULL, NULL};
  int cnt;

  while (!frm_list_empty (&conn->parser.in_frames)) {
    struct frm_list_item *li = frm_list_begin (&conn->parser.in_frames);

    if (!li)
      break;

    // remove frame from the list
    frm_list_erase (&conn->parser.in_frames, li);

    struct frm_frame *fr = frm_cont (li, struct frm_frame, item);

    // frm_frame_destroy (fr);
    //fsock_sock_queue_event (conn->owner, FSOCK_EVENT_NEW_FRAME, fr, -1);
    int rc = fsock_queue_event (&queue, FSOCK_EVENT_NEW_FRAME, fr, -1);
    if (rc != 0) break;
    cnt++;
  }

  struct fsock_sock *owner = conn->owner;
  fsock_mutex_lock (&conn->owner->sync);
  owner->rcvqsz += cnt;
  //if (owner->sndqsz >= owner->sndhwm)
  //  ev_io_stop (EV_P_ r);
  if (cnt > 0) {
    if (owner->events.tail) {
      owner->events.tail->next = queue.head;
      owner->events.tail = queue.tail;
    }
    else if (owner->events.head) {
      owner->events.head->next = queue.head;
      owner->events.tail = queue.tail;
    } else {
      owner->events.head = queue.head;
      owner->events.tail = queue.tail;
    }
    if (owner->want_efd == 1) {
      nn_efd_signal (&owner->efd);
      owner->want_efd = 0;
    }
  }
  // let's do some high water mark magick
  if (owner->rcvhwm > 0 && owner->rcvhwm < owner->rcvqsz) {
    owner->reading = FSOCK_SOCK_STOP_OP;
    fsock_workers_lock();
    fsock_sock_bulk_schedule_unsafe(owner, t_stop_read, t_start_read);
    fsock_workers_unlock();
    fsock_workers_signal();
    //printf ("stoppped reads due to hwm\n");
  }
  fsock_mutex_unlock (&owner->sync);

  goto clean;

stop:
  printf ("bağlantı koptu.\n");
  ev_io_stop (EV_A_ r);
  ev_io_stop (EV_A_ &conn->wio);
  fsock_mutex_lock (&conn->sync);
  if (!(conn->flags & FSOCK_SOCK_ZOMBIE)) {
    conn->flags |= FSOCK_SOCK_ZOMBIE;
    printf ("conn->flags: %d FSOCK_SOCK_ZOMBIE: %d\n", conn->flags,
      FSOCK_SOCK_ZOMBIE);
  }
  fsock_mutex_unlock (&conn->sync);
clean:
  frm_cbuf_unref (cbuf);
}


void fsock_sock_write_handler (EV_P_ ev_io *w, int revents) {
  struct fsock_sock *conn = frm_cont (w, struct fsock_sock, wio);
  assert (conn->fd == w->fd);

  if (conn->flags & FSOCK_SOCK_ZOMBIE)
    return;

  int removed = 0;
  int must_stop = 0;

  for (;;) {
    fsock_mutex_lock (&conn->sync);

    if (frm_list_empty (&conn->ol.list)) {
      fsock_mutex_unlock (&conn->sync);
      if (removed > 0) {
        must_stop = 1;
        break;
      }
      goto stop;
    }

    struct frm_out_frame_list ol;
    /*  Get local copy of the out frame list of the socket. */
    memcpy(&ol, &conn->ol, sizeof (struct frm_out_frame_list));
    frm_out_frame_list_init (&conn->ol);
    fsock_mutex_unlock (&conn->sync);
    while (!frm_list_empty (&ol.list)) {
      struct iovec iovs[128];
      int retiovcnt = -1;
      ssize_t tow = frm_out_frame_list_get_iovs (&ol, iovs, 128, &retiovcnt);
      ssize_t nw = writev (w->fd, iovs, retiovcnt);

      if (nw < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /*  todo: merge current copy with old copy. */
        fsock_mutex_lock (&conn->sync);
        if (!frm_list_empty (&conn->ol.list)) {
          /*  new frame added to the out list */
          if (ol.list.last) {
            ol.list.last->next = conn->ol.list.first;
            conn->ol.list.first->prev = ol.list.last;
          }
          else {
            ol.list.first->next = conn->ol.list.first;
            conn->ol.list.first->prev = ol.list.first;
          }
        }
        memcpy(&conn->ol, &ol, sizeof (struct frm_out_frame_list));
        fsock_mutex_unlock (&conn->sync);
        if (removed == 0)
          return;
        else
          break;
      }
      if (nw <= 0) {
        printf ("nw: %zd\n", nw);
        goto stop;
      }
      removed += frm_out_frame_list_written (&ol, nw);
    }
  }

  if (removed > 0) {
    struct fsock_sock *owner = conn->owner;
    fsock_mutex_lock (&conn->owner->sync);
    owner->sndqsz -= removed;
    int broadcast = 0;
    if (owner->sndhwm > 0 && owner->writing == FSOCK_SOCK_STOP_OP
        && owner->sndqsz < owner->sndhwm) {
      owner->writing = 0;
      //printf ("owner->sndhwm > 0 && owner->writing == FSOCK_SOCK_STOP_OP && owner->sndqsz < owner->sndhwm\n");
      //fsock_workers_lock();
      //fsock_sock_bulk_schedule_unsafe(owner, t_start_write, t_stop_write);
      //fsock_workers_unlock();
      //fsock_workers_signal();
      if (owner->want_wcond) {
        owner->want_wcond = 0;
        broadcast = 1;
      }
    }
    fsock_mutex_unlock (&conn->owner->sync);
    if (broadcast)
      pthread_cond_broadcast(&owner->wcond);
  }

  if (must_stop)
    goto stop;

  return;

stop:
  ev_io_stop (EV_A_ w);
  fsock_mutex_lock (&conn->sync);
  conn->writing = 0;
  fsock_mutex_unlock (&conn->sync);
}
