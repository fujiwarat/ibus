# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA
SUBDIRS = \
	interface \
	$(NULL)

ibus_PYTHON = \
	ascii.py \
	application.py \
	attribute.py \
	common.py \
	bus.py \
	inputcontext.py \
	engine.py \
	exception.py \
	factory.py \
	_gtk.py \
	__init__.py \
	keysyms.py \
	lang.py \
	lookuptable.py \
	modifier.py \
	object.py \
	panel.py \
	notifications.py \
	property.py \
	serializable.py \
	text.py \
	component.py \
	enginedesc.py \
	observedpath.py \
	utility.py \
	config.py \
	$(NULL)

nodist_ibus_PYTHON = \
	_config.py \
	$(NULL)

ibusdir = @pkgpythondir@

xkblayout_py_in_files = \
	xkblayout.py.in \
	xkbxml.py.in \
	$(NULL)
xkblayout_py_DATA = $(xkblayout_py_in_files:.py.in=.py)
xkblayout_pydir = @pkgpythondir@

ibus_PYTHON += $(xkblayout_py_DATA)

if ENABLE_XKB
XKB_COMMAND=\\\"$(libexecdir)/ibus-xkb\\\"
HAVE_XKB=True
else
XKB_COMMAND="None"
HAVE_XKB=False
endif

%.py : %.py.in
	@sed -e "s|\@XKB_COMMAND\@|$(XKB_COMMAND)|g" \
	     -e "s|\@XKB_RULES_XML_FILE\@|$(XKB_RULES_XML_FILE)|g" \
	     -e "s|\@HAVE_XKB\@|$(HAVE_XKB)|g" \
	     -e "s|\@datadir\@|$(datadir)|g" \
	$< > $@

EXTRA_DIST = \
	_config.py.in \
	$(xkblayout_py_in_files) \
	$(NULL)

CLEANFILES = \
	*.pyc \
	$(xkblayout_py_DATA) \
	$(NULL)

DISTCLEANFILES = \
	_config.py \
	$(NULL)
