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
const GdkPixbuf = imports.gi.GdkPixbuf;
const Pango = imports.gi.Pango;
const IBus = imports.gi.IBus;

const _ = imports.ui.status.ibus.common._;


function EngineAbout(enginedesc) {
    this._init(enginedesc);
}

EngineAbout.prototype = {
    _init: function(enginedesc) {
        this._engine_desc = enginedesc;
        this._init_ui();
    },

    _init_ui: function() {
        this._about_dialog = new Gtk.Dialog();
        this._about_dialog.set_title(_("About"));
        this._about_dialog.add_button(Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE);
        this._about_dialog.set_icon_name('help-about');
        let sw = new Gtk.ScrolledWindow();
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        this._text_view = new Gtk.TextView();
        this._text_view.set_size_request(400, 400);
        this._text_view.set_editable(false);
        sw.add(this._text_view);
        sw.set_size_request(400, 400);
        sw.show_all();
        let vbox = this._about_dialog.get_content_area();
        vbox.pack_start(sw, false, false, 0);
        this._fill_text_view();
    },

    _fill_text_view: function() {
        let text_buffer = this._text_view.get_buffer();
        let iter = null;
        text_buffer.insert_at_cursor('\n ', -1);
        iter = text_buffer.get_end_iter();
        text_buffer.insert_pixbuf(iter,
                                  this._load_icon(this._engine_desc.get_icon()));
        text_buffer.insert_at_cursor('\n' + this._engine_desc.get_longname() +
                                     '\n', -1);
        let lang = IBus.get_language_name(this._engine_desc.get_language());
        if (lang == null) {
            lang = _("Other");
        }
        text_buffer.insert_at_cursor(_("Language: ") +
                                     lang +
                                     '\n', -1);
        text_buffer.insert_at_cursor(_("Keyboard layout: ") +
                                     this._engine_desc.get_layout() +
                                     '\n', -1);
        text_buffer.insert_at_cursor(_("Author: ") +
                                     this._engine_desc.get_author() +
                                     '\n', -1);
        text_buffer.insert_at_cursor(_("Description:\n"), -1);
        text_buffer.insert_at_cursor(this._engine_desc.get_description(), -1);
    },

    _create_tags: function(text_buffer) {
        let tag = null;

        tag = new Gtk.TextTag({name: 'heading'});
        text_buffer.get_tag_table().add(tag);
        tag.set_data('weight', Pango.Weight.BOLD);
        tag.set_data('size', 16 * Pango.SCALE);
        tag.unref();

        text_buffer.create_tag('bold', Pango.Weight.BOLD);
        text_buffer.create_tag('italic', Pango.Style.ITALIC);
        text_buffer.create_tag('small', 0.8333333333333);
        text_buffer.create_tag('gray_foreground', 'dark gray');
        text_buffer.create_tag('wrap_text', Gtk.WrapMode.WORD);
        text_buffer.create_tag('left_margin_16', left_margin=16);
        text_buffer.create_tag('left_margin_32', left_margin=32);
    },

    _load_icon: function(icon) {
        let pixbuf = null;
        try {
            pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(icon, 48, 48, true);
        } catch(e) {
            let theme = Gtk.icon_theme_get_default();
            icon = theme.lookup_icon('ibus-engine', 48, 0);
            if (icon == null) {
                icon = theme.lookup_icon('image-missing', 48, 0);
            }
            pixbuf = icon.load_icon();
        }
        return pixbuf;
    },

    run: function() {
        this._about_dialog.run();
    },

    destroy: function() {
        this._about_dialog.destroy();
    },
};
