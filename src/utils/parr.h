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
