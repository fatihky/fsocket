# Copyright (c) 2013 Luca Barbato
# Copyright 2016 Garrett D'Amore <garrett@damore.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES = @DOLT_CLEANFILES@

EXTRA_DIST =

################################################################################
#  fsocket library                                                             #
################################################################################

fsockincludedir = $(includedir)/fsocket
fsockinclude_HEADERS = \
		src/fsock.h

lib_LTLIBRARIES = libfsocket.la

FSOCK_CORE = \
    src/core/global.h \
    src/core/global.c \
    src/core/sock.h \
    src/core/sock.c

FSOCK_UTILS = \
		src/utils/anet.h \
		src/utils/anet.c \
		src/utils/efd.h \
		src/utils/efd.c \
		src/utils/glock.h \
		src/utils/glock.c \
		src/utils/mutex.h \
		src/utils/mutex.c \
		src/utils/parr.h \
		src/utils/parr.c \
		src/utils/queue.h \
		src/utils/queue.c \
		src/utils/thread.h \
		src/utils/thread.c

libfsocket_la_SOURCES = \
    $(FSOCK_CORE) \
    $(FSOCK_UTILS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfsocket.pc

libfsocket_la_LDFLAGS = -no-undefined -version-info @FSOCK_LIBTOOL_VERSION@
libfsocket_la_CFLAGS = -DFSOCK_EXPORTS

################################################################################
#  documentation                                                               #
################################################################################

#  Source files.

MAN7 = \
    doc/fsocket.txt

MAN3 = \
    doc/fsock_socket.txt

MAN1 = \
    doc/fcat.txt

if DOC

#  Build instructions.

SUFFIXES = .1 .3 .7 .1.html .3.html .7.html
.txt.1:
	$(AM_V_GEN)$(ASCIIDOCTOR) -b manpage -D doc \
	-amanmanual="fsocket @FSOCK_PACKAGE_VERSION@" $<
.txt.3:
	$(AM_V_GEN)$(ASCIIDOCTOR) -b manpage -D doc \
	-amanmanual="fsocket @FSOCK_PACKAGE_VERSION@" $<
.txt.7:
	$(AM_V_GEN)$(ASCIIDOCTOR) -b manpage -D doc \
	-amanmanual="fsocket @FSOCK_PACKAGE_VERSION@" $<

.txt.1.html:
	$(AM_V_GEN)$(ASCIIDOCTOR) -d manpage -b html5 \
        -a stylesheet=$(abs_srcdir)/doc/stylesheet.css -atoc=left \
        -a version-label=$(PACKAGE) -a revnumber=$(PACKAGE_VERSION) \
        -o $@ $<

.txt.3.html:
	$(AM_V_GEN)$(ASCIIDOCTOR) -d manpage -b html5 \
        -a stylesheet=$(abs_srcdir)/doc/stylesheet.css -atoc=left \
        -a version-label=$(PACKAGE) -a revnumber=$(PACKAGE_VERSION) \
        -o $@ $<

.txt.7.html:
	$(AM_V_GEN)$(ASCIIDOCTOR) -d manpage -b html5 \
        -a stylesheet=$(abs_srcdir)/doc/stylesheet.css -atoc=left \
        -a version-label=$(PACKAGE) -a revnumber=$(PACKAGE_VERSION) \
        -o $@ $<

#  Cause man pages to be generated and installed.

dist_man1_MANS = $(MAN1:%.txt=%.1)
dist_man3_MANS = $(MAN3:%.txt=%.3)
dist_man7_MANS = $(MAN7:%.txt=%.7)

#  Cause HTML docs to be generated.

html_DATA = $(MAN1:%.txt=%.1.html) $(MAN3:%.txt=%.3.html) $(MAN7:%.txt=%.7.html)

#  Extra files to be cleaned.

CLEANFILES = $(MAN1:%.txt=%.1) $(MAN3:%.txt=%.3) $(MAN7:%.txt=%.7) \
    $(MAN1:%.txt=%.1.html) $(MAN3:%.txt=%.3.html) $(MAN7:%.txt=%.7.html)

endif

#  Extra files to be included into the source package.

EXTRA_DIST += doc/stylesheet.css $(MAN1) $(MAN3) $(MAN7)

################################################################################
#  performance tests and examples                                              #
################################################################################

noinst_PROGRAMS = \
    perf/perf_ex \
    perf/perf_thr \
    example/example

LDFLAGS += -lframer -lnanomsg -lev

LDADD = libfsocket.la

################################################################################
#  automated tests                                                             #
################################################################################

FEATURE_TESTS = \
    tests/feature1

EXTRA_DIST += tests/testutil.h

check_PROGRAMS = \
    $(FEATURE_TESTS)

TESTS = $(check_PROGRAMS)

################################################################################
#  tools                                                                       #
################################################################################

bin_PROGRAMS =

fcat_SOURCES = \
	tools/fcat.c

if FCAT
bin_PROGRAMS += fcat
endif #FCAT

################################################################################
#  RFCs                                                                        #
################################################################################

EXTRA_DIST += \
    ./rfc/sp-protocol-ids-01.txt \
    ./rfc/sp-tcp-mapping-01.txt \
    ./rfc/sp-udp-mapping-01.txt \
    ./rfc/sp-tls-mapping-01.txt \
    ./rfc/sp-request-reply-01.txt

################################################################################
#  additional packaging-related stuff                                          #
################################################################################

#  Generate ChangeLog file from git.
#  Also, there's no git availabe when building from the source package and
#  thus no way to obtain package version. Therefore, package version is
#  saved into a file when building a source package.
dist-hook:
	$(srcdir)/package_version.sh > $(distdir)/.version


if FCAT
if SYMLINKS

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
          $(LN_S) -f nanocat nn_push$(EXEEXT) && \
          $(LN_S) -f nanocat nn_pull$(EXEEXT) && \
          $(LN_S) -f nanocat nn_pub$(EXEEXT) && \
          $(LN_S) -f nanocat nn_sub$(EXEEXT) && \
          $(LN_S) -f nanocat nn_req$(EXEEXT) && \
          $(LN_S) -f nanocat nn_rep$(EXEEXT) && \
          $(LN_S) -f nanocat nn_surveyor$(EXEEXT) && \
          $(LN_S) -f nanocat nn_respondent$(EXEEXT) && \
          $(LN_S) -f nanocat nn_bus$(EXEEXT) && \
          $(LN_S) -f nanocat nn_pair$(EXEEXT)

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
          rm -f nn_push$(EXEEXT) ; \
          rm -f nn_pull$(EXEEXT) ; \
          rm -f nn_pub$(EXEEXT) ; \
          rm -f nn_sub$(EXEEXT) ; \
          rm -f nn_req$(EXEEXT) ; \
          rm -f nn_rep$(EXEEXT) ; \
          rm -f nn_surveyor$(EXEEXT) ; \
          rm -f nn_respondent$(EXEEXT) ; \
          rm -f nn_bus$(EXEEXT) ; \
          rm -f nn_pair$(EXEEXT)
	cd $(DESTDIR)$(libdir) && \
          rm -f libfsocket.*
	cd $(DESTDIR)$(includedir) && \
          rm -rf fsocket

endif #SYMLINKS
endif #FCAT

EXTRA_DIST += \
    ./abi_version.sh \
    ./package_version.sh \
    ./src/CMakeLists.txt \
    ./CMakeLists.txt

clean-local:
	-rm -f test.ipc test-shutdown.ipc test-separation.ipc test-stress.ipc
