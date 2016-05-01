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

#ifndef FSOCK_PARR_INCLUDED
#define FSOCK_PARR_INCLUDED

#define FSOCK_PARR_EMPTY (void *)-1
#define FSOCK_PARR_END   (void *)-2

struct fsock_parr {
  /*  array of pointers */
  void **elems;
  /*  last index in the array */
  int last;
  /*  capacity of the array */
  int capacity;
  /*  increment size of the array */
  int increment;
};

void fsock_parr_init (struct fsock_parr *self, int increment);
void fsock_parr_term (struct fsock_parr *self);
int fsock_parr_empty (struct fsock_parr *self);
int fsock_parr_insert (struct fsock_parr *self, void *elem);
int fsock_parr_size (struct fsock_parr *self);
int fsock_parr_clear (struct fsock_parr *self, int index);
void *fsock_parr_begin (struct fsock_parr *self, int *index);
void *fsock_parr_next (struct fsock_parr *self, int *index);
void *fsock_parr_end (struct fsock_parr *self);

#endif
