# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2015 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2015 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA


libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

libexec_PROGRAMS = ibus-fbterm

AM_VALAFLAGS = \
        --vapidir=$(top_builddir)/bindings/vala \
        --vapidir=$(top_srcdir)/bindings/vala \
        --pkg=ibus-1.0 \
        --pkg=posix \
        --pkg=config \
        --target-glib="$(VALA_TARGET_GLIB_VERSION)" \
        $(NULL)

ibus_fbterm_SOURCES = \
        fbio.c \
        fbio.h \
        fbkey.c \
        fbkey.h \
        fbshell.c \
        fbshell.h \
        fbshellman.c \
        fbshellman.h \
        fbterm.c \
        fbterm.h \
        fbtty.c \
        fbtty.h \
        ibusfbcontext.c \
        ibusfbcontext.h \
        input_key.h \
        $(NULL)

ibus_fbterm_DEPENDENCIES = \
        $(libibus) \
        $(NULL)

ibus_fbterm_LDADD = \
        $(libibus) \
        @GLIB2_LIBS@ \
        -lutil \
        $(NULL)

ibus_fbterm_CFLAGS = \
        @GLIB2_CFLAGS@ \
        -I$(top_srcdir)/src \
        -I$(top_builddir)/src \
        $(NULL)

$(libibus):
	(cd $(top_builddir)/src; make)

-include $(top_srcdir)/git.mk
