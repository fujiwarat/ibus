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
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const IBus = imports.gi.IBus;
const Lang = imports.lang;
const Util = imports.misc.util;
const IBUS_PREFIX = imports.misc.config.IBUS_PREFIX;
const IBUS_PKGDATADIR = imports.misc.config.IBUS_PKGDATADIR;

const PanelBase = imports.ui.status.ibus.panelBase;

const common = imports.ui.status.ibus.common;
const _ = imports.ui.status.ibus.common._;

const CandidatePanel = imports.candidatePanel;
const EngineAbout = imports.engineAbout;
const LanguageBar = imports.languageBar;

const ICON_KEYBOARD = 'input-keyboard-symbolic';
const ICON_ENGINE = 'ibus-engine';
const LIGHTBOX_FADE_TIME = 0.1


function Panel(bus, indicator) {
    this._init(bus, indicator);
}

Panel.prototype = {
    __proto__ : PanelBase.PanelBase.prototype,

    _init: function(bus, indicator) {
        this._bus = bus;
        this._indicator = indicator;
        this._config = this._bus.get_config();
        this._focus_ic = null;
        this._setup_pid = -1;
        let prefix = IBUS_PREFIX;
        let data_dir = IBUS_PKGDATADIR;
        // this._icons_dir = data_dir + '/icons';
        this._setup_cmd = prefix + '/bin/ibus-setup';
        this._active_engine = null;
        this._callback_after_focus_in = null;

        // connect bus signal
        if (this._config == null) {
            printerr ('Could not get ibus-gconf.');
            return;
        }

        PanelBase.PanelBase.prototype._init.call(this, bus, ICON_KEYBOARD);

        this._config.connect('value-changed', this._config_value_changed_cb)
        //this._config.connect('reloaded', this._config_reloaded_cb)
        this._bus.get_connection().signal_subscribe('org.freedesktop.DBus',
                                                    'org.freedesktop.DBus',
                                                    'NameOwnerChanged',
                                                    '/org/freedesktop/DBus',
                                                    IBus.SERVICE_PANEL,
                                                    Gio.DBusSignalFlags.NONE,
                                                    this._name_owner_changed_cb,
                                                    null,
                                                    null);
        // this._bus.config_add_watch('panel')

        // add icon search path
        // let icon_theme = Gtk.icon_theme_get_default();
        // icon_theme.prepend_search_path(self._icons_dir)

        this._language_bar = new LanguageBar.LanguageBar(indicator);
        this._language_bar.connect('property-activate',
                                   Lang.bind(this, this._on_language_bar_property_activate));
        this._language_bar.connect('show-engine-about',
                                   Lang.bind(this, this._on_language_bar_show_engine_about));

        this._candidate_panel = new CandidatePanel.CandidatePanel();
        this._candidate_panel.connect('cursor-up',
                                      Lang.bind(this, function(widget) {
                                          this.cursor_up();}));
        this._candidate_panel.connect('cursor-down',
                                      Lang.bind(this, function(widget) {
                                          this.cursor_down();}));
        this._candidate_panel.connect('page-up',
                                      Lang.bind(this, function(widget) {
                                          this.page_up();}));
        this._candidate_panel.connect('page-down',
                                      Lang.bind(this, function(widget) {
                                          this.page_down();}));
        this._candidate_panel.connect('candidate-clicked',
                                      Lang.bind(this, function(widget, index, button, state) {
                                          this.candidate_clicked(index, button, state);}));

        if (this._indicator != null) {
            this._indicator.setIcon(ICON_KEYBOARD);
        } else {
            this._status_icon = new Gtk.StatusIcon();
            this._status_icon.connect('popup-menu', Lang.bind(this, this._status_icon_popup_menu_cb));
            this._status_icon.connect('activate', Lang.bind(this, this._status_icon_activate_cb));
            this._status_icon.set_from_icon_name(ICON_KEYBOARD);
            this._status_icon.set_tooltip_text(_("IBus input method framework"));
            this._status_icon.set_visible(true);
        }
    },

    _config_value_changed_cb: function(bus, section, name, value) {
        if (section != 'panel') {
            return;
        }
        if (name == 'lookup_table_orientation') {
            return;
        }
    },

    _config_reloaded_cb: function(bus) {
    },

    _name_owner_changed_cb: function(bus, name, oldname, newname) {
        this._config_reloaded_cb(bus);
    },

    _on_language_bar_property_activate: function(widget, prop_name, prop_state) {
        this.property_activate(prop_name, prop_state);
    },

    _run_engine_about_dialog: function() {
        if (this._focus_ic == null) {
            return;
        }

        try {
            let engine = this._focus_ic.get_engine();
            let dlg = new EngineAbout.EngineAbout(engine);
            dlg.run();
            dlg.destroy();
        } catch(e) {
        }
    },

    _on_language_bar_show_engine_about: function(languagebar) {
        if (this._focus_ic == null) {
            this._callback_after_focus_in = this._run_engine_about_dialog;
            return;
        }
        this._run_engine_about_dialog();
    },

    _create_sys_menu: function() {
        let menu = new Gtk.Menu();
        let item = Gtk.ImageMenuItem.new_from_stock(Gtk.STOCK_PREFERENCES, null);
        item.connect('activate', Lang.bind(this, this._preferences_item_status_activate_cb));
        menu.add(item);
        item = Gtk.ImageMenuItem.new_from_stock(Gtk.STOCK_ABOUT, null);
        item.connect('activate', Lang.bind(this, this._about_item_status_activate_cb));
        menu.add(item);
        menu.add(new Gtk.SeparatorMenuItem());
        item = Gtk.MenuItem.new_with_label(_("Restart"));
        item.connect('activate', Lang.bind(this, this._restart_item_status_activate_cb));
        menu.add(item);
        item = Gtk.ImageMenuItem.new_from_stock(Gtk.STOCK_QUIT, null);
        item.connect('activate', Lang.bind(this, this._quit_item_status_activate_cb));
        menu.add(item);
        menu.show_all();
        menu.set_take_focus(false);
        return menu;
    },

    _create_im_menu_gtk: function() {
        let engines = this._bus.list_active_engines();
        let current_engine = null;
        current_engine = (this._focus_ic != null && this._focus_ic.get_engine());
        if (current_engine == null) {
            current_engine = (engines && engines[0]);
        }
        let width = [];
        let height = [];
        Gtk.icon_size_lookup(Gtk.IconSize.MENU, width, height);
        let menu = new Gtk.Menu();
        for (let i = 0; i < engines.length; i++) {
            let engine = engines[i];
            if (engine == null) {
                continue;
            }
            let lang = IBus.get_language_name(engine.language);
            if (lang == null) {
                lang = _("Other");
            }
            let item = Gtk.ImageMenuItem.new_with_label(engine.language + ' ' + lang + '(' + engine.longname + ')');
            if (current_engine != null && current_engine.name == engine.name) {
                let children = item.get_children();
                for (let j = 0; j < children.length; j++) {
                    let widget = children[j];
                    widget.set_markup('<b>' + widget.get_text() + '</b>');
                }
            }
            if (engine.icon != null) {
                //item.set_image(_icon.IconWidget(engine.icon, height[0]));
            } else {
                //item.set_image(_icon.IconWidget(ICON_ENGINE, height[0]));
            }
            item._engine = engine;
            item.connect('activate', Lang.bind(this, this._im_menu_item_status_activate_cb));
            menu.add(item);
        }
        let item = Gtk.ImageMenuItem.new_with_label(_("Turn off input method"));
        //item.set_image(_icon.IconWidget('window-close', height[0]));
        item._engine = null;
        item.connect('activate', Lang.bind(this, this._im_menu_item_status_activate_cb));
        if (this._focus_ic == null || !this._focus_ic.is_enabled()) {
            item.set_sensitive(false);
        }
        menu.add(item);
        menu.show_all();
        menu.set_take_focus(false);
        return menu;
    },

    _status_icon_popup_menu_cb: function(status_icon, button, active_time) {
        let menu = this._create_sys_menu();
        menu.popup_for_device(null, null, null,
                              Gtk.StatusIcon.position_menu,
                              this._status_icon,
                              null,
                              button,
                              active_time);
    },

    _status_icon_activate_cb: function(status_icon) {
        let menu = null;
        if (this._focus_ic == null) {
            menu = new Gtk.Menu();
            let item = Gtk.ImageMenuItem.new_with_label(_("No input window"));
            let width = [];
            let height = [];
            Gtk.icon_size_lookup(Gtk.IconSize.MENU, width, height);
            //item.set_image(_icon.IconWidget('dialog-information', height));
            menu.add(item);
            menu.show_all();
        } else {
            menu = this._create_im_menu_gtk();
            this._language_bar.create_im_menu(menu);
        }
        menu.popup_for_device(null, null, null,
                              Gtk.StatusIcon.position_menu,
                              this._status_icon,
                              null,
                              0,
                              Gtk.get_current_event_time());
    },

    _im_menu_item_status_activate_cb: function(item) {
        /* this._focus_ic is null on gnome-shell because focus-in event is 
         * happened. So I moved set_engine in focus_in. */
        if (this._focus_ic == null) {
            if (item._engine != null) {
                this._active_engine = item._engine;
            }
            return;
        }
        if (item._engine != null) {
            this._focus_ic.set_engine(item._engine.name);
        } else {
            this._focus_ic.disable();
        }
    },

    _im_menu_item_shell_activate_cb: function(item, event) {
        this._im_menu_item_status_activate_cb(item);
    },

    _child_watch_cb: function(pid, status, data) {
        if (this._setup_pid == pid) {
            this._setup_pid = -1;
        }
    },

    _preferences_item_status_activate_cb: function(item, user_data) {
        if (this._setup_pid != -1) {
            try {
                Util.trySpawnCommandLine('kill -10 ' + this._setup_pid.toString());
                return;
            } catch (e) {
                this._setup_pid = -1;
            }
        }
        let pid = GLib.spawn_async(null,
                                   [this._setup_cmd, 'ibus-setup'],
                                   null,
                                   GLib.SpawnFlags.DO_NOT_REAP_CHILD, null,
                                   null)[1];
        this._setup_pid = pid;
        GLib.child_watch_add(0, this._setup_pid,
                             Lang.bind(this, this._child_watch_cb), null);
    },

    _preferences_item_shell_activate_cb: function(item, event, user_data) {
        this._preferences_item_status_activate_cb(item, user_data);
    },

    _run_panel_about_gtk: function() {
        let about_dialog = new Gtk.AboutDialog();
        about_dialog.set_program_name('IBus');
        about_dialog.set_version(common.get_version());
        about_dialog.set_copyright(common.get_copyright());
        about_dialog.set_license(common.get_license());
        about_dialog.set_comments(_("IBus is an intelligent input bus for Linux/Unix."));
        about_dialog.set_website('http://code.google.com/p/ibus');
        about_dialog.set_authors(['Peng Huang <shawn.p.huang@gmail.com>']);
        about_dialog.set_documenters(['Peng Huang <shawn.p.huang@gmail.com>']);
        about_dialog.set_translator_credits(_("translator-credits"));
        about_dialog.set_logo_icon_name('ibus');
        about_dialog.set_icon_name('ibus');
        about_dialog.run();
        about_dialog.destroy();
    },

    _about_item_status_activate_cb: function(item, user_data) {
        this._run_panel_about_gtk();
    },

    _restart_item_status_activate_cb: function(item, user_data) {
        this._bus.exit(true);
    },

    _quit_item_status_activate_cb: function(item, user_data) {
        this._bus.exit(false);
    },

    _set_im_icon: function(icon_name, label) {
        if (icon_name == null) {
            icon_name = ICON_ENGINE;
        }
        if (this._indicator != null) {
            if (icon_name[0] == '/') {
                let paths = null;
                let n_elements = 0;
                icon_name = GLib.path_get_basename(icon_name);
                if (icon_name.indexOf('.') >= 0) {
                    icon_name = icon_name.substr(0, icon_name.lastIndexOf('.'));
                }
            }
            if (label != null) {
                this._indicator.setLabel(label);
            } else {
                this._indicator.setIcon(icon_name);
            }
        } else {
            if (icon_name[0] == '/') {
                this._status_icon.set_from_file(icon_name);
            } else {
                this._status_icon.set_from_icon_name(icon_name);
            }
        }
    },

    _set_im_name: function(name) {
        this._language_bar.set_im_name(name);
    },

    set_cursor_location: function(panel, x, y, w, h) {
        this._candidate_panel.set_cursor_location(x, y, w, h);
    },

    update_preedit_text: function(panel, text, cursor_pos, visible) {
        this._candidate_panel.update_preedit_text(text, cursor_pos, visible);
    },

    show_preedit_text: function(panel) {
        this._candidate_panel.show_preedit_text();
    },

    hide_preedit_text: function(panel) {
        this._candidate_panel.hide_preedit_text();
    },

    update_auxiliary_text: function(panel, text, visible) {
        this._candidate_panel.update_auxiliary_text(text, visible);
    },

    show_auxiliary_text: function(panel) {
        this._candidate_panel.show_auxiliary_text();
    },

    hide_auxiliary_text: function(panel) {
        this._candidate_panel.hide_auxiliary_text();
    },

    update_lookup_table: function(panel, lookup_table, visible) {
        this._candidate_panel.update_lookup_table(lookup_table, visible);
    },

    show_lookup_table: function(panel) {
        this._candidate_panel.show_lookup_table();
    },

    hide_lookup_table: function(panel) {
        this._candidate_panel.hide_lookup_table();
    },

    page_up_lookup_table: function(panel) {
        this._candidate_panel.page_up_lookup_table();
    },

    page_down_lookup_table: function(panel) {
        this._candidate_panel.page_down_lookup_table();
    },

    cursor_up_lookup_table: function(panel) {
        this._candidate_panel.cursor_up_lookup_table();
    },

    cursor_down_lookup_table: function(panel) {
        this._candidate_panel.cursor_down_lookup_table();
    },

    show_candidate_window: function(panel) {
        this._candidate_panel.show_all();
    },

    hide_candidate_window: function(panel) {
        this._candidate_panel.hide_all();
    },

    register_properties: function(panel, props) {
        this._language_bar.register_properties(props);
    },

    update_property: function(panel, props) {
        this._language_bar.update_property(props);
    },

    focus_in: function(panel, path) {
        this.reset();
        this._focus_ic = IBus.InputContext.get_input_context(path, this._bus.get_connection());
        let enabled = this._focus_ic.is_enabled();
        this._language_bar.set_enabled(enabled);
        if (this._active_engine != null) {
            this._focus_ic.set_engine(this._active_engine.name);
            this._active_engine = null;
        }
        if (this._callback_after_focus_in != null) {
            this._callback_after_focus_in();
            this._callback_after_focus_in = null;
        }

        if (enabled == false) {
            this._set_im_icon(ICON_KEYBOARD, null);
            this._set_im_name(null);
        } else {
            let engine = this._focus_ic.get_engine();
            if (engine) {
                this._set_im_icon(engine.icon, engine.language);
                this._set_im_name(engine.longname);
            } else {
                this._set_im_icon(ICON_KEYBOARD, null);
                this._set_im_name(null);
            }
        }
    },

    focus_out: function(panel, path) {
        this.reset();
        this._focus_ic = null;
        this._language_bar.set_enabled(false);
        this._set_im_icon(ICON_KEYBOARD, null);
        this._set_im_name(null);
    },

    state_changed: function(panel) {
        if (this._focus_ic == null) {
            return;
        }

        let enabled = this._focus_ic.is_enabled();
        this._language_bar.set_enabled(enabled);

        if (enabled == false) {
            this.reset();
            this._set_im_icon(ICON_KEYBOARD, null);
            this._set_im_name(null);
        } else {
            let engine = this._focus_ic.get_engine();
            if (engine) {
                this._set_im_icon(engine.icon, engine.language);
                this._set_im_name(engine.longname);
            } else {
                this._set_im_icon(ICON_KEYBOARD, null);
                this._set_im_name(null);
            }
        }
    },

    reset: function(ic) {
        //this._candidate_panel.reset();
        //this._language_bar.reset();
    },
};
