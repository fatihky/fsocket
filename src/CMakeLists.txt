#
#   Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
#   Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
#   Copyright (c) 2015-2016 Jack R. Dunaway. All rights reserved.
#
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom
#   the Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#   IN THE SOFTWARE.
#

set (FSOCK_SOURCES
    nn.h

    core/global.h
    core/global.c

    utils/anet.h
    utils/anet.c
)

if (WIN32)
    list (APPEND FSOCK_SOURCES
        aio/usock_win.h
        aio/usock_win.inc
        aio/worker_win.h
        aio/worker_win.inc
        utils/thread_win.h
        utils/thread_win.inc
        utils/win.h
    )
elseif (UNIX)
    list (APPEND FSOCK_SOURCES
        aio/usock_posix.h
        aio/usock_posix.inc
        aio/worker_posix.h
        aio/worker_posix.inc
        utils/thread_posix.h
        utils/thread_posix.inc
    )
else ()
    message (FATAL_ERROR "Assertion failed; this path is unreachable.")
endif ()

if (FSOCK_HAVE_EPOLL)
    add_definitions (-DFSOCK_USE_EPOLL)
    list (APPEND FSOCK_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_epoll.h
        aio/poller_epoll.inc
    )
elseif (FSOCK_HAVE_KQUEUE)
    add_definitions (-DFSOCK_USE_KQUEUE)
    list (APPEND FSOCK_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_kqueue.h
        aio/poller_kqueue.inc
    )
elseif (FSOCK_HAVE_POLL)
    add_definitions (-DFSOCK_USE_POLL)
    list (APPEND FSOCK_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_poll.h
        aio/poller_poll.inc
    )
elseif (FSOCK_HAVE_WINSOCK)
    # No operation
else ()
    message (SEND_ERROR "ERROR: could not determine socket polling method.")
    message (SEND_ERROR "${ISSUE_REPORT_MSG}" )
endif ()

if (FSOCK_HAVE_EVENTFD)
    add_definitions (-DFSOCK_USE_EVENTFD)
    list (APPEND FSOCK_SOURCES
        utils/efd_eventfd.h
        utils/efd_eventfd.inc
    )
elseif (FSOCK_HAVE_PIPE)
    add_definitions (-DFSOCK_USE_PIPE)
    list (APPEND FSOCK_SOURCES
        utils/efd_pipe.h
        utils/efd_pipe.inc
    )
elseif (FSOCK_HAVE_SOCKETPAIR)
    add_definitions (-DFSOCK_USE_SOCKETPAIR)
    list (APPEND FSOCK_SOURCES
        utils/efd_socketpair.h
        utils/efd_socketpair.inc
    )
elseif (FSOCK_HAVE_WINSOCK)
    add_definitions (-DFSOCK_USE_WINSOCK)
    list (APPEND FSOCK_SOURCES
        utils/efd_win.h
        utils/efd_win.inc
    )
else ()
    message (SEND_ERROR "ERROR: could not determine socket signaling method.")
    message (SEND_ERROR "${ISSUE_REPORT_MSG}" )
endif ()

# Provide same folder structure in IDE as on disk
foreach (f ${FSOCK_SOURCES})
    # Get the path of the file relative to source directory
    if (IS_ABSOLUTE "${f}")
        file (RELATIVE_PATH f ${CMAKE_CURRENT_SOURCE_DIR} ${f})
    endif ()
    set (SRC_GROUP "${f}")
    set (f "${CMAKE_CURRENT_SOURCE_DIR}/${f}")

    # Remove the filename part
    string (REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRC_GROUP ${SRC_GROUP})

    # CMake source_group expects \\, not /
    string (REPLACE / \\ SRC_GROUP ${SRC_GROUP})
    source_group ("${SRC_GROUP}" FILES ${f})
endforeach ()

if (FSOCK_STATIC_LIB)
    add_library (${PROJECT_NAME} STATIC ${FSOCK_SOURCES})
    add_definitions (-DFSOCK_STATIC_LIB)
else ()
    add_library (${PROJECT_NAME} SHARED ${FSOCK_SOURCES})
    add_definitions (-DFSOCK_SHARED_LIB)
    set_target_properties (${PROJECT_NAME} PROPERTIES
        VERSION "${FSOCK_PACKAGE_VERSION}"
        SOVERSION "${FSOCK_ABI_VERSION}")
endif ()

# Set library outputs same as top-level project binary outputs
set_target_properties (${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties (${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties (${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_link_libraries (${PROJECT_NAME} ${NN_REQUIRED_LIBRARIES})
if (WIN32)
    target_link_libraries (${PROJECT_NAME} ws2_32)
    target_link_libraries (${PROJECT_NAME} mswsock)
    target_link_libraries (${PROJECT_NAME} advapi32)
elseif (UNIX)
    if(THREADS_HAVE_PTHREAD_ARG)
        add_definitions (-pthread)
    endif()
    if(CMAKE_THREAD_LIBS_INIT)
        target_link_libraries (${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
    endif()
else ()
    message (FATAL_ERROR "Assertion failed; this path is unreachable.")
endif ()

install (TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
