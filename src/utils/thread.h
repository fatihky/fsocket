#include <ev.h>
#include <pthread.h>
#include "queue.h"
#include "parr.h"
#include "mutex.h"

#pragma once

struct fsock_task;
struct fsock_sock;
struct fsock_thread;

typedef void (*fsock_task_fn) (struct fsock_thread *thr, struct fsock_task *task);

struct fsock_task {
  int type; /* rw */
  fsock_task_fn fn; /* rw */
  void *data; /* rw */
  struct fsock_queue_item item; /* private */
};

struct fsock_thread {
  struct ev_loop *loop;
  pthread_t thread;
  ev_async job_async;
  struct fsock_mutex sync;
  struct fsock_queue jobs;
  struct fsock_task stop;
};

void fsock_task_init (struct fsock_task *self, int type, fsock_task_fn fn,
  void *data);
void fsock_task_term (struct fsock_task *self, int type, fsock_task_fn fn,
  void *data);

struct fsock_thread *fsock_thread_new();
void fsock_thread_init (struct fsock_thread *self);
void fsock_thread_start (struct fsock_thread *self);
void fsock_thread_join (struct fsock_thread *self);

void fsock_thread_schedule_task (struct fsock_thread *self,
  struct fsock_task *task);
void fsock_thread_start_listen (struct fsock_thread *self,
  struct fsock_sock *sock, char *addr, int port);
void fsock_thread_start_connection (struct fsock_thread *self,
  struct fsock_sock *sock);
