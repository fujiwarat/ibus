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

const Clutter = imports.gi.Clutter;
const St = imports.gi.St;
const IBus = imports.gi.IBus;
const Lang = imports.lang;

const ModalDialog = imports.ui.modalDialog;

const Common = imports.ui.status.ibus.common;
const _ = imports.ui.status.ibus.common._;

function ShellEngineAbout(enginedesc) {
    this._init(enginedesc);
}

ShellEngineAbout.prototype = {
    __proto__: ModalDialog.ModalDialog.prototype,

    _init : function(enginedesc) {
        ModalDialog.ModalDialog.prototype._init.call(this);
        this._engine_desc = enginedesc;
        this._init_ui();
    },

    _init_ui: function() {
        let mainContentLayout = new St.BoxLayout({ vertical: false });
        this.contentLayout.add(mainContentLayout,
                               { x_fill: true,
                                 y_fill: false });

        this._iconBin = new St.Bin();
        mainContentLayout.add(this._iconBin,
                              { x_fill:  true,
                                y_fill:  false,
                                x_align: St.Align.END,
                                y_align: St.Align.START });

        let messageLayout = new St.BoxLayout({ vertical: true });
        mainContentLayout.add(messageLayout,
                              { y_align: St.Align.START });

        let label = new St.Label({ style_class: 'run-dialog-label',
                                   text: '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        let icon = new St.Icon({ icon_name: this._engine_desc.get_icon(),
                                 icon_type: St.IconType.SYMBOLIC,
                                 icon_size: 24 });
        messageLayout.add(icon, { expand: true, x_align: St.Align.START,
                                  x_fill: false, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: '\n' +
                                     this._engine_desc.get_longname() +
                                     '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        let lang = IBus.get_language_name(this._engine_desc.get_language());
        if (lang == null) {
            lang = _("Other");
        }
        label = new St.Label({ style_class: 'run-dialog-label',
                               text: _("Language: ") +
                                     lang +
                                     '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: _("Keyboard layout: ") +
                                     this._engine_desc.get_layout() +
                                     '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: _("Author: ") +
                                     this._engine_desc.get_author() +
                                     '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: _("Description:\n") });
        messageLayout.add(label, { expand: true, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: this._engine_desc.get_description() });
        messageLayout.add(label, { expand: true, y_fill: false });

        this.setButtons([{ label: _("Close"),
                           action: Lang.bind(this, this._close),
                           key:    Clutter.Return
                         }]);
        this._actionKeys[Clutter.Escape] = Lang.bind(this, this._close);
    },

    _close: function() {
        this.close(global.get_current_time());
    },
};

function ShellPanelAbout() {
    this._init();
}

ShellPanelAbout.prototype = {
    __proto__: ModalDialog.ModalDialog.prototype,

    _init : function() {
        ModalDialog.ModalDialog.prototype._init.call(this);
        this._init_ui();
    },

    _init_ui: function() {
        let mainContentLayout = new St.BoxLayout({ vertical: false });
        this.contentLayout.add(mainContentLayout,
                               { x_fill: true,
                                 y_fill: false });

        this._iconBin = new St.Bin();
        mainContentLayout.add(this._iconBin,
                              { x_fill:  true,
                                y_fill:  false,
                                x_align: St.Align.END,
                                y_align: St.Align.START });

        let messageLayout = new St.BoxLayout({ vertical: true });
        mainContentLayout.add(messageLayout,
                              { y_align: St.Align.START });

        let label = new St.Label({ style_class: 'run-dialog-label',
                                   text: '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        let icon = new St.Icon({ icon_name: 'ibus',
                                 icon_type: St.IconType.SYMBOLIC,
                                 icon_size: 24 });

        messageLayout.add(icon, { expand: true, x_fill: false, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: '\n' +
                                     'IBus ' +
                                     Common.get_version() +
                                     '\n' });
        messageLayout.add(label, { expand: true, x_align: St.Align.MIDDLE,
                                   x_fill: false, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: _("IBus is an intelligent input bus for Linux/Unix.") +
                                     '\n' });
        messageLayout.add(label, { expand: true, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: Common.get_copyright() +
                                     '\n' });
        messageLayout.add(label, { expand: true, x_align: St.Align.MIDDLE,
                                   x_fill: false, y_fill: false });

        label = new St.Label({ style_class: 'run-dialog-label',
                               text: 'http://code.google.com/p/ibus' +
                                     '\n' });
        messageLayout.add(label, { expand: true, x_align: St.Align.MIDDLE,
                                   x_fill: false, y_fill: false });

        this.setButtons([{ label: _("Close"),
                           action: Lang.bind(this, this._close),
                           key:    Clutter.Return
                         }]);
        this._actionKeys[Clutter.Escape] = Lang.bind(this, this._close);
    },

    _close: function() {
        this.close(global.get_current_time());
    },
};
