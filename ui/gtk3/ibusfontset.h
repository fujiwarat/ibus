/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2017 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef __IBUS_HARFBUZZ_H_
#define __IBUS_HARFBUZZ_H_

/**
 * SECTION: ibusfontset
 * @short_description: Object for HarfBuzz and Fontconfig.
 * @title: IBusFontSet
 * @stability: Unstable
 *
 * IBusFontSet offers FcFontSet, glyph info with HarfBuzz and rendering
 * on Cairo context.
 * Current Pango changes fonts by emoji variants and draws the separated
 * glyphs [1] but actually the emoji characters with variants can be drawn
 * as one glyph so this class  manages Fontconfig fontsets to select a font,
 * HarfBuzz to get glyphs for emoji variants, Cairo to draw glyphs.
 *
 * [1]: https://bugzilla.gnome.org/show_bug.cgi?id=780669
 *      https://bugzilla.gnome.org/show_bug.cgi?id=781123
 */

#include <ibus.h>
#include <cairo.h>

#define IBUS_TYPE_CAIRO_LINE (ibus_cairo_line_get_type ())
#define IBUS_TYPE_REQUISITION_EX (ibus_requisition_ex_get_type ())
#define IBUS_TYPE_FONTSET (ibus_fontset_get_type ())
#define IBUS_FONTSET(obj) (G_TYPE_CHECK_INSTANCE_CAST (\
        (obj), \
        IBUS_TYPE_FONTSET, \
        IBusFontSet))
#define IBUS_FONTSET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST (\
        (klass), \
        IBUS_TYPE_FONTSET, \
        IBusFontSetClass))
#define IBUS_IS_FONTSET(obj) (G_TYPE_CHECK_INSTANCE_TYPE (\
        (obj), \
        IBUS_TYPE_FONTSET))
#define IBUS_IS_FONTSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE (\
        (klass), \
        IBUS_TYPE_FONTSET))
#define IBUS_FONTSET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS (\
        (obj), \
        IBUS_TYPE_FONTSET, \
        IBusFontSetClass))

G_BEGIN_DECLS

typedef struct _IBusGlyph IBusGlyph;
typedef struct _IBusCairoLine IBusCairoLine;
typedef struct _IBusRequisitionEx IBusRequisitionEx;
typedef struct _IBusFontSet IBusFontSet;
typedef struct _IBusFontSetPrivate IBusFontSetPrivate;
typedef struct _IBusFontSetClass IBusFontSetClass;

struct _IBusGlyph {
    unsigned long index;
    double               x;
    double               y;
};

struct _IBusCairoLine {
    IBusGlyph           *glyphs;
    guint                num_glyphs;
    cairo_scaled_font_t *scaled_font;
    gpointer             pdummy[5];
};

struct _IBusRequisitionEx {
    guint                width;
    guint                height;
    IBusCairoLine       *cairo_lines;
    gpointer             pdummy[5];
};

struct _IBusFontSet {
    IBusObject           parent_instance;
    IBusFontSetPrivate  *priv;
};

struct _IBusFontSetClass {
    IBusObjectClass parent_class;
    /* signals */
    /*< private >*/
    /* padding */
    gpointer        pdummy[10];
};

GType           ibus_cairo_line_get_type        (void) G_GNUC_CONST;

/**
 * ibus_cairo_line_copy:
 * @cairo_lines: #IBusCairoLine
 *
 * Creates a copy of @cairo_liens, which should be freed with
 * ibus_cairo_line_free(). Primarily used by language bindings,
 * not that useful otherwise (since @req can just be copied
 * by assignment in C).
 *
 * Returns: the newly allocated #IBusCairoLine, which should
 *          be freed with ibus_cairo_line_free(), or %NULL
 *          if @cairo_lines was %NULL.
 **/
IBusCairoLine * ibus_cairo_line_copy            (IBusCairoLine *cairo_lines);

/**
 * ibus_cairo_line_free:
 * @cairo_lines: #IBusCairoLine
 *
 * Free an #IBusCairoLine.
 */
void            ibus_cairo_line_free            (IBusCairoLine *cairo_lines);


GType           ibus_requisition_ex_get_type    (void) G_GNUC_CONST;

/**
 * ibus_requisition_ex_copy:
 * @req: #IBusRequisitionEx
 *
 * Creates a copy of @req, which should be freed with
 * ibus_requisition_ex_free(). Primarily used by language bindings,
 * not that useful otherwise (since @req can just be copied
 * by assignment in C).
 *
 * Returns: the newly allocated #IBusRequisitionEx, which should
 *          be freed with ibus_requisition_ex_free(), or %NULL
 *          if @req was %NULL.
 **/
IBusRequisitionEx *
                ibus_requisition_ex_copy        (IBusRequisitionEx *req);

