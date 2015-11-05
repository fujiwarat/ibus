/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus
 * Copyright (C) 2015 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2015 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#include <glib.h>
#include <ibus.h>
#include <string.h>

#include "fbterm.h"
#include "imapi.h"
#include "inputcontext.h"

#define FONT_WIDTH(x, info) ((x) * (info).fontWidth)
#define FONT_HEIGHT(x, info) ((x) * (info).fontHeight)
#define MARGIN 5
#define GAP 5

enum {
    AuxiliaryTextWin = 0,
    LookupTableWin,
    StatusBarWin,
    PreeditTextWin
};

struct _FbTermInputContextPrivate {
    Info                info;
    guint16             cursor_x;
    guint16             cursor_y;
    guint16             window_height;
    guint16             window_interval;
    IBusText           *preedit_text;
    Rectangle           preedit_text_window;
    ColorType           color_fg;
    ColorType           color_bg;
    ColorType           color_activated_fg;
};

struct interval {
    gunichar first;
    gunichar last;
};


G_DEFINE_TYPE_WITH_PRIVATE (FbTermInputContext,
                            fbterm_input_context,
                            IBUS_TYPE_INPUT_CONTEXT);

static guint16
get_window_y (FbTermInputContext *input_context)
{
    FbTermInputContextPrivate *priv;
    Info *info;
    guint16 cursor_y;
    guint16 window_interval;
    guint16 max_cursor_y;

    g_return_val_if_fail (FBTERM_IS_INPUT_CONTEXT (input_context), 0);

    priv = input_context->priv;
    info = &priv->info;
    cursor_y = priv->cursor_y;
    window_interval = priv->window_interval;
    max_cursor_y = info->screenHeight - 3 * window_interval;

    if (cursor_y < max_cursor_y)
    if (info->screenHeight <= 3 * window_interval)
        return 0;

        return cursor_y;
    if (cursor_y >= 4 * window_interval)
        return cursor_y - 4 * window_interval;
    return max_cursor_y;
}

static char
bisearch (guint16                ucs,
          const struct interval *table,
          guint                  max)
{
        guint min = 0;
        guint mid;

        if (ucs < table[0].first || ucs > table[max].last)
                return 0;
        while (max >= min) {
                mid = (min + max) / 2;
                if (ucs > table[mid].last)
                        min = mid + 1;
                else if (ucs < table[mid].first)
                        max = mid - 1;
                else
                        return 1;
        }
        return 0;
}

static char
is_double_width (gunichar2 ucs)
{
    static const struct interval double_width[] = {
                { 0x1100, 0x115F}, { 0x2329, 0x232A}, { 0x2E80, 0x303E},
                { 0x3040, 0xA4CF}, { 0xAC00, 0xD7A3}, { 0xF900, 0xFAFF},
                { 0xFE10, 0xFE19}, { 0xFE30, 0xFE6F}, { 0xFF00, 0xFF60},
                { 0xFFE0, 0xFFE6}, { 0x20000, 0x2FFFD}, { 0x30000, 0x3FFFD}
    };

    return bisearch (ucs, double_width,
                     sizeof (double_width) / sizeof (struct interval) - 1);
}

static guint
text_width (gchar *utf8)
{
    glong items_read = 0;
    glong items_write = 0;
    GError *error = NULL;
    guint i;
    guint w = 0;

    gunichar2 *utf16 = g_utf8_to_utf16 (utf8,
                                        strlen (utf8),
                                        &items_read,
                                        &items_write,
                                        &error);

    if (error != NULL) {
        g_warning ("Conversion error to UTF16: %s", error->message);
        g_error_free (error);
        return 0;
    }

    for (i = 0; utf16[i]; i++, w++) {
        if (is_double_width (utf16[i]))
            w++;
    }

    g_free (utf16);

    return w;
}

