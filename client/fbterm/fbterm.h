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

#ifndef __IBUS_FBTERM_H_
#define __IBUS_FBTERM_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _IBusFbTerm IBusFbTerm;

/**
 * ibus_fbterm_put_im_text:
 * @ibus_fbterm: an #IBusFbTerm
 * @text: a string
 * @length: the lenght of @text
 *
 * Write @text on console.
 */
void             ibus_fbterm_put_im_text          (IBusFbTerm  *ibus_fbterm,
                                                   const char  *text,
                                                   unsigned int length);

/**
 * ibus_fbterm_sut_im_window:
 * @ibus_fbterm: an #IBusFbTerm
 * @id: an unique window id
 * @x: the coordinate x of the window
 * @y: the coordinate y of the window
 * @width: the width of the window
 * @height: the height of the window
 *
 * Set the window coordinate for an unique window id.
 */
void             ibus_fbterm_set_im_window        (IBusFbTerm    *ibus_fbterm,
                                                   unsigned int   id,
                                                   guint          x,
                                                   guint          y,
                                                   guint          width,
                                                   guint          height);

/**
 * ibus_fbterm_sut_im_window:
 * @ibus_fbterm: an #IBusFbTerm
 * @x: the coordinate x of the rectangle
 * @y: the coordinate y of the rectangle
 * @width: the width of the rectangle
 * @height: the height of the rectangle
 * @color: the color of the rectangle
 *
 * Fill a rect with a color.
 */
void             ibus_fbterm_fill_rect            (IBusFbTerm    *ibus_fbterm,
                                                   guint          x,
                                                   guint          y,
                                                   guint          width,
                                                   guint          height,
                                                   unsigned char  color);

/**
 * ibus_fbterm_draw_text:
 * @ibus_fbterm: an #IBusFbTerm
 * @x: coordinate x
 * @y: coordinate y
 * @fc: foreground color
 * @bc: background color
 * @text: an drawn text
 * @length: the length of @text
 *
 * Draw the text on console.
 */
void             ibus_fbterm_draw_text            (IBusFbTerm    *ibus_fbterm,
                                                   guint16        x,
                                                   guint16        y,
                                                   unsigned char  fc,
                                                   unsigned char  bc,
                                                   const gchar   *text,
                                                   guint16        length);

G_END_DECLS
#endif
