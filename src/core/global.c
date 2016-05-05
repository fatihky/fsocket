#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <framer/cont.h>
#include "../utils/glock.h"
#include "global.h"
#include "sock.h"
#include "../fsock.h"
#include "../utils/anet.h"

static int g_initialized = 0;
static int thr_last_idx = 0;

static struct {
  struct fsock_thread threads[4];
  struct fsock_parr   socks;
} self;

/*  call after fsock_glock_lock() */
struct fsock_thread *f_choose_thr() {
  struct fsock_thread *thr;
  thr = &self.threads[thr_last_idx++];
  if (thr_last_idx == 4)
    thr_last_idx = 0;
  return thr;
}

static int fsock_global_init() {
  g_initialized = 1;
  fsock_parr_init (&self.socks, 10);
  for (int i = 0; i < 4; i++) {
    fsock_thread_init (&self.threads[i]);
    fsock_thread_start (&self.threads[i]);
  }
  return 0;
}

#define FSOCK_GLOBAL_INIT() do { \
  if (!g_initialized) {          \
    rc = fsock_global_init();    \
    if (rc != 0) {               \
      errno = rc;                \
      fsock_glock_unlock();      \
      return -1;                 \
    }                            \
  }                              \
} while (0)

int fsock_socket (char *name) {
  int rc;
  int index;
  struct fsock_sock *sock = malloc (sizeof (struct fsock_sock));
  if (!sock) {
    errno = ENOMEM;
    return -1;
  }

  fsock_glock_lock();
  FSOCK_GLOBAL_INIT();
  index = fsock_parr_insert (&self.socks, sock);
  if (index < 0) {
    free (sock);
    fsock_glock_unlock();
    errno = ENOMEM;
    return -1;
  }
  fsock_sock_init (sock, FSOCK_SOCK_BASE);
  sock->idx = index;
  fsock_glock_unlock();
  return index;
}

int fsock_bind (int s, char *addr, int port) {
  struct fsock_sock *sock;
  struct fsock_sock *bnd = malloc (sizeof (struct fsock_sock));
  struct fsock_thread *thr;
  char err[255] = {0};
  int fd;
  int index;
  if (!bnd) {
    errno = ENOMEM;
    return -1;
  }
  fsock_glock_lock();
  thr = f_choose_thr();
  fsock_glock_unlock();
  fsock_sock_init (bnd, FSOCK_SOCK_BIND);
  sock = self.socks.elems[s];
  bnd->owner = sock;
  bnd->thr = thr;
  fd = anetTcpServer(err, port, addr, 1024);
  if (fd < 0) {
    if (err)
      printf ("listen err: %s %d %s\n", err, errno, strerror (errno));
    return -1;
  }
  index = fsock_parr_insert (&sock->binds, bnd);
  if (index < 0)
    assert (0 && "[fsocket] fsock_bind: index < 0");
  bnd->idx = index;
  bnd->fd = fd;
  fsock_thread_start_listen (thr, bnd, addr, port);
  return index;
}

int fsock_connect (int s, char *addr, int port) {
  struct fsock_sock *sock;
  struct fsock_sock *conn = malloc (sizeof (struct fsock_sock));
  struct fsock_thread *thr;
  char err[255] = {0};
  int fd;
  int index;
  if (!conn) {
    errno = ENOMEM;
    return -1;
  }
  fsock_glock_lock();
  thr = f_choose_thr();
  fsock_glock_unlock();
  fsock_sock_init (conn, FSOCK_SOCK_OUT);
  sock = self.socks.elems[s];
  conn->owner = sock;
  conn->thr = thr;
  fd = anetTcpNonBlockConnect(err, addr, port);
  if (fd < 0) {
    if (err)
      printf ("listen err: %s %d %s\n", err, errno, strerror (errno));
    return -1;
  }
  fsock_mutex_lock (&sock->sync);
  index = fsock_parr_insert (&sock->conns, conn);
  fsock_mutex_unlock (&sock->sync);
  if (index < 0)
    assert (0 && "[fsocket] fsock_bind: index < 0");
  conn->idx = index;
  conn->fd = fd;
  fsock_thread_start_connection (thr, conn);

  return index;
}