static void
draw_margin (Rectangle          *rect,
             ColorType           color,
             IBusFbTerm         *ibus_fbterm)
{
        ibus_fbterm_fill_rect (ibus_fbterm,
                               rect->x,
                               rect->y,
                               rect->w,
                               MARGIN,
                               color);
        ibus_fbterm_fill_rect (ibus_fbterm,
                               rect->x,
                               rect->y + rect->h - MARGIN,
                               rect->w,
                               MARGIN,
                               color);
        ibus_fbterm_fill_rect (ibus_fbterm,
                               rect->x,
                               rect->y + MARGIN,
                               MARGIN,
                               rect->h - 2 * MARGIN,
                               color);
        ibus_fbterm_fill_rect (ibus_fbterm,
                               rect->x + rect->w - MARGIN,
                               rect->y + MARGIN,
                               MARGIN,
                               rect->h - 2 * MARGIN,
                               color);
}

static void
draw_preedit_text (IBusText           *preedit_text,
                   Rectangle          *rect,
                   ColorType           color_fg,
                   ColorType           color_bg,
                   ColorType           color_activated_fg,
                   Info               *info,
                   IBusFbTerm         *ibus_fbterm)
{
    IBusAttribute *attr;
    int i;
    guint start_index = G_MAXUINT;
    guint end_index = G_MAXUINT;
    guint x = 0;
    guint y = 0;

    ibus_fbterm_set_im_window (ibus_fbterm,
                               PreeditTextWin,
                               rect->x,
                               rect->y,
                               rect->w,
                               rect->h);
    if (!rect->w)
        return;

    draw_margin (rect, color_bg, ibus_fbterm);

    if (preedit_text->attrs != 0) {
        for (i = 0; ; i++) {
            attr = ibus_attr_list_get (preedit_text->attrs, i);
            if (attr == 0)
                break;
            if (attr->type == IBUS_ATTR_TYPE_BACKGROUND) {
                if ((attr->value & 0x00ffffff) > 0) {
                    start_index = attr->start_index;
                    end_index = attr->end_index;
                    break;
                }
            }
        }
    }

    ibus_fbterm_set_im_window (ibus_fbterm,
                               PreeditTextWin,
                               rect->x,
                               rect->y,
                               rect->w,
                               rect->h);
    x = rect->x + MARGIN;
    y = rect->y + MARGIN;
    if (start_index != G_MAXUINT && end_index > start_index) {
        char *start_text;
        char *end_text;
        char *segment;
        guint length;
        if (start_index > 0) {
            start_text = preedit_text->text;
            end_text = g_utf8_offset_to_pointer (preedit_text->text,
                                                 start_index);
            length = end_text - start_text;
            ibus_fbterm_draw_text (ibus_fbterm,
                                   x, y, color_fg, color_bg,
                                   start_text, length);
            segment = g_strndup (start_text, length);
            x += text_width (segment) * info->fontWidth;
            g_free (segment);
        }
        start_text = g_utf8_offset_to_pointer (preedit_text->text, start_index);
        end_text = g_utf8_offset_to_pointer (preedit_text->text, end_index);
        length = end_text - start_text;
        ibus_fbterm_draw_text (ibus_fbterm,
                               x, y, color_activated_fg, color_bg,
                               start_text, length);
        segment = g_strndup (start_text, length);
        x += text_width (segment) * info->fontWidth;
        g_free (segment);

        start_text = g_utf8_offset_to_pointer (preedit_text->text,
                                               end_index);
        end_text = preedit_text->text + strlen (preedit_text->text);
        length = end_text - start_text;
        if (length > 0)
            ibus_fbterm_draw_text (ibus_fbterm,
                                   x, y, color_fg, color_bg,
                                   start_text, length);
    } else {
        ibus_fbterm_draw_text (ibus_fbterm,
                               x, y, color_fg, color_bg,
                               preedit_text->text,
                               strlen (preedit_text->text));
    }
}

