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

#include <framer/err.h>
#include "worker.h"

static void *fsock_worker_main_routine (void *arg) {
  struct fsock_worker *self;
  
  self = (struct fsock_worker *)arg;
  
  /*  Run the thread routine. */
  self->routine (self, self->arg);

  return NULL;
}

int fsock_worker_init (struct fsock_worker *self,
    fsock_worker_routine *routine, fsock_async_cb *async_routine, void *arg) {
  int rc;

  self->loop = ev_loop_new (0);

  if (self->loop == NULL) {
    return ENOMEM;
  }

  self->routine = routine;
  self->arg = arg;
  self->async.data = self;

  ev_async_init (&self->async, async_routine);
  ev_async_start (self->loop, &self->async);

  fsock_mutex_init (&self->sync);
  fsock_queue_init (&self->tasks);
  fsock_queue_item_init (&self->stop);

  rc = pthread_create(&self->thread, NULL, fsock_worker_main_routine, self);

  if (rc != 0)
    goto fail_free_loop;

  return 0;

fail_free_loop:
  ev_loop_destroy (self->loop);
  return rc;
}

int fsock_worker_term (struct fsock_worker *self) {

  fsock_mutex_lock (&self->sync);

  fsock_queue_push (&self->tasks, &self->stop);
  ev_async_send (self->loop, &self->async);

  fsock_mutex_unlock (&self->sync);

  // wait thread to exit
  pthread_join (self->thread, NULL);

  ev_async_stop (self->loop, &self->async);
  ev_loop_destroy (self->loop);
}

void fsock_worker_schedule_task (struct fsock_worker *self,
    struct fsock_worker_task *task) {
  fsock_mutex_lock (&self->sync);
  fsock_queue_push (&self->tasks, &task->item);
  fsock_mutex_unlock (&self->sync);
  ev_async_send (self->loop, &self->async);
}

/*  caller  responsible to locking task holder object */
void fsock_worker_erase_task (struct fsock_worker *self,
    struct fsock_worker_task *task)
{
  fsock_mutex_lock (&self->sync);
  if (fsock_queue_item_isinqueue (&task->item))
    fsock_queue_remove (&self->tasks, &task->item);
  fsock_mutex_unlock (&self->sync);
}

void fsock_worker_task_init (struct fsock_worker_task *self) {
  self->fn = NULL;
  fsock_queue_item_init (&self->item);
}

void fsock_worker_task_term (struct fsock_worker_task *self) {
  fsock_queue_item_term (&self->item);
}

void fsock_worker_task_set_func (struct fsock_worker_task *self,
    fsock_worker_task_fn fn) {
  self->fn = fn;
}
