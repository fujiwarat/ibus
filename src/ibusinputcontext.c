/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2017 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2017 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <dbus/dbus.h>
#include "ibusshare.h"
#include "ibusinternal.h"
#include "ibusinputcontext.h"
#include "ibusattribute.h"
#include "ibuslookuptable.h"
#include "ibusproplist.h"

#define IBUS_CURSOR_LOCATION_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_CURSOR_LOCATION, IBusCursorLocationPrivate))
#define IBUS_INPUT_CONTEXT_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_INPUT_CONTEXT, IBusInputContextPrivate))

enum {
    PROP_0 = 0,
    PROP_X,
    PROP_Y,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_DISPLAY_NAME
};

enum {
    ENABLED,
    DISABLED,
    COMMIT_TEXT,
    FORWARD_KEY_EVENT,
    DELETE_SURROUNDING_TEXT,
    UPDATE_PREEDIT_TEXT,
    SHOW_PREEDIT_TEXT,
    HIDE_PREEDIT_TEXT,
    UPDATE_AUXILIARY_TEXT,
    SHOW_AUXILIARY_TEXT,
    HIDE_AUXILIARY_TEXT,
    UPDATE_LOOKUP_TABLE,
    SHOW_LOOKUP_TABLE,
    HIDE_LOOKUP_TABLE,
    PAGE_UP_LOOKUP_TABLE,
    PAGE_DOWN_LOOKUP_TABLE,
    CURSOR_UP_LOOKUP_TABLE,
    CURSOR_DOWN_LOOKUP_TABLE,
    REGISTER_PROPERTIES,
    UPDATE_PROPERTY,
    LAST_SIGNAL,
};


/* BusInputContextPriv */
struct _IBusCursorLocationPrivate {
    int    x;
    int    y;
    int    width;
    int    height;
    gchar *display_name;
};

struct _IBusInputContextPrivate {
    gboolean own;
};
typedef struct _IBusInputContextPrivate IBusInputContextPrivate;

static guint            context_signals[LAST_SIGNAL] = { 0 };
// static guint            context_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void     ibus_cursor_location_destroy    (IBusCursorLocation    *cursor);
static void     ibus_cursor_location_set_property
                                                (IBusCursorLocation    *desc,
                                                 guint                  prop_id,
                                                 const GValue          *value,
                                                 GParamSpec            *pspec);
static void     ibus_cursor_location_get_property
                                                (IBusCursorLocation    *desc,
                                                 guint                  prop_id,
                                                 GValue                *value,
                                                 GParamSpec            *pspec);
static gboolean ibus_cursor_location_serialize  (IBusCursorLocation    *cursor,
                                                 IBusMessageIter       *iter);
static gboolean ibus_cursor_location_deserialize
                                                (IBusCursorLocation    *cursor,
                                                 IBusMessageIter       *iter);
static gboolean ibus_cursor_location_copy       (IBusCursorLocation    *dest,
                                                 const IBusCursorLocation
                                                                       *src);
static void     ibus_input_context_real_destroy (IBusInputContext       *context);
static gboolean ibus_input_context_ibus_signal  (IBusProxy              *proxy,
                                                 DBusMessage            *message);

G_DEFINE_TYPE (IBusInputContext, ibus_input_context, IBUS_TYPE_PROXY)
G_DEFINE_TYPE (IBusCursorLocation, ibus_cursor_location, IBUS_TYPE_SERIALIZABLE)

