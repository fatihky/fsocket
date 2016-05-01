/*
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

#include <stddef.h>

#include <framer/err.h>
#include "queue.h"

void fsock_queue_init (struct fsock_queue *self)
{
    self->head = NULL;
    self->tail = NULL;
}

void fsock_queue_term (struct fsock_queue *self)
{
    self->head = NULL;
    self->tail = NULL;
}

int fsock_queue_empty (struct fsock_queue *self)
{
    return self->head ? 0 : 1;
}

void fsock_queue_push (struct fsock_queue *self, struct fsock_queue_item *item)
{
    frm_assert (item->next == FSOCK_QUEUE_NOTINQUEUE);

    item->next = NULL;
    if (!self->head)
        self->head = item;
    if (self->tail)
        self->tail->next = item;
    self->tail = item;
}

void fsock_queue_remove (struct fsock_queue *self, struct fsock_queue_item *item)
{
    struct fsock_queue_item *it;
    struct fsock_queue_item *prev;

    if (item->next == FSOCK_QUEUE_NOTINQUEUE)
        return;

    prev = NULL;
    for (it = self->head; it != NULL; it = it->next) {
        if (it == item) {
            if (self->head == it)
                self->head = it->next;
            if (self->tail == it)
                self->tail = prev;
            if (prev)
                prev->next = it->next;
            item->next = FSOCK_QUEUE_NOTINQUEUE;
            return;
        }
        prev = it;
    }
}

struct fsock_queue_item *fsock_queue_pop (struct fsock_queue *self)
{
    struct fsock_queue_item *result;

    if (!self->head)
        return NULL;
    result = self->head;
    self->head = result->next;
    if (!self->head)
        self->tail = NULL;
    result->next = FSOCK_QUEUE_NOTINQUEUE;
    return result;
}

void fsock_queue_item_init (struct fsock_queue_item *self)
{
    self->next = FSOCK_QUEUE_NOTINQUEUE;
}

void fsock_queue_item_term (struct fsock_queue_item *self)
{
    frm_assert (self->next == FSOCK_QUEUE_NOTINQUEUE);
}

int fsock_queue_item_isinqueue (struct fsock_queue_item *self)
{
    return self->next == FSOCK_QUEUE_NOTINQUEUE ? 0 : 1;
}

