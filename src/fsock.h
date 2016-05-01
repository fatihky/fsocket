/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright 2015-2016 Garrett D'Amore <garrett@damore.org>
    Copyright (c) 2015-2016 Jack R. Dunaway.  All rights reserved.

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

#ifndef FSOCK_H_INCLUDED
#define FSOCK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/*  Handle DSO symbol visibility. */
#if defined FSOCK_NO_EXPORTS
#   define FSOCK_EXPORT
#else
#   if defined _WIN32
#      if defined FSOCK_STATIC_LIB
#          define FSOCK_EXPORT extern
#      elif defined FSOCK_SHARED_LIB
#          define FSOCK_EXPORT __declspec(dllexport)
#      else
#          define FSOCK_EXPORT __declspec(dllimport)
#      endif
#   else
#      if defined __SUNPRO_C
#          define FSOCK_EXPORT __global
#      elif (defined __GNUC__ && __GNUC__ >= 4) || \
             defined __INTEL_COMPILER || defined __clang__
#          define FSOCK_EXPORT __attribute__ ((visibility("default")))
#      else
#          define FSOCK_EXPORT
#      endif
#   endif
#endif

/******************************************************************************/
/*  ABI versioning support.                                                   */
/******************************************************************************/

/*  Don't change this unless you know exactly what you're doing and have      */
/*  read and understand the following documents:                              */
/*  www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html     */
/*  www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html  */

/*  The current interface version. */
#define FSOCK_VERSION_CURRENT 0

/*  The latest revision of the current interface. */
#define FSOCK_VERSION_REVISION 1

/*  How many past interface versions are still supported. */
#define FSOCK_VERSION_AGE 0

/******************************************************************************/
/*  Errors.                                                                   */
/******************************************************************************/

/*  A number random enough not to collide with different errno ranges on      */
/*  different OSes. The assumption is that error_t is at least 32-bit type.   */
#define FSOCK_HAUSNUMERO 156384712

/*  On some platforms some standard POSIX errnos are not defined.    */
#ifndef ENOTSUP
#define ENOTSUP (FSOCK_HAUSNUMERO + 1)
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT (FSOCK_HAUSNUMERO + 2)
#endif
#ifndef ENOBUFS
#define ENOBUFS (FSOCK_HAUSNUMERO + 3)
#endif
#ifndef ENETDOWN
#define ENETDOWN (FSOCK_HAUSNUMERO + 4)
#endif
#ifndef EADDRINUSE
#define EADDRINUSE (FSOCK_HAUSNUMERO + 5)
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL (FSOCK_HAUSNUMERO + 6)
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED (FSOCK_HAUSNUMERO + 7)
#endif
#ifndef EINPROGRESS
#define EINPROGRESS (FSOCK_HAUSNUMERO + 8)
#endif
#ifndef ENOTSOCK
#define ENOTSOCK (FSOCK_HAUSNUMERO + 9)
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT (FSOCK_HAUSNUMERO + 10)
#endif
#ifndef EPROTO
#define EPROTO (FSOCK_HAUSNUMERO + 11)
#endif
#ifndef EAGAIN
#define EAGAIN (FSOCK_HAUSNUMERO + 12)
#endif
#ifndef EBADF
#define EBADF (FSOCK_HAUSNUMERO + 13)
#endif
#ifndef EINVAL
#define EINVAL (FSOCK_HAUSNUMERO + 14)
#endif
#ifndef EMFILE
#define EMFILE (FSOCK_HAUSNUMERO + 15)
#endif
#ifndef EFAULT
#define EFAULT (FSOCK_HAUSNUMERO + 16)
#endif
#ifndef EACCES
#define EACCES (FSOCK_HAUSNUMERO + 17)
#endif
#ifndef EACCESS
#define EACCESS (EACCES)
#endif
#ifndef ENETRESET
#define ENETRESET (FSOCK_HAUSNUMERO + 18)
#endif
#ifndef ENETUNREACH
#define ENETUNREACH (FSOCK_HAUSNUMERO + 19)
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH (FSOCK_HAUSNUMERO + 20)
#endif
#ifndef ENOTCONN
#define ENOTCONN (FSOCK_HAUSNUMERO + 21)
#endif
#ifndef EMSGSIZE
#define EMSGSIZE (FSOCK_HAUSNUMERO + 22)
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT (FSOCK_HAUSNUMERO + 23)
#endif
#ifndef ECONNABORTED
#define ECONNABORTED (FSOCK_HAUSNUMERO + 24)
#endif
#ifndef ECONNRESET
#define ECONNRESET (FSOCK_HAUSNUMERO + 25)
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT (FSOCK_HAUSNUMERO + 26)
#endif
#ifndef EISCONN
#define EISCONN (FSOCK_HAUSNUMERO + 27)
#define FSOCK_EISCONN_DEFINED
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT (FSOCK_HAUSNUMERO + 28)
#endif

/*  Native fsock error codes.                                                 */
/*  fsocket terminating.                                                      */
#ifndef EFTERM
#define EFTERM (FSOCK_HAUSNUMERO + 53)
#endif

/*  This function retrieves the errno as it is known to the library.          */
/*  The goal of this function is to make the code 100% portable, including    */
/*  where the library is compiled with certain CRT library (on Windows) and   */
/*  linked to an application that uses different CRT library.                 */
// FSOCK_EXPORT int nn_errno (void);

/*  Resolves system errors and native errors to human-readable string.        */
// FSOCK_EXPORT const char *nn_strerror (int errnum);

#include <framer/framer.h>
#include "utils/queue.h"

#pragma once

/*  Send/recv options.                                                        */
#define FSOCK_DONWAIT 1

#define FSOCK_EVENT_NEW_CONN 1
#define FSOCK_EVENT_LOST_CONN 2
#define FSOCK_EVENT_NEW_FRAME 3

struct fsock_event {
  int type;
  union {
    int conn;
    struct frm_frame *frame;
  };
  struct fsock_queue_item item;
};

int fsock_socket (char *name);
int fsock_bind (int s, char *addr, int port);
int fsock_connect (int s, char *addr, int port);
struct fsock_event *fsock_get_event (int s, int flags);
int fsock_rand (int s);
int fsock_send (int s, int c, struct frm_frame *fr, int flags);

#ifdef __cplusplus
}
#endif

#endif