static void
commit_text_cb (IBusInputContext *ibus_input_context,
                IBusText         *text,
                gpointer          data)
{
    IBusFbTerm *ibus_fbterm = (IBusFbTerm *)data;
    ibus_fbterm_put_im_text (ibus_fbterm, text->text, strlen (text->text));
}

static void
show_preedit_text_cb (IBusInputContext *ibus_input_context,
                      gpointer          data)
{
}

static void
hide_preedit_text_cb (IBusInputContext *ibus_input_context,
                      gpointer          data)
{
    IBusFbTerm *ibus_fbterm = (IBusFbTerm *)data;
    FbTermInputContext *input_context;
    FbTermInputContextPrivate *priv;

    g_return_if_fail (FBTERM_IS_INPUT_CONTEXT (ibus_input_context));

    input_context = FBTERM_INPUT_CONTEXT (ibus_input_context);
    priv = input_context->priv;
    if (priv->preedit_text) {
                g_object_unref (priv->preedit_text);
                priv->preedit_text = 0;
    }

    draw_preedit_text (priv->preedit_text,
                       &priv->preedit_text_window,
                       priv->color_fg,
                       priv->color_bg,
                       priv->color_activated_fg,
                       &priv->info,
                       ibus_fbterm);
}

static void
update_preedit_text_cb (IBusInputContext *ibus_input_context,
                        IBusText         *text,
                        guint             cursor_pos,
                        gboolean          visible,
                        gpointer          data)
{
    IBusFbTerm *ibus_fbterm = (IBusFbTerm *)data;
    FbTermInputContext *input_context;
    FbTermInputContextPrivate *priv;

    g_return_if_fail (FBTERM_IS_INPUT_CONTEXT (ibus_input_context));

    input_context = FBTERM_INPUT_CONTEXT (ibus_input_context);
    priv = input_context->priv;
    if (priv->preedit_text) {
                g_object_unref (priv->preedit_text);
                priv->preedit_text = 0;
    }

    if (visible)
        priv->preedit_text = g_object_ref (text);
    else
        priv->preedit_text_window.w = 0;

    fbterm_input_context_calculate_preedit_window (input_context);
    draw_preedit_text (priv->preedit_text,
                       &priv->preedit_text_window,
                       priv->color_fg,
                       priv->color_bg,
                       priv->color_activated_fg,
                       &priv->info,
                       ibus_fbterm);
}

static void
fbterm_input_context_init (FbTermInputContext *input_context)
{
    FbTermInputContextPrivate *priv =
            fbterm_input_context_get_instance_private (input_context);

    input_context->priv = priv;
    priv->color_fg = Black;
    priv->color_bg = Gray;
    priv->color_activated_fg = DarkBlue;
}

static void
fbterm_input_context_class_init (FbTermInputContextClass *class)
{
}

