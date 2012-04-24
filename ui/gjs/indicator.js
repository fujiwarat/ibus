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

const Gio = imports.gi.Gio;
const IBus = imports.gi.IBus;
const DBus = imports.dbus;
const Lang = imports.lang;
const Signals = imports.signals;

const Panel = imports.panel;

const UIApplicationIface = {
    name: IBus.SERVICE_PANEL,
    methods: [],
    signals: [{ name: 'NameOwnerChanged',
                inSignature: 'sss',
                outSignature: ''
              },
              { name: 'NameLost',
                inSignature: 's',
                outSignature: ''
              },
              { name: 'NameAcquired',
                inSignature: 's',
                outSignature: ''
              }],
    properties: []
};

function UIApplication(indicator) {
    this._init(indicator);
}

UIApplication.prototype = {
    _init: function(indicator) {
        IBus.init();
        this._bus = new IBus.Bus();
        DBus.session.exportObject('/org/freedesktop/IBus/Panel',
                                  this);

        if (this._bus.is_connected() == false) {
            printerr('ibus-daemon is not running');
            return;
        }
        this._bus.connect('disconnected',
                          Lang.bind(this, this._disconnect_cb));
        let match_rule = "type='signal',\
                         sender='org.freedesktop.IBus',\
                         path='/org/freedesktop/IBus'";
        this._bus.add_match(match_rule);
        match_rule = "type='signal',\
                     sender='org.freedesktop.IBus',\
                     member='NameLost',\
                     arg0='" + IBus.SERVICE_PANEL + "'";
        this._bus.add_match(match_rule);
        this._bus.request_name(IBus.SERVICE_PANEL,
                               IBus.BusNameFlag.ALLOW_REPLACEMENT |
                               IBus.BusNameFlag.REPLACE_EXISTING);
        if (this._bus.is_connected() == false) {
            printerr('RequestName ' + IBus.SERVICE_PANEL + ' is time out.');
            return;
        }
        this._panel = new Panel.Panel(this._bus, indicator);

        this._bus.get_connection().signal_subscribe('org.freedesktop.DBus',
                                                    'org.freedesktop.DBus',
                                                    'NameLost',
                                                    '/org/freedesktop/DBus',
                                                    IBus.SERVICE_PANEL,
                                                    Gio.DBusSignalFlags.NONE,
                                                    Lang.bind(this, this._name_lost_cb),
                                                    null,
                                                    null);
    },
    _disconnect_cb: function() {
        this.emit('disconnected');
    },
    _name_lost_cb: function() {
        this.emit('name-lost');
    },
};

Signals.addSignalMethods(UIApplication.prototype);
DBus.conformExport(UIApplication.prototype, UIApplicationIface);
