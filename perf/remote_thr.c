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
  int connect_port;
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
    printf ("usage: local_thr <connect port> <msg-size> <msg-count>\n");
    return 1;
  }
  connect_port = atoi (argv [1]);
  sz = atoi (argv [2]);
  count = atoi (argv [3]);

  s = fsock_socket ("remote thr client");
  assert (s != -1);
  rc = fsock_connect (s, "0.0.0.0", connect_port);
  assert (rc >= 0);

  fr = frm_frame_new();
  assert (fr);
  char *fake = malloc (sz);
  frm_frame_set_data (fr, fake, sz);
  free (fake);

  for (i = 0; i != count; i++) {
    rc = fsock_send (s, 0, fr, 0);
    assert (rc == 0);
  }

  // rc = nn_close (s);
  assert (1 || rc == 0);

  // TODO: wait till all data sent
  sleep (500);

  return 0;
}