FbTermInputContext *
fbterm_input_context_new (IBusFbTerm *ibus_fbterm,
                          IBusBus    *bus)
{
    const gchar *client_name = "fbterm";
    GVariant *result;
    GError *error = NULL;
    gchar *path;
    GInitable *initable;
    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
    FbTermInputContext *input_context = NULL;

    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (ibus_bus_is_connected (bus), NULL);

    result = g_dbus_connection_call_sync (ibus_bus_get_connection (bus),
                                          IBUS_SERVICE_IBUS,
                                          IBUS_PATH_IBUS,
                                          IBUS_INTERFACE_IBUS,
                                          "CreateInputContext",
                                          g_variant_new ("(s)", client_name),
                                          G_VARIANT_TYPE ("(o)"),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          ibus_get_timeout (),
                                          NULL,
                                          &error);
    if (result == NULL) {
        g_warning ("fbterm_input_context_new(): %s", error->message);
        g_error_free (error);
        return NULL;
    }

    g_variant_get (result, "(o)", &path);
    initable = g_initable_new (FBTERM_TYPE_INPUT_CONTEXT,
                               NULL,
                               &error,
                               "g-connection",    ibus_bus_get_connection (bus),
                               "g-name",            IBUS_SERVICE_IBUS,
                               "g-flags",           flags,
                               "g-interface-name",
                                       IBUS_INTERFACE_INPUT_CONTEXT,
                               "g-object-path",     path,
                               "g-default-timeout", ibus_get_timeout (),
                               NULL);
    if (initable != NULL)
        input_context = FBTERM_INPUT_CONTEXT (initable);
 
    g_variant_unref (result);
    if (input_context == NULL) {
        g_warning ("fbterm_input_context_new(): %s", error->message);
        g_error_free (error);
        return NULL;
    }

    g_object_connect (input_context,
                      "signal::commit-text", commit_text_cb, ibus_fbterm,
                      "signal::show-preedit-text", show_preedit_text_cb, ibus_fbterm,
                      "signal::hide-preedit-text", hide_preedit_text_cb, ibus_fbterm,
                      "signal::update-preedit-text", update_preedit_text_cb, ibus_fbterm,
#if 0
                      "signal::hide-lookup-table", hide_lookup_table_cb, ibus_fbterm,
                      "signal::update-lookup-table", update_lookup_table_cb, ibus_fbterm,
                      "signal::hide-auxiliary-text", hide_auxiliary_text_cb, ibus_fbterm,
                      "signal::update-auxiliary-text", update_auxiliary_text_cb, ibus_fbterm,
                      "signal::register-properties", register_properties_cb, ibus_fbterm,
                      "signal::update-property", update_property_cb, ibus_fbterm,
#endif
                      NULL);

    ibus_input_context_set_capabilities (IBUS_INPUT_CONTEXT (input_context),
                                         IBUS_CAP_AUXILIARY_TEXT |
                                         IBUS_CAP_LOOKUP_TABLE |
                                         IBUS_CAP_PROPERTY |
                                         IBUS_CAP_FOCUS |
                                         IBUS_CAP_PREEDIT_TEXT);
    return input_context;
}

void
fbterm_input_context_update_info (FbTermInputContext *input_context,
                                  Info               *info)
{
    FbTermInputContextPrivate *priv;

    g_return_if_fail (FBTERM_IS_INPUT_CONTEXT (input_context));

    priv = input_context->priv;

    memcpy ((void *)&priv->info, info, sizeof (Info));
    priv->window_height = info->fontHeight + 2 * MARGIN;
    priv->window_interval = priv->window_height + GAP;
}

void
fbterm_input_context_update_cursor_position (FbTermInputContext *input_context,
                                             guint16             x,
                                             guint16             y)
{
    FbTermInputContextPrivate *priv;

    g_return_if_fail (FBTERM_IS_INPUT_CONTEXT (input_context));

    priv = input_context->priv;
    priv->cursor_x = x;
    priv->cursor_y = y;
}

void
fbterm_input_context_calculate_preedit_window (
        FbTermInputContext *input_context)
{
    FbTermInputContextPrivate *priv;

    g_return_if_fail (FBTERM_IS_INPUT_CONTEXT (input_context));

    priv = input_context->priv;

    if (!priv->preedit_text) {
        priv->preedit_text_window.w = 0;
        return;
    }

    priv->preedit_text_window.x = priv->cursor_x;
    priv->preedit_text_window.y = get_window_y (input_context) + GAP;
    priv->preedit_text_window.w =
            text_width (priv->preedit_text->text) * priv->info.fontWidth
            + 2 * MARGIN;
    priv->preedit_text_window.h = priv->window_height;

    if (priv->preedit_text_window.x + priv->preedit_text_window.w
                > priv->info.screenWidth) {
        if (priv->preedit_text_window.w > priv->info.screenWidth) {
            priv->preedit_text_window.x = 0;
        } else {
            priv->preedit_text_window.x =
                    priv->info.screenWidth - priv->preedit_text_window.w;
        }
    }
}
