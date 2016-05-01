#ifndef FSOCK_WORKER_INCLUDED
#define FSOCK_WORKER_INCLUDED

#include <ev.h>
#include "queue.h"
#include "mutex.h"

struct fsock_worker;
struct fsock_worker_task;

typedef void (fsock_worker_routine) (struct fsock_worker *self, void *arg);
typedef void (fsock_async_cb) (EV_P_ ev_async *a, int revents);
typedef void (*fsock_worker_task_fn) (struct fsock_worker_task *self);

struct fsock_worker_task {
  int type; /*  read/write */
  fsock_worker_task_fn fn; /*  read/write */
  struct fsock_queue_item item; /* read-only */
};

struct fsock_worker {
  struct ev_loop *loop;
  struct fsock_mutex sync;
  struct fsock_queue tasks;
  struct fsock_queue_item stop;

  void *arg;
  ev_async async;
  pthread_t thread;
  fsock_worker_routine *routine;
};

int fsock_worker_init (struct fsock_worker *self,
  fsock_worker_routine *routine, fsock_async_cb *async_routine, void *arg);
int fsock_worker_term (struct fsock_worker *self);
void fsock_worker_schedule_task (struct fsock_worker *self,
  struct fsock_worker_task *task);
/*  caller  responsible to locking task holder object */
void fsock_worker_erase_task (struct fsock_worker *self,
  struct fsock_worker_task *task);

void fsock_worker_task_init (struct fsock_worker_task *self);
void fsock_worker_task_term (struct fsock_worker_task *self);
void fsock_worker_task_set_func (struct fsock_worker_task *self,
  fsock_worker_task_fn fn);

#endif
