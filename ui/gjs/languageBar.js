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

const IBus = imports.gi.IBus;
const Lang = imports.lang;
const Signals = imports.signals;

const Menu = imports.menu;

const _ = imports.ui.status.ibus.common._;

function LanguageBar(indicator) {
    this._init(indicator);
}

LanguageBar.prototype = {
    _init: function(indicator) {
        this._enabled = false;
        this._im_name = null;
        this._props = null;

        this._properties = [];
    },

    _remove_properties: function() {
        // reset all properties
        for (let i = 0; i < this._properties.length; i++) {
            this._properties[i].destroy();
        }
        this._properties = [];
    },

    _replace_property: function(old_prop, new_prop) {
        old_prop.set_label(new_prop.get_label());
        old_prop.set_icon(new_prop.get_icon());
        old_prop.set_tooltip(new_prop.get_tooltip());
        old_prop.set_sensitive(new_prop.get_sensitive());
        old_prop.set_visible(new_prop.get_visible());
        old_prop.set_state(new_prop.get_state());
        old_prop.set_sub_props(new_prop.get_sub_props());
    },

    _set_item_icon: function(item, prop) {
        item.set_property('always-show-image', true);
    },

    _on_item_property_activate: function(w, n, s) {
        this.emit('property-activate', n, s);
    },

    _on_item_show_engine_about: function(w, n, s) {
        this.emit('show-engine-about');
    },

    set_im_name: function(text) {
        this._im_name = text
    },

    reset: function() {
        this._remove_properties();
    },

    set_enabled: function(enabled) {
        this._enabled = enabled;
    },

    is_enabled: function() {
        return this._enabled;
    },

    register_properties: function(props) {
        this._props = props;
    },

    update_property: function(prop) {
        if (this._props) {
            for (let i = 0; this._props.get(i) != null; i++) {
                let p = this._props.get(i);
                if (p.get_key() == prop.get_key() && p.get_prop_type() == prop.get_prop_type()) {
                    this._replace_property(p, prop);
                    break;
                }
            }
        }
        for (let i = 0; i < this._properties.length; i++) {
            this._properties[i].update_property(prop);
        }
    },

    create_im_menu: function(menu) {
        if (!this._enabled) {
            return;
        }
        if (!menu) {
            assert (false);
        }
        let props = this._props;
        if (!props) {
            return;
        }

        this._remove_properties();
        let item = new Menu.SeparatorMenuItem();
        item.show();
        this._properties[this._properties.length] = item;
        menu.insert(item.get_raw(), 0);

        let about_label = _("About") + ' - ' + this._im_name;
        let prop = new IBus.Property.new('about',
                                         IBus.PropType.NORMAL,
                                         IBus.Text.new_from_string(about_label),
                                         'help-about',
                                         IBus.Text.new_from_string(_("About the Input Method")),
                                         true, true,
                                         IBus.PropState.UNCHECKED,
                                         null);
        item = new Menu.ImageMenuItem(prop = prop);
        item.set_property('always-show-image', true);
        item.set_no_show_all(true);
        item.show();
        this._properties[this._properties.length] = item;
        menu.insert(item.get_raw(), 0);
        item.connect('property-activate',
                     Lang.bind(this, this._on_item_show_engine_about));

        let i;
        for (i = 0; props.get(i) != null; i++) {
            prop = props.get(i);
        }
        let length = i;
        let radio_group = [];

        for (i = length - 1; i >= 0; i-=1) {
            prop = props.get(i);
            if (prop.get_prop_type() == IBus.PropType.NORMAL) {
                item = new Menu.ImageMenuItem(prop = prop);
                this._set_item_icon(item, prop);
            }
            else if (prop.get_prop_type() == IBus.PropType.TOGGLE) {
                item = new Menu.CheckMenuItem(prop = prop);
            }
            else if (prop.get_prop_type() == IBus.PropType.RADIO) {
                item = new Menu.RadioMenuItem(radio_group, prop = prop);
                radio_group[radio_group.length] = item;
            }
            else if (prop.get_prop_type() == IBus.PropType.SEPARATOR) {
                item = new Menu.SeparatorMenuItem();
                radio_group = null;
            }
            else if (prop.get_prop_type() == IBus.PropType.MENU) {
                item = new Menu.ImageMenuItem(prop = prop);
                this._set_item_icon(item, prop);
                let submenu = new Menu.Menu(prop);
                item.set_submenu(submenu.get_raw());
                submenu.connect('property-activate',
                                Lang.bind(this, this._on_item_property_activate));
            }
            else {
                IBusException('Unknown property type = %d' % prop.get_prop_type());
            }

            item.set_sensitive(prop.get_sensitive());

            item.set_no_show_all(true);

            if (prop.get_visible()) {
                item.show();
            } else {
                item.hide();
            }

            this._properties[this._properties.length] = item;
            menu.insert(item.get_raw(), 0);
            item.connect('property-activate',
                         Lang.bind(this, this._on_item_property_activate));
        }
    },
};

Signals.addSignalMethods(LanguageBar.prototype);
