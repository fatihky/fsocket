/*
  Copyright (c) 2016 Fatih Kaya <1994274@gmail.com>  All rights reserved.
  Copyright (c) 2012 Martin Sustrik  All rights reserved.
  
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "../src/fsock.h"
#include "../src/utils/stopwatch.h"

int main (int argc, char *argv [])
{
  int bind_port;
  size_t sz;
  int count;
  int s;
  int rc;
  int i;
  struct nn_stopwatch sw;
  struct fsock_event *ev;
  struct frm_frame *fr;
  uint64_t total;
  uint64_t thr;
  double mbs;

  if (argc != 4) {
    printf ("usage: local_thr <bind port> <msg-size> <msg-count>\n");
    return 1;
  }
  bind_port = atoi (argv [1]);
  sz = atoi (argv [2]);
  count = atoi (argv [3]);

  s = fsock_socket ("local thr server");
  assert (s != -1);
  rc = fsock_bind (s, "0.0.0.0", bind_port);
  assert (rc >= 0);

  fr = frm_frame_new();
  assert (fr);

  ev = fsock_get_event (s, 0);
  assert (ev->type == FSOCK_EVENT_NEW_FRAME && ev->frame->size == (int)sz);
  //assert (ev->type == FSOCK_EVENT_NEW_CONN);

  nn_stopwatch_init (&sw);
  for (i = 0; i != count - 1; i++) {
    ev = fsock_get_event (s, 0);
    if (ev->type != FSOCK_EVENT_NEW_FRAME)
      printf ("ev->type(%d) != FSOCK_EVENT_NEW_FRAME(%d)\n", ev->type, FSOCK_EVENT_NEW_FRAME);
    assert (ev->type == FSOCK_EVENT_NEW_FRAME && ev->frame->size == (int)sz);
  }
  total = nn_stopwatch_term (&sw);
  if (total == 0)
    total = 1;

  thr = (uint64_t) ((double) count / (double) total * 1000000);
  mbs = (double) (thr * sz * 8) / 1000000;

  printf ("message size: %d [B]\n", (int) sz);
  printf ("message count: %d\n", (int) count);
  printf ("throughput: %d [msg/s]\n", (int) thr);
  printf ("throughput: %.3f [Mb/s]\n", (double) mbs);

  // rc = nn_close (s);
  assert (1 || rc == 0);

  return 0;
}
