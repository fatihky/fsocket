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

void fsock_task_term (struct fsock_task *self) {
  assert (!fsock_queue_item_isinqueue (&self->item));
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

void fsock_thread_term (struct fsock_thread *self) {
  ev_loop_destroy (self->loop);
  fsock_mutex_term (&self->sync);
  fsock_queue_term (&self->jobs);
  fsock_task_term (&self->stop);
}

void fsock_thread_start (struct fsock_thread *self) {
  pthread_create (&self->thread, NULL, thread_routine, self);
  return;
}

void fsock_thread_join (struct fsock_thread *self) {
  fsock_thread_schedule_task (self, &self->stop);
  pthread_join (self->thread, NULL);
}

void fsock_thread_schedule_task (struct fsock_thread *self,
    struct fsock_task *task) {
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

static void task_routine (struct fsock_thread *thr, struct fsock_task *task) {
  struct task_data *data = task->data;
  struct fsock_sock *sock = data->sock;
  switch (task->type) {
    case FSOCK_THR_BIND: {
      printf ("listen on: %s:%d {sock: %d|%d}\n", data->addr, data->port, sock->idx, sock->uniq);
      ev_io_init (&sock->rio, fsock_sock_accept_handler, sock->fd, EV_READ);
      ev_io_start (thr->loop, &sock->rio);
      free (data->addr);
      free (data);
      free (task);
    } break;
    case FSOCK_THR_CONN: {
      printf ("start i/o for conn\n");
      ev_io_init (&sock->rio, fsock_sock_read_handler, sock->fd, EV_READ);
      ev_io_init (&sock->wio, fsock_sock_write_handler, sock->fd, EV_WRITE);
      ev_io_start (thr->loop, &sock->rio);
      free (data);
      free (task);
      if (sock->type == FSOCK_SOCK_OUT) {
        printf ("bu bir dış bağlantı\n");
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
