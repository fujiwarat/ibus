/* -*- mode: js2; js2-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ibus - The Input Bus
 *
 * Copyright (c) 2007-2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (c) 2010-2011 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (c) 2007-2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */

const Gtk = imports.gi.Gtk;
// GI_TYPELIB_PATH="/usr/lib64/gnome-shell:/usr/lib64/mutter:/usr/lib64/gnome-bluetooth" gjs mainGtk.js
imports.searchPath.push('/usr/share/ibus/ui/gjs');
imports.searchPath.push('/usr/share/gnome-shell/js');

const Indicator = imports.indicator;

function launch_panel() {
    let _uiapplication = new Indicator.UIApplication(null);
    _uiapplication.connect('disconnected', disconnect_cb);
    _uiapplication.connect('name-lost', name_lost_cb);
    Gtk.main();
}

function disconnect_cb() {
    print('Got disconnected signal from DBus');
    Gtk.main_quit();
}

function name_lost_cb() {
    print('Got NameLost signal from DBus');
    Gtk.main_quit();
}

function print_help() {
    printerr('-h, --help             show this message.');
    printerr('-d, --daemonize        daemonize ibus');
}

function main() {
    let daemonize = false;

    Gtk.init(0, null);
    for (let i = 0; i < ARGV.length; i++) {
        let arg = ARGV[i];
        if (arg == '-h' || arg == '--help') {
            print_help();
            return;
        } else if (arg == '-d' || arg == '--daemonize') {
            daemonize = true
            printerr('Currently fork is not supported');
        } else {
            printerr('Unknown argument: %s', arg);
            return;
        }
    }

    launch_panel();
}

main();
