/*
    Copyright 2016 Fatih Kaya <1994274@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

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
