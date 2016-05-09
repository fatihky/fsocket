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
  int rts;
  int nbytes;
  int s;
  int rc;
  int i;
  int opt;
  struct fsock_event *ev;
  struct frm_frame *fr;

  if (argc != 4) {
      printf ("usage: local_lat <bind-port> <msg-size> <roundtrips>\n");
      return 1;
  }
  bind_port = atoi (argv [1]);
  sz = atoi (argv [2]);
  rts = atoi (argv [3]);

  s = fsock_socket ("local lat server");
  assert (s != -1);
  //opt = 1;
  //rc = nn_setsockopt (s, NN_TCP, NN_TCP_NODELAY, &opt, sizeof (opt));
  //assert (rc == 0);
  rc = fsock_bind (s, "0.0.0.0", bind_port);
  assert (rc >= 0);

  fr = frm_frame_new();
  assert (fr);
  char *fake = malloc (sz);
  memset (fake, 111, sz);
  frm_frame_set_data (fr, fake, sz);
  free (fake);

  for (i = 0; i != rts; i++) {
    ev = fsock_get_event (s, 0);
    assert (ev->type == FSOCK_EVENT_NEW_FRAME && ev->frame->size == (int)sz);
    rc = fsock_send (s, 0, fr, 0);
    assert (rc == 0);
  }

  //rc = nn_close (s);
  //assert (rc == 0);
  sleep (1);

  return 0;
}
