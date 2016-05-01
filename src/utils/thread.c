#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <framer/cont.h>
#include "thread.h"
#include "../core/sock.h"
#include "../fsock.h"
#include "../core/global.h"
#include "anet.h"
#include "glock.h"

#define FSOCK_THR_BIND 9927400
#define FSOCK_THR_CONN 2568524

static void *thread_routine (void *data);
static void async_routine (EV_P_ ev_async *a, int revents);
static void task_routine (struct fsock_thread *thr, struct fsock_task *task);

struct task_data {
  char *addr;
  int port;
  struct fsock_sock *sock;
};

void fsock_task_init (struct fsock_task *self, int type, fsock_task_fn fn,
    void *data) {
  self->type = type;
  self->fn = fn;
  self->data = data;
  fsock_queue_item_init (&self->item);
}

void fsock_task_term (struct fsock_task *self, int type, fsock_task_fn fn,
    void *data) {
  // do nothing
}

struct fsock_thread *fsock_thread_new() {
  struct fsock_thread *self = calloc(1, sizeof (struct fsock_thread));
  if (self == NULL)
    return NULL;
  fsock_thread_init (self);
  return self;
}

void fsock_thread_init (struct fsock_thread *self) {
  self->loop = ev_loop_new (0);
  if (self->loop == NULL) {
    return;
  }
  fsock_mutex_init (&self->sync);
  fsock_queue_init (&self->jobs);
  fsock_task_init (&self->stop, 0, NULL, NULL);
  ev_async_init (&self->job_async, async_routine);
  ev_async_start (self->loop, &self->job_async);
}

void fsock_thread_start (struct fsock_thread *self) {
  pthread_create(&self->thread, NULL, thread_routine, self);
  return;
}

void fsock_thread_join (struct fsock_thread *self) {
  fsock_thread_schedule_task (self, &self->stop);
}

void fsock_thread_schedule_task (struct fsock_thread *self,
    struct fsock_task * task) {
  fsock_mutex_lock (&self->sync);
  fsock_queue_push (&self->jobs, &task->item);
  ev_async_send (self->loop, &self->job_async);
  fsock_mutex_unlock (&self->sync);
}

static void async_routine (EV_P_ ev_async *a, int revents) {
  struct fsock_thread *self = frm_cont (a, struct fsock_thread, job_async);
  fsock_mutex_lock (&self->sync);
  struct fsock_queue jobs;
  jobs.head = self->jobs.head;
  jobs.tail = self->jobs.tail;
  fsock_queue_init (&self->jobs);
  fsock_mutex_unlock (&self->sync);
  struct fsock_queue_item *item;
  struct fsock_task *task;
  while (item = fsock_queue_pop (&jobs)) {
    if (item == &self->stop.item) {
      printf ("stop thread\n");
      while (item = fsock_queue_pop (&jobs));
      ev_break (EV_A_ EVBREAK_ALL);
      return;
    }
    task = frm_cont (item, struct fsock_task, item);
    if (task->fn)
      task->fn (self, task);
    else
      assert (0 && "no callback set for the task\n");
  }
}

static void *thread_routine (void *data) {
  struct fsock_thread *self = (struct fsock_thread *)data;
  ev_run (self->loop, 0);
  return NULL;
}

static void accept_cb (EV_P_ ev_io *a, int revents) {
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

static void read_cb (EV_P_ ev_io *r, int revents)
{
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

static void write_cb (EV_P_ ev_io *w, int revents)
{
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

static void task_routine (struct fsock_thread *thr, struct fsock_task *task) {
  struct task_data *data = task->data;
  struct fsock_sock *sock = data->sock;
  switch (task->type) {
    case FSOCK_THR_BIND: {
      printf ("listen on: %s:%d {sock: %d|%d}\n", data->addr, data->port, sock->idx, sock->uniq);
      ev_io_init (&sock->rio, accept_cb, sock->fd, EV_READ);
      ev_io_start (thr->loop, &sock->rio);
    } break;
    case FSOCK_THR_CONN: {
      printf ("start i/o for conn\n");
      ev_io_init (&sock->rio, read_cb, sock->fd, EV_READ);
      ev_io_init (&sock->wio, write_cb, sock->fd, EV_WRITE);
      ev_io_start (thr->loop, &sock->rio);
      if (sock->type == FSOCK_SOCK_OUT) {
        printf ("bu bir dış bağlantı\n");
        struct frm_frame *frs = calloc(3, sizeof(struct frm_frame));
        for (int i = 0; i < 3; i++) {
          struct frm_out_frame_list_item *li = frm_out_frame_list_item_new();
      
          struct frm_frame *fr = &frs[i];
          frm_frame_init (fr);
          frm_frame_set_data (fr, "fatih", 5);
          frm_out_frame_list_item_set_frame (li, fr);
          frm_out_frame_list_insert (&sock->ol, li);
      
          // bu frame'i daha fazla kullanmayacağım
          frm_frame_term (fr);
        }
        ev_io_start (thr->loop, &sock->wio);
      }
    } break;
    default:
      printf ("[fsocket] unknown task type: %d\n", task->type);
      assert (0);
  }
}

void fsock_thread_start_listen (struct fsock_thread *self,
    struct fsock_sock *sock, char *addr, int port) {
  struct task_data *data = malloc(sizeof (struct task_data));
  struct fsock_task *task = malloc (sizeof (struct fsock_task));
  data->addr = strdup(addr);
  data->port = port;
  data->sock = sock;
  fsock_task_init (task, FSOCK_THR_BIND, task_routine, data);
  fsock_thread_schedule_task (self, task);
}

void fsock_thread_start_connection (struct fsock_thread *self,
    struct fsock_sock *sock) {
  struct task_data *data = malloc(sizeof (struct task_data));
  struct fsock_task *task = malloc (sizeof (struct fsock_task));
  data->sock = sock;
  fsock_task_init (task, FSOCK_THR_CONN, task_routine, data);
  fsock_thread_schedule_task (self, task);
}
