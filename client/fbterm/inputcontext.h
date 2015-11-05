/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FBTERM_INPUT_CONTEXT_H_
#define __FBTERM_INPUT_CONTEXT_H_

#include <glib-object.h>
#include <ibus.h>


/*
 * Type macros.
 */

/* define GOBJECT macros */
#define FBTERM_TYPE_INPUT_CONTEXT               (fbterm_input_context_get_type ())
#define FBTERM_INPUT_CONTEXT(o)                 (G_TYPE_CHECK_INSTANCE_CAST ((o), FBTERM_TYPE_INPUT_CONTEXT, FbTermInputContext))
#define FBTERM_INPUT_CONTEXT_CLASS(k)           (G_TYPE_CHECK_CLASS_CAST ((k), FBTERM_TYPE_INPUT_CONTEXT, FbTermInputContextClass))
#define FBTERM_IS_INPUT_CONTEXT(o)              (G_TYPE_CHECK_INSTANCE_TYPE ((o), FBTERM_TYPE_INPUT_CONTEXT))
#define FBTERM_IS_INPUT_CONTEXT_CLASS(k)        (G_TYPE_CHECK_CLASS_TYPE ((k), FBTERM_TYPE_INPUT_CONTEXT))


G_BEGIN_DECLS
typedef struct _FbTermInputContext FbTermInputContext;
typedef struct _FbTermInputContextPrivate FbTermInputContextPrivate;
typedef struct _FbTermInputContextClass FbTermInputContextClass;


/**
 * FbTermInputContext:
 *
 * <structname>FbTermInputContext</structname> provides a key structure
 * related with the frame buffer.
 */
struct _FbTermInputContext {
    IBusInputContext           parent;

    FbTermInputContextPrivate *priv;
};

struct _FbTermInputContextClass {
    IBusInputContextClass parent;
};

FbTermInputContext *fbterm_input_context_new
                                            (IBusFbTerm *ibus_fbterm,
                                             IBusBus    *bus);
void                fbterm_input_context_update_info
                                            (FbTermInputContext *input_context,
                                             Info               *info);
void                fbterm_input_context_update_cursor_position
                                            (FbTermInputContext *input_context,
                                             guint16             x,
                                             guint16             y);
void                fbterm_input_context_calculate_preedit_window
                                            (FbTermInputContext *input_context);

G_END_DECLS
#endif
