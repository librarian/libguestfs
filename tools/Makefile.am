# libguestfs virt-* tools
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

tools = \
	list-filesystems \
	list-partitions \
	make-fs \
	tar \
	win-reg

EXTRA_DIST = \
	$(tools:%=virt-%) \
	test-virt-*.sh

if HAVE_TOOLS

bin_SCRIPTS = $(tools:%=virt-%)

# Manual pages and HTML files for the website.

# XXX Bug in automake?  If you list virt-tar.1 explicitly, then it
# builds and installs the man pages.  However if this is removed,
# then the man pages are neither built nor installed.
man_MANS = virt-tar.1 $(patsubst %,virt-%.1,$(filter-out tar,$(tools)))

noinst_DATA = $(tools:%=$(top_builddir)/html/virt-%.1.html)

virt-%.1: virt-%
	$(PODWRAPPER) \
	  --man $@ \
	  --license GPLv2+ \
	  $<

$(top_builddir)/html/virt-%.1.html: virt-%
	$(PODWRAPPER) \
	  --html $@ \
	  --license GPLv2+ \
	  $<

# Tests.

TESTS_ENVIRONMENT = $(top_builddir)/run --test

if ENABLE_APPLIANCE
TESTS = test-virt-list-filesystems.sh \
	test-virt-make-fs.sh \
	test-virt-tar.sh
endif ENABLE_APPLIANCE

endif

# Make symlinks from virt-foo.pl to virt-foo.  This is just to keep
# xgettext happy since it uses the file extension to determine the
# implementation language of a file.

all-local: $(tools:%=virt-%.pl)

virt-%.pl: virt-%
	ln -sf $< $@
