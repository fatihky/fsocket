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

#include <stddef.h>
#include <stdlib.h>

#include <framer/err.h>

#include "parr.h"

void fsock_parr_init (struct fsock_parr *self, int increment) {
  self->elems = NULL;
  self->last = 0;
  self->capacity = 0;
  self->increment = increment;
}

void fsock_parr_term (struct fsock_parr *self) {
  if (self->elems)
    free (self->elems);
}

static int fsock_parr_realloc (struct fsock_parr *self, int increment) {
  size_t allocsz = sizeof (void *) * (self->capacity + increment);
  void **elems = (void **) realloc((void *)self->elems, allocsz);

  if (elems == NULL) {
    printf ("capacity: %d\n", self->capacity);
    printf ("increment: %d\n", increment);
    printf ("elems == NULL vay aq {allocsz: %zu}\n", allocsz);
    return ENOMEM;
  }

  self->capacity += increment;
  self->elems = elems;

  return 0;
}

int fsock_parr_insert (struct fsock_parr *self, void *elem) {
  frm_assert (self->last >= 0);
  frm_assert (self->capacity == 0 || self->last < self->capacity);

  if (self->capacity == 0 || self->last == self->capacity - 1) {
    int rc = fsock_parr_realloc (self, self->increment);

    if (rc != 0) {
      errno = rc;
      return -1;
    }
  }

  int ret = 0; // self->last;

  for (; ret < self->last; ret++) {
    if (self->elems[ret] == FSOCK_PARR_EMPTY)
      break;
  }

  self->elems[ret] = elem;
  self->last++;

  return ret;
}

int fsock_parr_empty (struct fsock_parr *self) {
  for (int i = 0; i < self->last; i++)
    if (self->elems[i] != FSOCK_PARR_EMPTY)
      return 0;

  if (self->elems == NULL)
    printf ("self->elems == NULL\n");

  return 1;
}

int fsock_parr_size (struct fsock_parr *self) {
  int len = 0;

  if (self->capacity == 0)
    return 0;

  for (int i = 0; i < self->last; i++)
    if (self->elems[i] != FSOCK_PARR_EMPTY)
      len++;

  return len;
}

static int fsock_parr_can_shrink (struct fsock_parr *self) {
  int diff = self->last - (self->capacity - self->increment);

  for (int i = 0; i < diff; i++) {
    int index = self->last - 1 - i;
    if (self->elems[index] != FSOCK_PARR_EMPTY)
      return 0; /*  cannot shrink */
  }

  return 1;
}

int fsock_parr_clear (struct fsock_parr *self, int index) {
  frm_assert (index >= 0 && index < self->last);
  self->elems[index] = FSOCK_PARR_EMPTY;

  while (fsock_parr_can_shrink (self)) {
    int rc;

    if (self->capacity == self->increment) {
      fsock_parr_term (self);
      fsock_parr_init (self, self->increment);
      return 0;
    }

    rc = fsock_parr_realloc (self, -self->increment);

    // probably we will never get here
    if (rc != 0) {
      errno = rc;
      return -1;
    }
  }

  return 0;
}

void *fsock_parr_begin (struct fsock_parr *self, int *index) {
  void *ptr;
  int i = 0;

  if (self->last == 0 ||self->elems == NULL)
    goto noelems;

  for (int i = 0; i < self->last; i++) {
    if (self->elems[i] == FSOCK_PARR_EMPTY)
      continue;

    *index = i;
    return self->elems[i];
  }

noelems:
  *index = -1;
  return NULL;
}

void *fsock_parr_next (struct fsock_parr *self, int *index) {
  frm_assert (*index > 0 && *index <= self->last);

  if (*index == self->last)
    return NULL;

  for (int i = *index + 1; i < self->last; i++) {
    *index++;

    if (self->elems[i] == FSOCK_PARR_EMPTY)
      continue;

    return self->elems[i];
  }

  // no valid elements exist
  return NULL;
}

void *fsock_parr_end (struct fsock_parr *self) {
  return NULL;
}
