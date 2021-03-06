# libguestfs
# Copyright (C) 2009-2013 Red Hat Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

include $(top_srcdir)/subdir-rules.mk

TESTS = \
	rhbz501893 \
	rhbz503169c13.sh \
	rhbz557655.sh \
	rhbz576879.sh \
	rhbz578407.sh \
	rhbz580246.sh \
	rhbz602997.sh \
	rhbz690819.sh \
	rhbz789960.sh \
	rhbz790721 \
	rhbz811649.sh \
	rhbz895904.sh \
	rhbz914931 \
	rhbz957772.sh \
	rhbz975797.sh \
	test-noexec-stack.pl

tests_not_run = \
	rhbz727178.sh \
	rhbz909624.sh

TESTS_ENVIRONMENT = \
	NOEXEC_CHECK="$(top_builddir)/src/.libs/libguestfs.so $(top_builddir)/daemon/guestfsd" \
	$(top_builddir)/run --test

check_PROGRAMS = \
	rhbz501893 \
	rhbz790721 \
	rhbz914931

rhbz501893_SOURCES = rhbz501893.c
rhbz501893_CPPFLAGS = \
	-I$(top_srcdir)/src -I$(top_builddir)/src
rhbz501893_CFLAGS = \
	$(WARN_CFLAGS) $(WERROR_CFLAGS) \
	$(GPROF_CFLAGS) $(GCOV_CFLAGS)
rhbz501893_LDADD = \
	$(top_builddir)/src/libguestfs.la

rhbz790721_SOURCES = rhbz790721.c
rhbz790721_CPPFLAGS = \
	-I$(top_srcdir)/src -I$(top_builddir)/src
rhbz790721_CFLAGS = \
	-pthread \
	$(WARN_CFLAGS) $(WERROR_CFLAGS) \
	$(GPROF_CFLAGS) $(GCOV_CFLAGS)
rhbz790721_LDADD = \
	$(top_builddir)/src/libguestfs.la

rhbz914931_SOURCES = rhbz914931.c
rhbz914931_CPPFLAGS = \
	-I$(top_srcdir)/src -I$(top_builddir)/src \
	-DGUESTFS_PRIVATE=1
rhbz914931_CFLAGS = \
	-pthread \
	$(WARN_CFLAGS) $(WERROR_CFLAGS) \
	$(GPROF_CFLAGS) $(GCOV_CFLAGS)
rhbz914931_LDADD = \
	$(top_builddir)/src/libguestfs.la

EXTRA_DIST = \
	$(TESTS) \
	$(tests_not_run) \
	rhbz557655-expected.stdout \
	rhbz557655-expected.stderr

check-slow:
	$(MAKE) TESTS="rhbz909624.sh" check