/**
 * ibus_requisition_ex_free:
 * @req: #IBusRequisitionEx
 *
 * Free an #IBusRequisitionEx.
 */
void            ibus_requisition_ex_free        (IBusRequisitionEx *req);


GType           ibus_fontset_get_type           (void);

/**
 * ibus_fontset_new:
 * @first_property_name: 
 *
 * Creates a new #IBusFcFontSet.
 *
 * Returns: (transfer full): A newly allocated #IBusFontSet and includes
 * #FcFontSet internally. E.g. ibus_fontset_new ("family",
 * "Noto Emoji Color", "size", 16, "language", "ja-jp");
 */
IBusFontSet *   ibus_fontset_new                (const gchar
                                                           *first_property_name,
                                                                 ...);

/**
 * ibus_fontset_new_with_font:
 * @family: font family
 * @size: font size
 * @language: font language
 *
 * Creates  a new #IBusFcFontSet.
 *
 * Returns: (transfer full): A newly allocated #IBusFcFontSet and includes
 * #FcFontSet internally.
 */
IBusFontSet *   ibus_fontset_new_with_font      (const gchar    *family,
                                                 guint           size,
                                                 const gchar    *language);
/**
 * ibus_fontset_get_family:
 * @fontset: #IBusFcFontSet
 *
 * Return the base font family of #FcFontSet
 *
 * Returns: Base font family of #FcFontSet
 */
const gchar *   ibus_fontset_get_family         (IBusFontSet    *fontset);

/**
 * ibus_fontset_set_family:
 * @fontset: #IBusFcFontSet
 * @family: base font family for #FcFontSet
 *
 * Set the base font family for #FcFontSet
 */
void            ibus_fontset_set_family         (IBusFontSet    *fontset,
                                                 const gchar    *family);
/**
 * ibus_fontset_get_size:
 * @fontset: #IBusFcFontSet
 *
 * Return the font size of #FcFontSet
 *
 * Returns: Font size of #FcFontSet
 */
guint           ibus_fontset_get_size           (IBusFontSet    *fontset);

/**
 * ibus_fontset_set_size:
 * @fontset: #IBusFcFontSet
 * @size: font size for #FcFontSet
 *
 * Set the font size for #FcFontSet
 */
void            ibus_fontset_set_size           (IBusFontSet    *fontset,
                                                 guint           size);
/**
 * ibus_fontset_get_language:
 * @fontset: #IBusFcFontSet
 *
 * Return the font language of #FcFontSet
 *
 * Returns: Font language of #FcFontSet
 */
const gchar *   ibus_fontset_get_language       (IBusFontSet    *fontset);

/**
 * ibus_fontset_set_language:
 * @fontset: #IBusFcFontSet
 * @language: font langauge for #FcFontSet
 *
 * Set the font language for #FcFontSet
 */
void            ibus_fontset_set_language       (IBusFontSet    *fontset,
                                                 const gchar    *language);

/**
 * ibus_fontset_update_fcfontset:
 * @fontset: #IBusFcFontSet
 *
 * Update #FcFontSet from font family, size and langauge of @fontset.
 * Returns: %TRUE if #FcFontSet is updated. %FALSE otherwise.
 */
gboolean        ibus_fontset_update_fcfontset   (IBusFontSet    *fontset);

/**
 * ibus_fontset_get_preferred_size_hb:
 * @fontset: #IBusFcFontSet
 * @text: a string to be calculate the preferred rectangle size.
 * @widest: (out): #cairo_rectangle_int_t is updated.
 *
 * Calculate @widest for @text.
 *
 * Returns: #IBusRequisitionEx which includes the glyphs and coordinates.
 */
IBusRequisitionEx *
                ibus_fontset_get_preferred_size_hb
                                                (IBusFontSet    *fontset,
                                                 const gchar    *text,
                                                 cairo_rectangle_int_t
                                                                *widest);

/**
 * ibus_fontset_draw_cairo_lines:
 * @fontset: #IBusFcFontSet
 * @cr: #cairo_t in #GtkWidget.draw().
 * @ex: #IBusRequisitionEx which includes glyph, x, y values, char width
 *      and height.
 *
 * Draw glyphs in @ex using cairo @cr.
 */
void            ibus_fontset_draw_cairo_with_requisition_ex
                                                (IBusFontSet    *fontset,
                                                 cairo_t        *cr,
                                                 IBusRequisitionEx
                                                                *ex);

/**
 * ibus_fontset_unref:
 * @fontset: #IBusFcFontSet
 *
 * Call g_object_unref().
 * FIXME: Seems Vala needs this API.
 */
void            ibus_fontset_unref              (IBusFontSet    *fontset);

G_END_DECLS
#endif
