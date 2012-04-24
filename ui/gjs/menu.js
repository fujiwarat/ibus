/* -*- mode: js2; js2-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright 2010-2011 Red Hat, Inc.
 * Copyright 2010-2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright 2010-2011 Takao Fujiwara <tfujiwar@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;
const IBus = imports.gi.IBus;
const Lang = imports.lang;
const Signals = imports.signals;

const PropItem = imports.ui.status.ibus.propItem;

function Menu(prop) {
    this._init(prop);
}

Menu.prototype = {
    __proto__ : PropItem.PropItem.prototype,

    _init: function(prop) {
        PropItem.PropItem.prototype._init.call(this, prop);

        this._menu = new Gtk.Menu();
        this._menu.set_take_focus(false);
        this._create_items(this._prop.get_sub_props());
        this._menu.show_all();
        this._menu.set_sensitive(prop.get_sensitive());
    },

    _create_items: function(props) {
        let radio_group = [];
        let item = null;

        for (let i = 0; props.get(i) != null; i++) {
            let prop = props.get(i);
            if (prop.get_prop_type() == IBus.PropType.NORMAL) {
                item = new ImageMenuItem(prop);
            } else if (prop.get_prop_type() == IBus.PropType.TOGGLE) {
                item = new CheckMenuItem(prop);
            } else if (prop.get_prop_type() == IBus.PropType.RADIO) {
                item = new RadioMenuItem(radio_group, prop);
                radio_group[radio_group.length] = item;
            } else if (prop.get_prop_type() == IBus.PropType.SEPARATOR) {
                item = new SeparatorMenuItem();
                radio_group = [];
            } else if (prop.get_prop_type() == IBus.PropType.MENU) {
                item = new ImageMenuItem(prop);
                if (prop.get_icon()) {
                    let width = [];
                    let height = [];
                    Gtk.icon_size_lookup(Gtk.IconSize.MENU, width, height);
                    //item.set_image(icon.IconWidget(prop.get_icon(), width[0]));
                }
                if (prop.get_label()) {
                    item.set_label(prop.get_label().get_text());
                }
                item.set_submenu(new Menu(prop));
            } else {
                assert (false);
            }

            if (prop.get_tooltip()) {
                item.set_tooltip_text(prop.get_tooltip().get_text());
            }
            item.set_sensitive(prop.get_sensitive());
            if (prop.get_visible()) {
                item.set_no_show_all(false);
                item.show()
            } else {
                item.set_no_show_all(true);
                item.hide()
            }

            this._menu.append(item.get_raw());
            this._sub_items[this._sub_items.length] = item;

            if (prop.get_prop_type() != IBus.PropType.NORMAL &&
                prop.get_prop_type() != IBus.PropType.TOGGLE &&
                prop.get_prop_type() != IBus.PropType.RADIO) {
                continue;
            }
            item.connect('property-activate',
                         Lang.bind(this, this._on_item_property_activate));
        }
    },

    _property_clicked: function(item, prop) {
    },

    _on_item_property_activate: function (w, n, s) {
        this.emit('property-activate', n, s);
    },

    popup: function(button, active_time, widget) {
        Gtk.Menu.popup(this, null, null, menu_position,
                            button, active_time, widget);
    },

    get_raw: function() {
        return this._menu;
    },
};

Signals.addSignalMethods(Menu.prototype);

function ImageMenuItem(prop) {
    this._init(prop);
}

ImageMenuItem.prototype = {
    __proto__ : PropItem.PropItem.prototype,

    _init: function(prop) {
        PropItem.PropItem.prototype._init.call(this, prop);
        this._item = new Gtk.ImageMenuItem();
        this._item.connect('activate', Lang.bind(this, this._on_activate));

        if (this._prop.get_icon()) {
            let width = [];
            let height = [];
            Gtk.icon_size_lookup(Gtk.IconSize.MENU, width, height);
            //this._item.set_image(icon.IconWidget(prop.get_icon(), width[0]));
        }
        if (this._prop.get_label()) {
            this._item.set_label(prop.get_label().get_text());
        }

        if (this._prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide();
        }
    },

    _on_activate: function() {
        this.emit('property-activate', this._prop.get_key(), this._prop.get_state());
    },

    property_changed: function() {
        this._item.set_sensitive(this._prop.get_sensitive());
        if (this._prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide();
        }
    },

    get_raw: function() {
        return this._item;
    },

    show: function() {
        this._item.show();
    },

    hide: function() {
        this._item.hide();
    },

    destroy: function() {
        this._item.destroy();
    },

    set_property: function(key, value) {
        this._item.set_property(key, value);
    },

    set_no_show_all: function(flag) {
        this._item.set_no_show_all(flag);
    },

    set_sensitive: function(flag) {
        this._item.set_sensitive(flag);
    },

    set_tooltip_text: function(text) {
        this._item.set_tooltip_text(text);
    },

    set_submenu: function(submenu) {
        this._item.set_submenu(submenu);
    },

    set_label: function(label) {
        this._item.set_label(label);
    },
};

Signals.addSignalMethods(ImageMenuItem.prototype);

function CheckMenuItem(prop) {
    this._init(prop);
}

CheckMenuItem.prototype = {
    __proto__ : PropItem.PropItem.prototype,

    _init: function(prop) {
        PropItem.PropItem.prototype._init.call(this, prop);
        this._item = new Gtk.CheckMenuItem.new_with_label(label=prop.get_label().get_text());

        this._item.set_active(this._prop.get_state() == IBus.PropState.CHECKED);
        this._item.connect('activate', Lang.bind(this, this._on_activate));

        if (this._prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide_all();
        }
    },

    _on_activate: function() {
        // Do not send property-activate to engine in case the event is
        // sent from engine.
        let do_emit = false;
        if (this._item.get_active()) {
            if (this._prop.get_state() != IBus.PropState.CHECKED) {
                do_emit = true;
            }
            this._prop.set_state(IBus.PropState.CHECKED);
        } else {
            if (this._prop.get_state() != IBus.PropState.UNCHECKED) {
                do_emit = true;
            }
            this._prop.set_state(IBus.PropState.UNCHECKED);
        }
        if (do_emit) {
            this.emit('property-activate', this._prop.get_key(), this._prop.get_state());
        }
    },

    property_changed: function() {
        this._item.set_active(this._prop.get_state() == IBus.PropState.CHECKED);
        this._item.set_sensitive(this._prop.sensitive);
        if (this._prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide_all();
        }
    },

    get_raw: function() {
        return this._item;
    },

    show: function() {
        this._item.show();
    },

    hide: function() {
        this._item.hide();
    },

    destroy: function() {
        this._item.destroy();
    },

    set_property: function(key, value) {
        this._item.set_property(key, value);
    },

    set_no_show_all: function(flag) {
        this._item.set_no_show_all(flag);
    },

    set_sensitive: function(flag) {
        this._item.set_sensitive(flag);
    },

    set_tooltip_text: function(text) {
        this._item.set_tooltip_text(text);
    },

    set_submenu: function(submenu) {
        this._item.set_submenu(submenu);
    },

    set_label: function(label) {
        this._item.set_label(label);
    },
};

Signals.addSignalMethods(CheckMenuItem.prototype);

function RadioMenuItem(group, prop) {
    this._init(group, prop);
}

RadioMenuItem.prototype = {
    __proto__ : PropItem.PropItem.prototype,

    _init: function(group, prop) {
        PropItem.PropItem.prototype._init.call(this, prop);
        let raw_group = this._get_raw_group(group);
        this._item = new Gtk.RadioMenuItem.new_with_label(raw_group,
                                                          prop.get_label().get_text());

        this._item.set_active(this._prop.get_state() == IBus.PropState.CHECKED);
        this._item.connect('activate', Lang.bind(this, this._on_activate));

        if (prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide_all();
        }
    },

    _get_raw_group: function(group) {
        let raw_group = [];
        for (let i = 0; i < group.length; i++) {
                raw_group[raw_group.length] = group[i].get_raw();
        }
        return raw_group;
    },

    _on_activate: function() {
        // Do not send property-activate to engine in case the event is
        // sent from engine.
        let do_emit = false;
        if (this._item.get_active()) {
            if (this._prop.get_state() != IBus.PropState.CHECKED) {
                do_emit = true;
            }
            this._prop.set_state(IBus.PropState.CHECKED);
        } else {
            if (this._prop.get_state() != IBus.PropState.UNCHECKED) {
                do_emit = true;
            }
            this._prop.set_state(IBus.PropState.UNCHECKED);
        }
        if (do_emit) {
            this.emit('property-activate', this._prop.get_key(), this._prop.get_state());
        }
    },

    property_changed: function() {
        this._item.set_active(this._prop.get_state() == IBus.PropState.CHECKED);
        this._item.set_sensitive(this._prop.get_sensitive());
        if (this._prop.get_visible()) {
            this._item.set_no_show_all(false);
            this._item.show_all();
        } else {
            this._item.set_no_show_all(true);
            this._item.hide_all()
        }
    },

    get_raw: function() {
        return this._item;
    },

    show: function() {
        this._item.show();
    },

    hide: function() {
        this._item.hide();
    },

    destroy: function() {
        this._item.destroy();
    },

    set_property: function(key, value) {
        this._item.set_property(key, value);
    },

    set_no_show_all: function(flag) {
        this._item.set_no_show_all(flag);
    },

    set_sensitive: function(flag) {
        this._item.set_sensitive(flag);
    },

    set_tooltip_text: function(text) {
        this._item.set_tooltip_text(text);
    },

    set_submenu: function(submenu) {
        this._item.set_submenu(submenu);
    },

    set_label: function(label) {
        this._item.set_label(label);
    },
};

Signals.addSignalMethods(RadioMenuItem.prototype);

function SeparatorMenuItem() {
    this._init();
}

SeparatorMenuItem.prototype = {
    __proto__ : PropItem.PropItem.prototype,

    _init: function() {
        PropItem.PropItem.prototype._init.call(this, null);
        this._item = new Gtk.SeparatorMenuItem();
    },

    get_raw: function() {
        return this._item;
    },

    show: function() {
        this._item.show();
    },

    destroy: function() {
        this._item.destroy();
    },
};

Signals.addSignalMethods(SeparatorMenuItem.prototype);

function menu_position(menu, button) {
    let screen = button.get_screen();
    let monitor = screen.get_monitor_at_window(button.window);
    let monitor_allocation = screen.get_monitor_geometry(monitor);

    let x, y = button.window.get_origin();
    x += button.allocation.x;
    y += button.allocation.y;

    let menu_width, menu_height = menu.size_request();

    if (x + menu_width >= monitor_allocation.width) {
        x -= menu_width - button.allocation.width;
    } else if (x - menu_width <= 0) {
    } else {
        if (x <= monitor_allocation.width * 3 / 4) {
        } else {
            x -= menu_width - button.allocation.width;
        }
    }

    if (y + button.allocation.height + menu_height >= monitor_allocation.height) {
        y -= menu_height;
    } else if (y - menu_height <= 0) {
        y += button.allocation.height;
    } else {
        if (y <= monitor_allocation.height * 3 / 4) {
            y += button.allocation.height;
        } else {
            y -= menu_height;
        }
    }

    return (x, y, false);
}