static void
ibus_cursor_location_class_init (IBusCursorLocationClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (klass);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (klass);

    ibus_cursor_location_parent_class =
            (IBusSerializableClass *) g_type_class_peek_parent (klass);

    g_type_class_add_private (klass, sizeof (IBusCursorLocationPrivate));

    object_class->destroy =
            (IBusObjectDestroyFunc) ibus_cursor_location_destroy;

    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_cursor_location_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_cursor_location_get_property;
    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_cursor_location_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_cursor_location_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_cursor_location_copy;

    g_string_append (serializable_class->signature, "iiiis");

    /* install properties */
    /**
     * IBusCursorLocation:x:
     *
     * X coordiante of the cursor location
     */
    g_object_class_install_property (gobject_class,
                    PROP_X,
                    g_param_spec_int ("x",
                        "X coordinate",
                        "X coordiante of the cursor location",
                        G_MININT,
                        G_MAXINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusCursorLocation:y:
     *
     * Y coordiante of the cursor location
     */
    g_object_class_install_property (gobject_class,
                    PROP_Y,
                    g_param_spec_int ("y",
                        "Y coordinate",
                        "Y coordiante of the cursor location",
                        G_MININT,
                        G_MAXINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusCursorLocation:width:
     *
     * The width of the cursor
     */
    g_object_class_install_property (gobject_class,
                    PROP_WIDTH,
                    g_param_spec_int ("width",
                        "cursor width",
                        "The width of the cursor",
                        G_MININT,
                        G_MAXINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusCursorLocation:height:
     *
     * The width of the cursor
     */
    g_object_class_install_property (gobject_class,
                    PROP_HEIGHT,
                    g_param_spec_int ("height",
                        "cursor height",
                        "The height of the cursor",
                        G_MININT,
                        G_MAXINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusCursorLocation:display_name:
     *
     * The value of $DISPLAY
     */
    g_object_class_install_property (gobject_class,
                    PROP_DISPLAY_NAME,
                    g_param_spec_string ("display-name",
                        "display name",
                        "The value of $DISPLAY",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_cursor_location_init (IBusCursorLocation *cursor)
{
    cursor->priv = IBUS_CURSOR_LOCATION_GET_PRIVATE (cursor);
}

static void
ibus_cursor_location_destroy (IBusCursorLocation *cursor)
{
    g_free (cursor->priv->display_name);
    cursor->priv->display_name = NULL;

    IBUS_OBJECT_CLASS (ibus_cursor_location_parent_class)->
            destroy ((IBusObject *)cursor);
}

static void
ibus_cursor_location_set_property (IBusCursorLocation *cursor,
                                   guint               prop_id,
                                   const GValue       *value,
                                   GParamSpec         *pspec)
{
    switch (prop_id) {
    case PROP_X:
        cursor->priv->x = g_value_get_int (value);
        break;
    case PROP_Y:
        cursor->priv->y = g_value_get_int (value);
        break;
    case PROP_WIDTH:
        cursor->priv->width = g_value_get_int (value);
        break;
    case PROP_HEIGHT:
        cursor->priv->height= g_value_get_int (value);
        break;
    case PROP_DISPLAY_NAME:
        g_assert (cursor->priv->display_name == NULL);
        cursor->priv->display_name = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (cursor, prop_id, pspec);
    }
}

static void
ibus_cursor_location_get_property (IBusCursorLocation *cursor,
                                   guint               prop_id,
                                   GValue             *value,
                                   GParamSpec         *pspec)
{
    switch (prop_id) {
    case PROP_X:
        g_value_set_int (value, ibus_cursor_location_get_x (cursor));
        break;
    case PROP_Y:
        g_value_set_int (value, ibus_cursor_location_get_y (cursor));
        break;
    case PROP_WIDTH:
        g_value_set_int (value, ibus_cursor_location_get_width (cursor));
        break;
    case PROP_HEIGHT:
        g_value_set_int (value, ibus_cursor_location_get_height (cursor));
        break;
    case PROP_DISPLAY_NAME:
        g_value_set_string (value,
                            ibus_cursor_location_get_display_name (cursor));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (cursor, prop_id, pspec);
    }
}


static gboolean
ibus_cursor_location_serialize (IBusCursorLocation *cursor,
                                IBusMessageIter    *iter)
{
    gboolean retval;
    IBusCursorLocationPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_cursor_location_parent_class)->
            serialize ((IBusSerializable *)cursor, iter);
    g_return_val_if_fail (retval, FALSE);

    priv = cursor->priv;
    retval = ibus_message_iter_append (iter, G_TYPE_INT, &priv->x);
    g_return_val_if_fail (retval, FALSE);
    retval = ibus_message_iter_append (iter, G_TYPE_INT, &priv->y);
    g_return_val_if_fail (retval, FALSE);
    retval = ibus_message_iter_append (iter, G_TYPE_INT, &priv->width);
    g_return_val_if_fail (retval, FALSE);
    retval = ibus_message_iter_append (iter, G_TYPE_INT, &priv->height);
    g_return_val_if_fail (retval, FALSE);
    retval = ibus_message_iter_append (iter, G_TYPE_STRING,
                                       &priv->display_name);
    g_return_val_if_fail (retval, FALSE);

    return TRUE;
}

static gboolean
ibus_cursor_location_deserialize (IBusCursorLocation *cursor,
                                  IBusMessageIter    *iter)
{
    gboolean retval;
    IBusCursorLocationPrivate *priv;
    gchar *str;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_cursor_location_parent_class)->
            deserialize ((IBusSerializable *)cursor, iter);
    g_return_val_if_fail (retval, FALSE);

    priv = cursor->priv;
    retval = ibus_message_iter_get (iter, G_TYPE_INT, &priv->x);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    retval = ibus_message_iter_get (iter, G_TYPE_INT, &priv->y);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    retval = ibus_message_iter_get (iter, G_TYPE_INT, &priv->width);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    retval = ibus_message_iter_get (iter, G_TYPE_INT, &priv->height);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);

    g_free (priv->display_name);
    priv->display_name = g_strdup (str);

    return TRUE;
}

static gboolean
ibus_cursor_location_copy (IBusCursorLocation       *dest,
                           const IBusCursorLocation *src)
{
    gboolean retval;
    IBusCursorLocationPrivate *priv_dest;
    IBusCursorLocationPrivate *priv_src;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_cursor_location_parent_class)->
            copy ( (IBusSerializable *)dest,
                   (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (src), FALSE);
    priv_dest = dest->priv;
    priv_src = src->priv;

    priv_dest->x = priv_src->x;
    priv_dest->y = priv_src->y;
    priv_dest->width = priv_src->width;
    priv_dest->height = priv_src->height;
    priv_dest->display_name = g_strdup (priv_src->display_name);
    return TRUE;
}

IBusCursorLocation *
ibus_cursor_location_new (const gchar *first_property_name,
                          ...)
{
    va_list var_args;
    IBusCursorLocation *cursor;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    cursor = (IBusCursorLocation *) g_object_new_valist (IBUS_TYPE_CURSOR_LOCATION,
                                                         first_property_name,
                                                         var_args);
    va_end (var_args);

    return cursor;
}

int
ibus_cursor_location_get_x (IBusCursorLocation *cursor)
{
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (cursor), 0);
    return cursor->priv->x;
}

int
ibus_cursor_location_get_y (IBusCursorLocation *cursor)
{
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (cursor), 0);
    return cursor->priv->y;
}

int
ibus_cursor_location_get_width (IBusCursorLocation *cursor)
{
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (cursor), 0);
    return cursor->priv->width;
}

int
ibus_cursor_location_get_height (IBusCursorLocation *cursor)
{
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (cursor), 0);
    return cursor->priv->height;
}

const gchar *
ibus_cursor_location_get_display_name (IBusCursorLocation *cursor)
{
    g_return_val_if_fail (IBUS_IS_CURSOR_LOCATION (cursor), NULL);
    return cursor->priv->display_name;
}

IBusInputContext *
ibus_input_context_new (const gchar     *path,
                        IBusConnection  *connection)
{
    g_assert (path != NULL);
    g_assert (IBUS_IS_CONNECTION (connection));
    GObject *obj;

    obj = g_object_new (IBUS_TYPE_INPUT_CONTEXT,
                        "name", IBUS_SERVICE_IBUS,
                        "interface", IBUS_INTERFACE_INPUT_CONTEXT,
                        "path", path,
                        "connection", connection,
                        NULL);

    return IBUS_INPUT_CONTEXT (obj);
}

IBusInputContext *
ibus_input_context_get_input_context (const gchar        *path,
                                      IBusConnection     *connection)
{
    IBusInputContext *context = ibus_input_context_new (path, connection);
    IBusInputContextPrivate *priv;
    if (!context)
        return NULL;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (context);
    priv->own = FALSE;
    return context;
}

static void
ibus_input_context_class_init (IBusInputContextClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusProxyClass *proxy_class = IBUS_PROXY_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IBusInputContextPrivate));

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_input_context_real_destroy;

    proxy_class->ibus_signal = ibus_input_context_ibus_signal;

    /* install signals */
    /**
     * IBusInputContext::enabled:
     * @context: An IBusInputContext.
     *
     * Emitted when an IME is enabled.
     */
    context_signals[ENABLED] =
        g_signal_new (I_("enabled"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::disabled:
     * @context: An IBusInputContext.
     *
     * Emitted when an IME is disabled.
     */
    context_signals[DISABLED] =
        g_signal_new (I_("disabled"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::commit-text:
     * @context: An IBusInputContext.
     * @text: Text to be committed.
     *
     * Emitted when the text is going to be committed.
     *
     * (Note: The text object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[COMMIT_TEXT] =
        g_signal_new (I_("commit-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_TEXT);

    /**
     * IBusInputContext::forward-key-event:
     * @context: An IBusInputContext.
     * @keyval: Key symbol of the keyboard event.
     * @keycode: Key symbol of the keyboard event.
     * @modifiers: Key modifier flags.
     *
     * Emitted to forward key event from IME to client of IME.
     */
    context_signals[FORWARD_KEY_EVENT] =
        g_signal_new (I_("forward-key-event"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);

    /**
     * IBusInputContext::delete-surrounding-text:
     * @context: An IBusInputContext.
     * @offset: the character offset from the cursor position of the text to be deleted.
     *   A negative value indicates a position before the cursor.
     * @n_chars: the number of characters to be deleted.
     *
     * Emitted to delete surrounding text event from IME to client of IME.
     */
    context_signals[DELETE_SURROUNDING_TEXT] =
        g_signal_new (I_("delete-surrounding-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__INT_UINT,
            G_TYPE_NONE,
            2,
            G_TYPE_INT,
            G_TYPE_UINT);

    /**
     * IBusInputContext::update-preedit-text:
     * @context: An IBusInputContext.
     * @text: Text to be updated.
     * @cursor_pos: Cursor position.
     * @visible: Whether the update is visible.
     *
     * Emitted to update preedit text.
     *
     * (Note: The text object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[UPDATE_PREEDIT_TEXT] =
        g_signal_new (I_("update-preedit-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT_UINT_BOOLEAN,
            G_TYPE_NONE,
            3,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::show-preedit-text:
     * @context: An IBusInputContext.
     *
     * Emitted to show preedit text.
     */
    context_signals[SHOW_PREEDIT_TEXT] =
        g_signal_new (I_("show-preedit-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-preedit-text:
     * @context: An IBusInputContext.
     *
     * Emitted to hide preedit text.
     */
    context_signals[HIDE_PREEDIT_TEXT] =
        g_signal_new (I_("hide-preedit-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::update-auxiliary-text:
     * @context: An IBusInputContext.
     *
     * Emitted to hide auxilary text.
     *
     * (Note: The text object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[UPDATE_AUXILIARY_TEXT] =
        g_signal_new (I_("update-auxiliary-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_TEXT,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::show-auxiliary-text:
     * @context: An IBusInputContext.
     *
     * Emitted to show auxiliary text.
     */
    context_signals[SHOW_AUXILIARY_TEXT] =
        g_signal_new (I_("show-auxiliary-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-auxiliary-text:
     * @context: An IBusInputContext.
     *
     * Emitted to hide auxiliary text.
     */
    context_signals[HIDE_AUXILIARY_TEXT] =
        g_signal_new (I_("hide-auxiliary-text"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::update-lookup-table:
     * @context: An IBusInputContext.
     * @table: An IBusLookupTable to be updated.
     * @visible: Whether the table should be visible.
     *
     * Emitted to update lookup table.
     *
     * (Note: The table object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[UPDATE_LOOKUP_TABLE] =
        g_signal_new (I_("update-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_LOOKUP_TABLE,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::show-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to show lookup table.
     */
    context_signals[SHOW_LOOKUP_TABLE] =
        g_signal_new (I_("show-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to hide lookup table.
     */
    context_signals[HIDE_LOOKUP_TABLE] =
        g_signal_new (I_("hide-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::page-up-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to view the previous page of lookup table.
     */
    context_signals[PAGE_UP_LOOKUP_TABLE] =
        g_signal_new (I_("page-up-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::page-down-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to view the next page of lookup table.
     */
    context_signals[PAGE_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("page-down-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::cursor-up-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to select previous candidate of lookup table.
     */
    context_signals[CURSOR_UP_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-up-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::cursor-down-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to select next candidate of lookup table.
     */
    context_signals[CURSOR_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-down-lookup-table"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    /**
     * IBusInputContext::register-properties:
     * @context: An IBusInputContext.
     * @props: An IBusPropList that contains properties.
     *
     * Emitted to register the properties in @props.
     *
     * (Note: The props object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[REGISTER_PROPERTIES] =
        g_signal_new (I_("register-properties"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROP_LIST);

    /**
     * IBusInputContext::update-property:
     * @context: An IBusInputContext.
     * @prop: The IBusProperty to be updated.
     *
     * Emitted to update the property @prop.
     *
     * (Note: The prop object is floating, and it will be released after the signal.
     *  If singal handler want to keep the object, the handler should use g_object_ref_sink()
     *  to get the ownership of the object.)
     */
    context_signals[UPDATE_PROPERTY] =
        g_signal_new (I_("update-property"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROPERTY);
}

static void
ibus_input_context_init (IBusInputContext *context)
{
    IBusInputContextPrivate *priv;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (context);
    priv->own = TRUE;
}

static void
ibus_input_context_real_destroy (IBusInputContext *context)
{
    IBusInputContextPrivate *priv;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (context);

    if (priv->own && ibus_proxy_get_connection ((IBusProxy *) context) != NULL) {
        ibus_proxy_call (IBUS_PROXY (context),
                         "Destroy",
                         G_TYPE_INVALID);
    }

    IBUS_OBJECT_CLASS(ibus_input_context_parent_class)->destroy (IBUS_OBJECT (context));
}

static gboolean
ibus_input_context_ibus_signal (IBusProxy           *proxy,
                                IBusMessage         *message)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (proxy));
    g_assert (message != NULL);
    g_assert (ibus_message_get_type (message) == DBUS_MESSAGE_TYPE_SIGNAL);

    IBusInputContext *context;
    IBusError *error = NULL;
    gint i;
    const gchar *interface;
    const gchar *name;

    context = IBUS_INPUT_CONTEXT (proxy);

    static const struct {
        const gchar *member;
        guint signal_id;
    } signals [] = {
        { "Enabled",                ENABLED                  },
        { "Disabled",               DISABLED                 },
        { "ShowPreeditText",        SHOW_PREEDIT_TEXT        },
        { "HidePreeditText",        HIDE_PREEDIT_TEXT        },
        { "ShowAuxiliaryText",      SHOW_AUXILIARY_TEXT      },
        { "HideAuxiliaryText",      HIDE_AUXILIARY_TEXT      },
        { "ShowLookupTable",        SHOW_LOOKUP_TABLE        },
        { "HideLookupTable",        HIDE_LOOKUP_TABLE        },
        { "PageUpLookupTable",      PAGE_UP_LOOKUP_TABLE     },
        { "PageDownLookupTable",    PAGE_DOWN_LOOKUP_TABLE   },
        { "CursorUpLookupTable",    CURSOR_UP_LOOKUP_TABLE   },
        { "CursorDownLookupTable",  CURSOR_DOWN_LOOKUP_TABLE },
    };

    do {
        interface = ibus_message_get_interface (message);
        name = ibus_message_get_member (message);

        if (interface != NULL && g_strcmp0 (interface, IBUS_INTERFACE_INPUT_CONTEXT) != 0) {
            error = ibus_error_new_from_printf (DBUS_ERROR_FAILED,
                                                "Signal %s.%s is not handled", interface, name);
            break;
        }

        if (g_strcmp0 (name, "CommitText") == 0) {
            IBusText *text;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_TEXT, &text,
                                            G_TYPE_INVALID);
            if (retval) {
                g_signal_emit (context, context_signals[COMMIT_TEXT], 0, text);
                if (g_object_is_floating (text))
                    g_object_unref (text);
            }
            break;
        }
        if (g_strcmp0 (name, "UpdatePreeditText") == 0) {
            IBusText *text;
            gint32 cursor_pos;
            gboolean visible;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_TEXT, &text,
                                            G_TYPE_UINT, &cursor_pos,
                                            G_TYPE_BOOLEAN, &visible,
                                            G_TYPE_INVALID);

            if (retval) {
                g_signal_emit (context,
                               context_signals[UPDATE_PREEDIT_TEXT],
                               0,
                               text,
                               cursor_pos,
                               visible);
                if (g_object_is_floating (text))
                    g_object_unref (text);
            }
            break;
        }

        for (i = 0;
             i < G_N_ELEMENTS (signals) && g_strcmp0 (name, signals[i].member) != 0;
             i++);

        if (i < G_N_ELEMENTS (signals)) {
            g_signal_emit (context, context_signals[signals[i].signal_id], 0);
            break;
        }

        if (g_strcmp0 (name, "UpdateAuxiliaryText") == 0) {
            IBusText *text;
            gboolean visible;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_TEXT, &text,
                                            G_TYPE_BOOLEAN, &visible,
                                            G_TYPE_INVALID);

            if (retval) {
                g_signal_emit (context,
                               context_signals[UPDATE_AUXILIARY_TEXT],
                               0,
                               text,
                               visible);
                if (g_object_is_floating (text))
                    g_object_unref (text);
            }
        }
        else if (g_strcmp0 (name, "UpdateLookupTable") == 0) {
            IBusLookupTable *table;
            gboolean visible;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_LOOKUP_TABLE, &table,
                                            G_TYPE_BOOLEAN, &visible,
                                            G_TYPE_INVALID);

            if (retval) {
                g_signal_emit (context,
                               context_signals[UPDATE_LOOKUP_TABLE],
                               0,
                               table,
                               visible);
                if (g_object_is_floating (table))
                    g_object_unref (table);
            }
        }
        else if (g_strcmp0 (name, "RegisterProperties") == 0) {
            IBusPropList *prop_list;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_PROP_LIST, &prop_list,
                                            G_TYPE_INVALID);

            if (retval) {
                g_signal_emit (context,
                               context_signals[REGISTER_PROPERTIES],
                               0,
                               prop_list);
                if (g_object_is_floating (prop_list))
                    g_object_unref (prop_list);
            }
        }
        else if (g_strcmp0 (name, "UpdateProperty") == 0) {
            IBusProperty *prop;
            gboolean retval;

            retval = ibus_message_get_args (message,
                                            &error,
                                            IBUS_TYPE_PROPERTY, &prop,
                                            G_TYPE_INVALID);
            if (retval) {
                g_signal_emit (context, context_signals[UPDATE_PROPERTY], 0, prop);
                if (g_object_is_floating (prop))
                    g_object_unref (prop);
            }
        }
        else if (g_strcmp0 (name, "ForwardKeyEvent") == 0) {
            guint32 keyval;
            guint32 keycode;
            guint32 state;
            gboolean retval;


            retval = ibus_message_get_args (message,
                                            &error,
                                            G_TYPE_UINT, &keyval,
                                            G_TYPE_UINT, &keycode,
                                            G_TYPE_UINT, &state,
                                            G_TYPE_INVALID);

            if (retval) {
                /* Forward key event back with IBUS_FORWARD_MASK. And process_key_event will
                 * not process key event with IBUS_FORWARD_MASK again. */
                g_signal_emit (context,
                               context_signals[FORWARD_KEY_EVENT],
                               0,
                               keyval,
                               keycode,
                               state | IBUS_FORWARD_MASK);
            }
        }
        else if (g_strcmp0 (name, "DeleteSurroundingText") == 0) {
            gint offset_from_cursor;
            guint nchars;
            gboolean retval;
            retval = ibus_message_get_args (message,
                                            &error,
                                            G_TYPE_INT, &offset_from_cursor,
                                            G_TYPE_UINT, &nchars,
                                            G_TYPE_INVALID);


            if (retval) {
                g_signal_emit (context,
                               context_signals[DELETE_SURROUNDING_TEXT],
                               0,
                               offset_from_cursor,
                               nchars);
            }
        }
        else {
            error = ibus_error_new_from_printf (DBUS_ERROR_FAILED,
                                                "Signal %s.%s is not handled", interface, name);
            break;
        }
    } while (0);

    if (error == NULL) {
        g_signal_stop_emission_by_name (context, "ibus-signal");
        return TRUE;
    }

    /* some error happens */
    g_warning ("%s: %s", error->name, error->message);
    ibus_error_free (error);
    return FALSE;
}

typedef struct {
    IBusInputContext *context;
    guint32 keyval;
    guint32 keycode;
    guint32 state;
} CallData;

static void
_process_key_event_reply_cb (IBusPendingCall *pending,
                             CallData        *call_data)
{
    IBusMessage *reply_message;
    IBusError *error;
    gboolean retval = FALSE;

    reply_message = dbus_pending_call_steal_reply (pending);

    if (reply_message == NULL) {
        /* reply timeout */
        retval = FALSE;
    }
    else if ((error = ibus_error_new_from_message (reply_message)) != NULL) {
        /* some error happens */
        g_warning ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        retval = FALSE;
    }
    else if (!ibus_message_get_args (reply_message,
                                    &error,
                                     G_TYPE_BOOLEAN, &retval,
                                     G_TYPE_INVALID)) {
        /* can not get return value */
        g_warning ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        retval = FALSE;
    }

    if (!retval) {
        /* Forward key event back with IBUS_FORWARD_MASK. And process_key_event will
         * not process key event with IBUS_FORWARD_MASK again. */
        g_signal_emit (call_data->context,
                       context_signals[FORWARD_KEY_EVENT],
                       0,
                       call_data->keyval,
                       call_data->keycode,
                       call_data->state | IBUS_FORWARD_MASK);
    }

    if (reply_message != NULL) {
        dbus_message_unref (reply_message);
    }
}

static void
_call_data_free (CallData *call_data)
{
    g_object_unref (call_data->context);
    g_slice_free (CallData, call_data);
}

gboolean
ibus_input_context_process_key_event (IBusInputContext *context,
                                      guint32           keyval,
                                      guint32           keycode,
                                      guint32           state)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    IBusPendingCall *pending = NULL;
    IBusError *error = NULL;
    CallData *call_data;
    gboolean retval;

    if (state & IBUS_HANDLED_MASK)
        return TRUE;

    if (state & IBUS_IGNORED_MASK)
        return FALSE;

    retval = ibus_proxy_call_with_reply ((IBusProxy *) context,
                                         "ProcessKeyEvent",
                                         &pending,
                                         -1,
                                         &error,
                                         G_TYPE_UINT, &keyval,
                                         G_TYPE_UINT, &keycode,
                                         G_TYPE_UINT, &state,
                                         G_TYPE_INVALID);
    if (!retval) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        return FALSE;
    }

    call_data = g_slice_new0 (CallData);
    g_object_ref (context);
    call_data->context = context;
    call_data->keyval = keyval;
    call_data->keycode = keycode;
    call_data->state = state;

    /* set notify callback to handle the reply from daemon */
    retval = ibus_pending_call_set_notify (pending,
                                           (IBusPendingCallNotifyFunction)_process_key_event_reply_cb,
                                           call_data,
                                           (GDestroyNotify)_call_data_free);
    ibus_pending_call_unref (pending);

    if (!retval) {
        _call_data_free (call_data);
        g_warning ("%s : ProcessKeyEvent", DBUS_ERROR_NO_MEMORY);
        return FALSE;
    }

    return TRUE;
}

typedef struct {
    IBusInputContext                   *context;
    IBusInputContextNotifyFunction      callback;
    gpointer                            user_data;
} IBusPendingCallNotifyData;

static void
_process_key_event_notify (IBusPendingCall *pending, gpointer user_data)
{
    IBusPendingCallNotifyData *data = (IBusPendingCallNotifyData *) user_data;

    data->callback (data->context, pending, data->user_data);

    g_object_unref (data->context);
    g_slice_free (IBusPendingCallNotifyData, data);
}

void
ibus_input_context_process_key_event_async (IBusInputContext   *context,
                                            guint32             keyval,
                                            guint32             keycode,
                                            guint32             state,
                                            gint                timeout_msec,
                                            GDestroyNotify      free_user_data,
                                            IBusInputContextNotifyFunction
                                                                callback,
                                            gpointer            user_data)
{
    IBusPendingCall *pending = NULL;
    IBusError *error = NULL;
    gboolean retval;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    if (state & IBUS_HANDLED_MASK)
        return;

    if (state & IBUS_IGNORED_MASK)
        return;

    retval = ibus_proxy_call_with_reply ((IBusProxy *) context,
                                         "ProcessKeyEvent",
                                         &pending,
                                         -1,
                                         &error,
                                         G_TYPE_UINT, &keyval,
                                         G_TYPE_UINT, &keycode,
                                         G_TYPE_UINT, &state,
                                         G_TYPE_INVALID);
    if (!retval) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        return;
    }

    IBusPendingCallNotifyData *data = g_slice_new0 (IBusPendingCallNotifyData);
    data->context = g_object_ref_sink (context);
    data->callback= callback;
    data->user_data = user_data;

    ibus_pending_call_set_notify (pending,
                                  _process_key_event_notify,
                                  data,
                                  free_user_data);
}

gboolean
ibus_input_context_process_key_event_async_finish (IBusInputContext  *context,
                                                   IBusPendingCall   *pending,
                                                   IBusError        **error)
{
    IBusMessage *reply_message;
    gboolean processed = TRUE;
    IBusError *_error = NULL;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (pending != NULL);
    g_assert (error == NULL || *error == NULL);

    reply_message = ibus_pending_call_steal_reply (pending);
    ibus_pending_call_unref (pending);

    if (reply_message == NULL) {
        g_debug ("%s: Do not recevie reply of ProcessKeyEvent",
                 DBUS_ERROR_NO_REPLY);
        processed = FALSE;
    }
    else if ((_error = ibus_error_new_from_message (reply_message)) != NULL) {
        ibus_message_unref (reply_message);
        processed = FALSE;
    }
    else {
        if (!ibus_message_get_args (reply_message,
                                    &_error,
                                    G_TYPE_BOOLEAN, &processed,
                                    G_TYPE_INVALID)) {
            processed = FALSE;
        }
        ibus_message_unref (reply_message);
    }

    if (_error != NULL) {
        if (error != NULL)
            *error = ibus_error_new_from_text (_error->name, _error->message);
        ibus_error_free (_error);
    }

    return processed;
}

#if 0
static GVariant *
cursor_valist_serialize (const gchar     *first_property_name,
                         va_list          args)
{
    GVariantBuilder builder;
    const gchar *name;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);

    name = first_property_name;

    do {
        if (!g_strcmp0 (name, "x")) {
            int x = va_arg (args, int);
            g_variant_builder_add (&builder, "u", x);
        }
        else if (!g_strcmp0 (name, "y")) {
            int y = va_arg (args, int);
            g_variant_builder_add (&builder, "u", y);
        }
        else if (!g_strcmp0 (name, "width")) {
            int w = va_arg (args, int);
            g_variant_builder_add (&builder, "u", w);
        }
        else if (!g_strcmp0 (name, "height")) {
            int h = va_arg (args, int);
            g_variant_builder_add (&builder, "u", h);
        }
        else if (!g_strcmp0 (name, "display_name")) {
            const gchar *display_name = va_arg (args, const gchar *);
            g_variant_builder_add (&builder, "s", display_name);
        }
        else {
            g_warning ("cursor_valist_serialize() got a invalid argument: %s",
                       name);
        }
    } while ((name = va_arg (args, const gchar *)));

    return g_variant_builder_end (&builder);
}
#endif

void
ibus_input_context_set_cursor_varargs (IBusInputContext *context,
                                       const gchar      *first_property_name,
                                       ...)
{
    IBusMessage *message;
    IBusCursorLocation *cursor = NULL;
    va_list var_args;
    gboolean retval = FALSE;

    g_assert (first_property_name);

    message = ibus_proxy_create_method ((IBusProxy *) context,
                                        "SetCursorObject");

    va_start (var_args, first_property_name);

    cursor = (IBusCursorLocation *) g_object_new_valist (
            IBUS_TYPE_CURSOR_LOCATION,
            first_property_name,
            var_args);

    va_end (var_args);

    if (cursor) {
        retval = ibus_message_append_args (message,
                                           IBUS_TYPE_CURSOR_LOCATION, &cursor,
                                           G_TYPE_INVALID);
    }

    if (!retval) {
        ibus_message_unref (message);
        if (cursor)
            g_object_unref (cursor);
        g_return_if_reached ();
    }

    ibus_proxy_send ((IBusProxy *) context, message);
    ibus_message_unref (message);
}

void
ibus_input_context_set_cursor_location (IBusInputContext *context,
                                        gint32            x,
                                        gint32            y,
                                        gint32            w,
                                        gint32            h)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "SetCursorLocation",
                     G_TYPE_INT, &x,
                     G_TYPE_INT, &y,
                     G_TYPE_INT, &w,
                     G_TYPE_INT, &h,
                     G_TYPE_INVALID);
}

void
ibus_input_context_set_capabilities (IBusInputContext   *context,
                                     guint32             capabilites)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "SetCapabilities",
                     G_TYPE_UINT, &capabilites,
                     G_TYPE_INVALID);
}

void
ibus_input_context_property_activate (IBusInputContext *context,
                                      const gchar      *prop_name,
                                      gint32            state)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "PropertyActivate",
                     G_TYPE_STRING, &prop_name,
                     G_TYPE_INT, &state,
                     G_TYPE_INVALID);
}

void
ibus_input_context_property_show (IBusInputContext *context,
                                  const gchar     *prop_name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "PropertyShow",
                     G_TYPE_STRING, &prop_name,
                     G_TYPE_INVALID);
}

void
ibus_input_context_property_hide (IBusInputContext *context,
                                       const gchar      *prop_name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "PropertyHide",
                     G_TYPE_STRING, &prop_name,
                     G_TYPE_INVALID);
}

gboolean
ibus_input_context_is_enabled (IBusInputContext *context)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    gboolean retval = FALSE;

    IBusMessage *reply_message;
    IBusError *error = NULL;

    reply_message = ibus_proxy_call_with_reply_and_block ((IBusProxy *) context,
                                                          "IsEnabled",
                                                          -1,
                                                          &error,
                                                          G_TYPE_INVALID);
    if (!reply_message) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        return FALSE;
    }

    if (!ibus_message_get_args (reply_message,
                                &error,
                                G_TYPE_BOOLEAN, &retval,
                                G_TYPE_INVALID)) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        retval = FALSE;
    }
    ibus_message_unref (reply_message);

    return retval;
}

IBusEngineDesc *
ibus_input_context_get_engine (IBusInputContext *context)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    IBusMessage *reply_message;
    IBusError *error = NULL;
    IBusSerializable *object = NULL;

    reply_message = ibus_proxy_call_with_reply_and_block ((IBusProxy *) context,
                                                          "GetEngine",
                                                          -1,
                                                          &error,
                                                          G_TYPE_INVALID);
    if (!reply_message) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        return NULL;
    }

    if (!ibus_message_get_args (reply_message,
                                &error,
                                IBUS_TYPE_ENGINE_DESC, &object,
                                G_TYPE_INVALID)) {
        g_debug ("%s: %s", error->name, error->message);
        ibus_error_free (error);
        ibus_message_unref (reply_message);
        return NULL;
    }
    ibus_message_unref (reply_message);

    return IBUS_ENGINE_DESC (object);
}

void
ibus_input_context_set_engine (IBusInputContext *context,
                               const gchar *name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    ibus_proxy_call ((IBusProxy *) context,
                     "SetEngine",
                     G_TYPE_STRING, &name,
                     G_TYPE_INVALID);
}

#define DEFINE_FUNC(name,Name)                              \
    void                                                    \
    ibus_input_context_##name (IBusInputContext *context)   \
    {                                                       \
        g_assert (IBUS_IS_INPUT_CONTEXT (context));         \
        ibus_proxy_call ((IBusProxy *) context,             \
                         #Name,                             \
                         G_TYPE_INVALID);                   \
    }

DEFINE_FUNC(focus_in, FocusIn);
DEFINE_FUNC(focus_out, FocusOut);
DEFINE_FUNC(reset, Reset);
DEFINE_FUNC(page_up, PageUp);
DEFINE_FUNC(page_down, PageDown);
DEFINE_FUNC(cursor_up, CursorUp);
DEFINE_FUNC(cursor_down, CursorDown);
DEFINE_FUNC(enable, Enable);
DEFINE_FUNC(disable, Disable);

#undef DEFINE_FUNC