struct fsock_event *fsock_get_event (int s, int flags) {
  struct fsock_sock *sock;
  sock = self.socks.elems[s];
  assert (sock->type == FSOCK_SOCK_BASE);
  fsock_mutex_lock(&sock->sync);
  if (fsock_queue_empty (&sock->events)) {
    if (flags & FSOCK_DONWAIT) {
      fsock_mutex_unlock(&sock->sync);
      return NULL;
    }
    // printf ("hiç iş yok. efd'yi beklemek lazım. idx: %d|%d [empty: %d]\n", sock->idx, sock->uniq, fsock_queue_empty (&sock->events));
    sock->want_efd = 1;
    fsock_mutex_unlock (&sock->sync);

    int rc = nn_efd_wait (&sock->efd, 10000);
    if (rc == -ETIMEDOUT) {
      printf ("zamanaşımına uğradı efd\n");
      errno = ETIMEDOUT;
      return NULL;
    }

    assert (rc == 0);
    nn_efd_unsignal (&sock->efd);
    fsock_mutex_lock (&sock->sync);
  }
  assert (!fsock_queue_empty (&sock->events));
  struct fsock_queue_item *item = fsock_queue_pop (&sock->events);
  fsock_mutex_unlock (&sock->sync);
  return frm_cont (item, struct fsock_event, item);
}

int fsock_rand (int s) {
  return ((struct fsock_sock *)self.socks.elems[s])->uniq;
}

int fsock_send (int s, int c, struct frm_frame *fr, int flags) {
  struct fsock_sock *sock;
  struct fsock_sock *conn;
  sock = self.socks.elems[s];
  conn = sock->conns.elems[c];
  assert (sock->type == FSOCK_SOCK_BASE);
  assert (fr && "you should pass a frame to send");
  if (conn->flags & FSOCK_SOCK_ZOMBIE) {
    errno = EINVAL;
    return -1;
  }
  struct frm_out_frame_list_item *li = frm_out_frame_list_item_new();
  if (!li) {
    errno = ENOMEM;
    return -1;
  }
  frm_out_frame_list_item_set_frame (li, fr);
  fsock_mutex_lock(&conn->sync);
  if (conn->writing == 0) {
    fsock_thread_schedule_task (conn->thr, &conn->t_start_write);
    conn->writing = 1;
  }
  frm_out_frame_list_insert (&conn->ol, li);
  fsock_mutex_unlock(&conn->sync);
}

static inline int fsock_conn_type (struct fsock_sock *sock, int c) {
  return ((struct fsock_sock *)sock->conns.elems[c])->type;
}

static int fsock_send_all (int s, struct fsock_parr *parr, struct frm_frame *fr, int dflags,
    int sndflags) {
  struct fsock_sock *sock;
  sock = self.socks.elems[s];
  fsock_mutex_lock (&sock->sync);
  int index;
  int rc = 0;
  for(void *ptr = fsock_parr_begin (parr, &index);
      ptr != fsock_parr_end (parr);
      ptr = fsock_parr_next (parr, &index)) {

    if (dflags == 0 || /*  No flags set. */
      ((dflags & FSOCK_DIST_IN) && /*  Send to only incoming conns. */
        fsock_conn_type(sock, index) == FSOCK_SOCK_IN) ||
      ((dflags & FSOCK_DIST_OUT) && /*  Send to only outgoing conns. */
        fsock_conn_type(sock, index) == FSOCK_SOCK_OUT)) {

      rc = fsock_send (s, index, fr, sndflags);
      if (rc != 0 && (sndflags & FSOCK_STOP_ONERR))
        break;
    }
  }
  fsock_mutex_unlock (&sock->sync);
  return rc;
}

int fsock_sendc (int s, int type, int b, int c, struct frm_frame *fr,
    int dflags, int sndflags) {

  struct fsock_sock *sock;
  sock = self.socks.elems[s];

  if (type == FSOCK_SND_ALL)
    return fsock_send_all (s, &sock->conns, fr, dflags, sndflags);

  if (type == FSOCK_SND_BIND) {
    // get bind socket
    assert (sock->binds.last > b);
    struct fsock_sock *bnd = sock->binds.elems[b];
    assert (bnd);
    return fsock_send_all (s, &bnd->conns, fr, dflags, sndflags);
  }

  if (type == FSOCK_SND_BINDCONN) {
    // get bind socket
    assert (sock->binds.last > b);
    struct fsock_sock *bnd = sock->binds.elems[b];
    assert (bnd);

    // get connection
    assert (bnd->conns.last > c);
    struct fsock_sock *conn = bnd->conns.elems[c];
    assert (conn);
    return fsock_send(s, conn->idxlocal, fr, sndflags);
  }

  errno = EINVAL;
  return -1;
}