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

#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glib.h>
#include <hb-ot.h>
#include <pango/pango.h>

#include "ibusfontset.h"

#define XPAD 2
#define YPAD 2
#define UNKNOWN_FONT_SIZE 7
#define IBUS_FONTSET_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_FONTSET, IBusFontSetPrivate))
#define MONOSPACE "monospace"
#define SERIF     "serif"
#define SANS      "sans"

typedef struct _FcFontSetEx {
    int                 nfont;
    int                 sfont;
    FcPattern         **fonts;
    FT_Face            *ft_faces;
} FcFontSetEx;

static gboolean         m_color_supported;
static FT_Library       m_ftlibrary;
static FcFontSetEx     *m_fcfontset;
static gchar           *m_family;
static guint            m_size;
static gchar           *m_language;
static GHashTable      *m_font_index_per_char_table;
static GHashTable      *m_scaled_font_table;
static GHashTable      *m_hb_font_table;

enum {
    PROP_0,
    PROP_FAMILY,
    PROP_SIZE,
    PROP_LANGUAGE
};

typedef struct {
    gunichar   ch;
    FcPattern *fcfont;
} FontPerChar;

struct _IBusFontSetPrivate {
    gchar     *family;
    guint      size;
    gchar     *language;
};

static GObject * ibus_fontset_constructor   (GType                  type,
                                             guint                  n,
                                             GObjectConstructParam *args);
static void      ibus_fontset_destroy       (IBusFontSet           *fontset);
static void      ibus_fontset_set_property  (IBusFontSet           *fontset,
                                             guint                  prop_id,
                                             const GValue          *value,
                                             GParamSpec            *pspec);
static void      ibus_fontset_get_property  (IBusFontSet           *fontset,
                                             guint                  prop_id,
                                             GValue                *value,
                                             GParamSpec            *pspec);
static cairo_scaled_font_t *
                 ibus_fontset_cairo_scaled_font_new_with_font
                                            (const gchar           *family,
                                             guint                  size,
                                             gboolean               has_color);

G_DEFINE_BOXED_TYPE (IBusCairoLine,
                     ibus_cairo_line,
                     ibus_cairo_line_copy,
                     ibus_cairo_line_free);
G_DEFINE_BOXED_TYPE (IBusRequisitionEx,
                     ibus_requisition_ex,
                     ibus_requisition_ex_copy,
                     ibus_requisition_ex_free);
G_DEFINE_TYPE (IBusFontSet, ibus_fontset, IBUS_TYPE_OBJECT)

static void
ibus_fontset_class_init (IBusFontSetClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    cairo_glyph_t dummy;
    IBusGlyph dummy2;

    m_color_supported = (FcGetVersion () >=  21205);
    gobject_class->constructor = ibus_fontset_constructor;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_fontset_get_property;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_fontset_set_property;
    object_class->destroy = (IBusObjectDestroyFunc) ibus_fontset_destroy;

    /* install properties */
    /**
     * IBusFontSet:family:
     *
     * Font family of this IBusFontSet.
     */
    g_object_class_install_property (gobject_class,
                    PROP_FAMILY,
                    g_param_spec_string ("family",
                        "family",
                        "family",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusFontSet:size:
     *
     * Font size of this IBusFontSet.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SIZE,
                    g_param_spec_uint ("size",
                        "size",
                        "size",
                        0, G_MAXUINT16, 0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusFontSet:language:
     *
     * Font language of this IBusFontSet.
     */
    g_object_class_install_property (gobject_class,
                    PROP_LANGUAGE,
                    g_param_spec_string ("language",
                        "language",
                        "language",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_type_class_add_private (class, sizeof (IBusFontSetPrivate));
    FT_Init_FreeType (&m_ftlibrary);
    m_scaled_font_table = g_hash_table_new_full (
            g_str_hash, g_str_equal,
            g_free,
            (GDestroyNotify) cairo_scaled_font_destroy);
    m_hb_font_table = g_hash_table_new_full (
            g_str_hash, g_str_equal,
            g_free,
            (GDestroyNotify) hb_font_destroy);

    /* hb_glyph_t is not available in Vala so override it with IBusGlyph. */
    g_assert (sizeof (dummy) == sizeof (dummy2));
    g_assert (sizeof (dummy.index) == sizeof (dummy2.index));
    g_assert (sizeof (dummy.x) == sizeof (dummy2.x));
    g_assert (sizeof (dummy.y) == sizeof (dummy2.y));
}

static void
ibus_fontset_init (IBusFontSet *fontset)
{
    fontset->priv = IBUS_FONTSET_GET_PRIVATE (fontset);
}


static GObject *
ibus_fontset_constructor (GType                   type,
                          guint                   n,
                          GObjectConstructParam  *args)
{
    GObject *object;
    IBusFontSet *fontset;
    const gchar *family;
    guint size;

    object = G_OBJECT_CLASS (ibus_fontset_parent_class)->constructor (
            type, n ,args);
    fontset = IBUS_FONTSET (object);
    family = ibus_fontset_get_family (fontset);
    size = ibus_fontset_get_size (fontset);
    ibus_fontset_update_fcfontset (fontset);
    if (family != NULL && size > 0) {
        /* cache the font */
        ibus_fontset_cairo_scaled_font_new_with_font (family,
                                                      size,
                                                      TRUE);
    }
    return object;
}

static void
ibus_fontset_destroy (IBusFontSet *fontset)
{
    g_clear_pointer (&fontset->priv->family, g_free);
    g_clear_pointer (&fontset->priv->language, g_free);
}

static void
ibus_fontset_set_property (IBusFontSet  *fontset,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_FAMILY:
        ibus_fontset_set_family (fontset, g_value_get_string (value));
        break;
    case PROP_SIZE:
        ibus_fontset_set_size (fontset, g_value_get_uint (value));
        break;
    case PROP_LANGUAGE:
        ibus_fontset_set_language (fontset, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (fontset, prop_id, pspec);
    }
}

static void
ibus_fontset_get_property (IBusFontSet *fontset,
                           guint          prop_id,
                           GValue        *value,
                           GParamSpec    *pspec)
{
    switch (prop_id) {
    case PROP_FAMILY:
        g_value_set_string (value, ibus_fontset_get_family (fontset));
        break;
    case PROP_SIZE:
        g_value_set_uint (value, ibus_fontset_get_size (fontset));
        break;
    case PROP_LANGUAGE:
        g_value_set_string (value, ibus_fontset_get_language (fontset));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (fontset, prop_id, pspec);
    }
}

static cairo_scaled_font_t *
ibus_fontset_cairo_scaled_font_new_with_font (const gchar *family,
                                              guint        size,
                                              gboolean     has_color)
{
    gchar *font_name;
    cairo_scaled_font_t *scaled_font = NULL;
    FcPattern *pattern, *resolved;
    FcResult result;
    cairo_font_options_t *font_options;
    double pixel_size = 0.;
    FcMatrix fc_matrix, *fc_matrix_val;
    cairo_font_face_t *cairo_face = NULL;
    cairo_matrix_t font_matrix;
    cairo_matrix_t ctm;
    int i;

    g_return_val_if_fail (family != NULL, NULL);
    g_return_val_if_fail (m_scaled_font_table != NULL, NULL);

    if (m_color_supported) {
        font_name = g_strdup_printf ("%s %u:color=%s",
                                     family, size,
                                     has_color ? "TRUE" : "FALSE");
    } else {
        font_name = g_strdup_printf ("%s %u", family, size);
    }
    scaled_font = g_hash_table_lookup (m_scaled_font_table, font_name);
    if (scaled_font != NULL) {
        g_free (font_name);
        return scaled_font;
    }
    pattern = FcPatternCreate ();
    FcPatternAddString (pattern, FC_FAMILY, (FcChar8*) family);
    FcPatternAddDouble (pattern, FC_SIZE, (double) size);
/* FC_VERSION is for the build check of FC_COLOR and m_color_supported is
 * for the runtime check.
 */
#if FC_VERSION >=  21205
    if (m_color_supported)
        FcPatternAddBool (pattern, FC_COLOR, has_color);
#endif
    FcPatternAddDouble (pattern, FC_DPI, 96);
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    font_options = cairo_font_options_create ();
    cairo_ft_font_options_substitute (font_options, pattern);
    FcDefaultSubstitute (pattern);
    resolved = FcFontMatch (NULL, pattern, &result);
    FcPatternDestroy (pattern);
    FcPatternGetDouble (resolved, FC_PIXEL_SIZE, 0, &pixel_size);
    if (pixel_size == 0.)
        g_warning ("Failed to scaled the font: %s %u", family, size);
    cairo_face = cairo_ft_font_face_create_for_pattern (resolved);
    FcMatrixInit (&fc_matrix);
    for (i = 0;
         FcPatternGetMatrix (resolved, FC_MATRIX, i, &fc_matrix_val)
                 == FcResultMatch;
         i++) {
        FcMatrixMultiply (&fc_matrix, &fc_matrix, fc_matrix_val);
    }
    FcPatternDestroy (resolved);
    cairo_matrix_init (&font_matrix,
                       fc_matrix.xx, -fc_matrix.yx,
                       -fc_matrix.xy, fc_matrix.yy,
                       0., 0.);
    if (pixel_size != 0.)
        cairo_matrix_scale (&font_matrix, pixel_size, pixel_size);
    cairo_matrix_init_identity (&ctm);
    scaled_font = cairo_scaled_font_create (cairo_face,
                                            &font_matrix, &ctm,
                                            font_options);
    cairo_font_face_destroy (cairo_face);
    if (font_name)
        g_hash_table_insert(m_scaled_font_table, font_name, scaled_font);

    return scaled_font;
}

static hb_font_t *
ibus_fontset_hb_font_new_with_font_path (const gchar *font_path)
{
    hb_font_t *hb_font;
    GError *error = NULL;
    GMappedFile *mf;
    char *font_data = NULL;
    gsize len;
    hb_blob_t *hb_blob;
    hb_face_t *hb_face;

    g_return_val_if_fail (font_path != NULL, NULL);
    g_return_val_if_fail (m_hb_font_table != NULL, NULL);

    hb_font = g_hash_table_lookup (m_hb_font_table, font_path);
    if (hb_font != NULL)
        return hb_font;

    mf = g_mapped_file_new (font_path, FALSE, &error);
    if (mf == NULL) {
        g_warning ("Not found font %s", font_path);
        return NULL;
    }
    font_data = g_mapped_file_get_contents (mf);
    len = g_mapped_file_get_length (mf);
    if (len == 0) {
        g_warning ("zero size font %s", font_path);
        g_mapped_file_unref (mf);
        return NULL;
    }
    hb_blob = hb_blob_create (font_data, len,
                              HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
                              mf, (hb_destroy_func_t)g_mapped_file_unref);
    hb_face = hb_face_create (hb_blob, 0);
    hb_blob_destroy (hb_blob);
    hb_font = hb_font_create (hb_face);
    unsigned int upem = hb_face_get_upem (hb_face);
    hb_font_set_scale (hb_font, upem, upem);
    hb_face_destroy (hb_face);
    hb_ot_font_set_funcs (hb_font);
    g_hash_table_insert (m_hb_font_table, g_strdup (font_path), hb_font);

    return hb_font;
}

static void
get_font_extents_with_scaled_font (cairo_scaled_font_t *scaled_font,
                                   PangoRectangle      *font_rect)
{
    cairo_font_extents_t font_extents;

    g_assert (scaled_font != NULL && font_rect != NULL);

    cairo_scaled_font_extents (scaled_font, &font_extents);
    font_rect->x = 0;
    font_rect->y = - pango_units_from_double (font_extents.ascent);
    font_rect->width = 0;
    font_rect->height = pango_units_from_double (
            font_extents.ascent + font_extents.descent);
}

static void
get_glyph_extents_with_scaled_hb_font (const gchar          *str,
                                       cairo_scaled_font_t  *scaled_font,
                                       hb_font_t            *hb_font,
                                       PangoRectangle       *font_rect,
                                       IBusCairoLine       **cairo_lines,
                                       FcChar8              *fallback_family)
{
    gboolean has_unknown_glyph = FALSE;
    hb_buffer_t *hb_buffer;
    unsigned int len, n, i;
    hb_glyph_info_t *info;
    hb_glyph_position_t *pos;
    double x;
    cairo_glyph_t *glyph;
    cairo_text_extents_t text_extents = { 0, };

    g_return_if_fail (str != NULL);

    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, str, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);
    for (n = 0; *cairo_lines && (*cairo_lines)[n].scaled_font; n++);
    if (n == 0)
        *cairo_lines = g_new0 (IBusCairoLine, 2);
    else
        *cairo_lines = g_renew (IBusCairoLine, *cairo_lines, n + 2);
    (*cairo_lines)[n + 1].scaled_font = NULL;
    (*cairo_lines)[n + 1].num_glyphs = 0;
    (*cairo_lines)[n + 1].glyphs = NULL;
    hb_shape (hb_font, hb_buffer, NULL, 0);
    len = hb_buffer_get_length (hb_buffer);
    info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
    (*cairo_lines)[n].scaled_font = scaled_font;
    (*cairo_lines)[n].num_glyphs = len;
    (*cairo_lines)[n].glyphs = (IBusGlyph*) cairo_glyph_allocate (len + 1);
    x = 0.;
    for (i = 0; i < len; i++) {
        hb_codepoint_t c = info[i].codepoint;
        if (c) {
            (*cairo_lines)[n].glyphs[i].index = info[i].codepoint;
            (*cairo_lines)[n].glyphs[i].x = x;
            (*cairo_lines)[n].glyphs[i].y = -font_rect->y / PANGO_SCALE;
            glyph = (cairo_glyph_t *) &((*cairo_lines)[n].glyphs[i]);
            cairo_scaled_font_glyph_extents (scaled_font, glyph,
                                             1, &text_extents);
            x += text_extents.width;
        } else {
            has_unknown_glyph = TRUE;
            c = g_utf8_get_char (str);
            (*cairo_lines)[n].glyphs[i].index = PANGO_GET_UNKNOWN_GLYPH (c);
            (*cairo_lines)[n].glyphs[i].x = x;
            (*cairo_lines)[n].glyphs[i].y = -font_rect->y / PANGO_SCALE;
            glyph = (cairo_glyph_t *) &((*cairo_lines)[n].glyphs[i]);
            cairo_scaled_font_glyph_extents (scaled_font, glyph,
                                             1, &text_extents);
            x += 10;
        }
    }
    (*cairo_lines)[n].glyphs[i].index = -1;
    (*cairo_lines)[n].glyphs[i].x = 0;
    (*cairo_lines)[n].glyphs[i].y = 0;
    glyph = (cairo_glyph_t *) (*cairo_lines)[n].glyphs;
    cairo_scaled_font_glyph_extents (scaled_font, glyph,
                                     len, &text_extents);
    if (text_extents.width) {
        font_rect->width = pango_units_from_double (text_extents.width);
    } else {
        font_rect->width = font_rect->height;
    }
    if (has_unknown_glyph && fallback_family != NULL) {
        cairo_scaled_font_t  *unknown_font;
        unknown_font = ibus_fontset_cairo_scaled_font_new_with_font (
            (const gchar *) fallback_family,
            UNKNOWN_FONT_SIZE,
            FALSE);
        (*cairo_lines)[n].scaled_font = unknown_font;
    }
    hb_buffer_destroy (hb_buffer);
}

static void
get_string_extents_with_font (const gchar            *str,
                              FontPerChar            *buff,
                              cairo_rectangle_int_t  *rect,
                              IBusCairoLine         **cairo_lines)
{
    FcChar8 *family = NULL;
    FcChar8 *font_path = NULL;
    gboolean has_color = TRUE;
    guint size = 0;
    cairo_scaled_font_t *scaled_font = NULL;
    PangoRectangle font_rect = { 0, };
    hb_font_t *hb_font;

    g_return_if_fail (str != NULL);
    g_return_if_fail (buff != NULL && buff->fcfont != NULL);

    FcPatternGetString (buff->fcfont, FC_FAMILY, 0, &family);
    g_return_if_fail (family != NULL);
#if FC_VERSION >=  21205
    if (m_color_supported)
        FcPatternGetBool (buff->fcfont, FC_COLOR, 0, &has_color);
#endif
    size = m_size;
    if (size == 0) {
        g_warning ("Font size is not right for font %s.", family);
        size = 14;
    }
    scaled_font = ibus_fontset_cairo_scaled_font_new_with_font (
            (const gchar *) family,
            size,
            has_color);
    g_return_if_fail (scaled_font != NULL);
    get_font_extents_with_scaled_font (scaled_font, &font_rect);

    FcPatternGetString (buff->fcfont, FC_FILE, 0, &font_path);
    g_return_if_fail (font_path != NULL);
    hb_font = ibus_fontset_hb_font_new_with_font_path (
            (const gchar *) font_path);
    if (hb_font == NULL)
        return;
    get_glyph_extents_with_scaled_hb_font (str,
                                           scaled_font,
                                           hb_font,
                                           &font_rect,
                                           cairo_lines,
                                           family);
    rect->width += font_rect.width / PANGO_SCALE;
    rect->height += font_rect.height / PANGO_SCALE;
}

static FT_Face
ibus_fontset_get_ftface_from_fcfont (IBusFontSet *fontset,
                                     FcPattern   *fcfont)
{
    FcChar8 *font_file = NULL;
    FT_Face ft_face;
    guint size = ibus_fontset_get_size (fontset);

    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), NULL);
    g_return_val_if_fail (fcfont != NULL, NULL);

    size = ibus_fontset_get_size (fontset);
    FcPatternGetString (fcfont, FC_FILE, 0, &font_file);
    FT_New_Face (m_ftlibrary, (const gchar *) font_file, 0, &ft_face);
    FT_Set_Pixel_Sizes (ft_face, size, size);
    return ft_face;
}

static void
_cairo_show_unknown_glyphs (cairo_t             *cr,
                            const cairo_glyph_t *glyphs,
                            guint                num_glyphs,
                            guint                width,
                            guint                height)
{
    gunichar ch;
    gboolean invalid_input;
    int rows = 2;
    int cols;
    int row, col;
    char buf[7];
    double cx = 0.;
    double cy;
    const double box_descent = 3.;
    double x0;
    double y0;
    const double digit_width = 5.;
    const double digit_height= 6.;
    char hexbox_string[2] = {0, 0};

    g_assert (glyphs != NULL);
    g_assert (num_glyphs > 0);

    ch = glyphs[0].index & ~PANGO_GLYPH_UNKNOWN_FLAG;
    invalid_input = G_UNLIKELY (glyphs[0].index == PANGO_GLYPH_INVALID_INPUT ||
                                ch > 0x10FFFF);
    if (G_UNLIKELY (invalid_input)) {
      g_warning ("Unsupported U+%06X",  ch);
      return;
    }

    cairo_save (cr);

    cols = (ch > 0xffff ? 6 : 4) / rows;
    g_snprintf (buf, sizeof(buf), (ch > 0xffff) ? "%06X" : "%04X", ch);
    cy = (double) height;
    x0 = cx + box_descent + XPAD / 2;
    y0 = cy - box_descent - YPAD / 2;

    for (row = 0; row < rows; row++) {
        double y = y0 - (rows - 1 - row) * (digit_height + YPAD);
        for (col = 0; col < cols; col++) {
            double x = x0 + col * (digit_width + XPAD);
            cairo_move_to (cr, x, y);
            hexbox_string[0] = buf[row * cols + col];
            cairo_show_text (cr, hexbox_string);
        }
    }
    cairo_move_to (cr, XPAD, YPAD);
    cairo_line_to (cr, width - XPAD, YPAD);
    cairo_line_to (cr, width - XPAD,
                       height - YPAD);
    cairo_line_to (cr, XPAD, height - YPAD);
    cairo_line_to (cr, XPAD, YPAD);
    cairo_set_line_width (cr, 1.);
    cairo_stroke (cr);

    cairo_restore (cr);
}

static void
ibus_fcfontset_destroy_ex (FcFontSetEx *fcfontset_ex)
{
    FcFontSet *fcfontset = NULL;
    int i;

    g_return_if_fail (fcfontset_ex != NULL);

    for (i = 0; i < fcfontset_ex->nfont; i++)
        FT_Done_Face (fcfontset_ex->ft_faces[i]);
    fcfontset = FcFontSetCreate ();
    fcfontset->nfont = fcfontset_ex->nfont;
    fcfontset->sfont = fcfontset_ex->sfont;
    fcfontset->fonts = fcfontset_ex->fonts;
    FcFontSetDestroy (fcfontset);
    g_free (fcfontset_ex->ft_faces);
    g_free (fcfontset_ex);
}

IBusCairoLine *
ibus_cairo_line_copy (IBusCairoLine *cairo_lines)
{
    IBusCairoLine *ret;
    guint n, i, j, num_glyphs;
    if (!cairo_lines)
        return NULL;

    for (n = 0; cairo_lines[n].scaled_font; n++);
    ret = g_new0 (IBusCairoLine, n + 1);
    for (i = 0; i < n; i++) {
        ret[i].scaled_font = cairo_lines[i].scaled_font;
        num_glyphs = cairo_lines[i].num_glyphs;
        ret[i].num_glyphs = num_glyphs;
        ret[i].glyphs = (IBusGlyph *) cairo_glyph_allocate (num_glyphs + 1);
        for (j = 0; j < num_glyphs; j++) {
            ret[i].glyphs[j] = cairo_lines[i].glyphs[j];
        }
        ret[i].glyphs[j].index = -1;
        ret[i].glyphs[j].x = 0;
        ret[i].glyphs[j].y = 0;
    }
    ret[i].scaled_font = NULL;
    ret[i].num_glyphs = 0;
    ret[i].glyphs = NULL;
    return ret;
}

void
ibus_cairo_line_free (IBusCairoLine *cairo_lines)
{
    guint i;
    if (!cairo_lines)
        return;
    for (i = 0; cairo_lines[i].scaled_font; i++) {
        g_free (cairo_lines[i].glyphs);
    }
    g_free (cairo_lines);
}

IBusRequisitionEx *
ibus_requisition_ex_copy (IBusRequisitionEx *req)
{
    IBusRequisitionEx *ret;
    if (!req)
        return NULL;
    ret = g_new0 (IBusRequisitionEx, 1);
    ret->width = req->width;
    ret->height = req->height;
    ret->cairo_lines = ibus_cairo_line_copy (req->cairo_lines);
    return ret;
}

void
ibus_requisition_ex_free (IBusRequisitionEx *req)
{
    if (!req)
        return;
    g_clear_pointer (&req->cairo_lines, ibus_cairo_line_free);
    g_free (req);
}

IBusFontSet *
ibus_fontset_new (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusFontSet *fontset;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    fontset = (IBusFontSet *)g_object_new_valist (IBUS_TYPE_FONTSET,
                                                  first_property_name,
                                                  var_args);
    va_end (var_args);
    g_assert (fontset->priv->family);
    g_assert (fontset->priv->language);
    return fontset;
}

IBusFontSet *
ibus_fontset_new_with_font (const gchar *family,
                            guint        size,
                            const gchar *language)
{
    return ibus_fontset_new ("family", family,
                             "size", size,
                             "language", language,
                             NULL);
}

void
ibus_fontset_exit ()
{
    g_clear_pointer (&m_ftlibrary, FT_Done_FreeType);
}

const gchar *
ibus_fontset_get_family (IBusFontSet *fontset)
{
    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), NULL);
    return fontset->priv->family;
}

void
ibus_fontset_set_family (IBusFontSet *fontset,
                         const gchar   *family)
{
    g_return_if_fail (IBUS_IS_FONTSET (fontset));
    g_free (fontset->priv->family);
    fontset->priv->family = g_strdup (family);
}

guint
ibus_fontset_get_size (IBusFontSet *fontset)
{
    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), 0);
    return fontset->priv->size;
}

void
ibus_fontset_set_size (IBusFontSet *fontset,
                       guint          size)
{
    g_return_if_fail (IBUS_IS_FONTSET (fontset));
    fontset->priv->size = size;
}

const gchar *
ibus_fontset_get_language (IBusFontSet *fontset)
{
    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), NULL);
    return fontset->priv->language;
}

void
ibus_fontset_set_language (IBusFontSet *fontset,
                           const gchar *language)
{
    g_return_if_fail (IBUS_IS_FONTSET (fontset));
    g_free (fontset->priv->language);
    fontset->priv->language = g_strdup (language);
}

gboolean
ibus_fontset_update_fcfontset (IBusFontSet *fontset)
{
    FcPattern *pattern;
    const gchar *family;
    guint size;
    const gchar *language;
    gboolean update_fontset = FALSE;
    FcFontSet *fcfontset = NULL;
    FcResult result;

    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), FALSE);

    pattern = FcPatternCreate ();
    family = fontset->priv->family;
    size = fontset->priv->size;
    language = fontset->priv->language;

    if (g_strcmp0 (m_family, family)) {
        g_free (m_family);
        m_family = g_strdup (family);
        update_fontset = TRUE;
    }
    if (m_size != size) {
        m_size = size;
        update_fontset = TRUE;
    }
    if (g_strcmp0 (m_language, language)) {
        g_free (m_language);
        m_language = g_strdup (language);
        update_fontset = TRUE;
    }
    if (!update_fontset && m_fcfontset != NULL)
        return FALSE;

    if (m_font_index_per_char_table)
        g_hash_table_destroy (m_font_index_per_char_table);
    if (m_fcfontset)
        g_clear_pointer (&m_fcfontset, ibus_fcfontset_destroy_ex);

    m_font_index_per_char_table = g_hash_table_new (g_direct_hash,
                                                    g_direct_equal);
    if (g_strcmp0 (family, ""))
        FcPatternAddString (pattern, FC_FAMILY, (const FcChar8*) family);
    if (size > 0)
        FcPatternAddDouble (pattern, FC_SIZE, (double) size);
    if (g_strcmp0 (language, ""))
        FcPatternAddString (pattern, FC_LANG, (const FcChar8*) language);
    FcPatternAddInteger (pattern, FC_WEIGHT, FC_WEIGHT_NORMAL);
    FcPatternAddInteger (pattern, FC_WIDTH, FC_WIDTH_NORMAL);
    FcPatternAddInteger (pattern, FC_DPI, 96);
#if FC_VERSION >=  21205
    if (m_color_supported &&
        (!g_ascii_strncasecmp (family, MONOSPACE, strlen (MONOSPACE)) ||
         !g_ascii_strncasecmp (family, SERIF, strlen (SERIF)) ||
         !g_ascii_strncasecmp (family, SANS, strlen (SANS)))) {
        FcPatternAddBool (pattern, FC_COLOR, TRUE);
    }
#endif
    FcConfigSubstitute (NULL, pattern, FcMatchPattern);
    FcConfigSubstitute (NULL, pattern, FcMatchFont);
    FcDefaultSubstitute (pattern);
    fcfontset = FcFontSort (NULL, pattern, FcTrue, NULL, &result);
    FcPatternDestroy (pattern);
    if (result == FcResultNoMatch || fcfontset->nfont == 0) {
        g_warning ("No FcFontSet for %s", family ? family : "(null)");
        return FALSE;
    }
    m_fcfontset = g_new0 (FcFontSetEx, 1);
    m_fcfontset->nfont = fcfontset->nfont;
    m_fcfontset->sfont = fcfontset->sfont;
    m_fcfontset->fonts = fcfontset->fonts;
    m_fcfontset->ft_faces = g_new0 (FT_Face, fcfontset->nfont);
    fcfontset->nfont = 0;
    fcfontset->sfont = 0;
    fcfontset->fonts = NULL;
    FcFontSetDestroy (fcfontset);
    return TRUE;
}

void
ibus_fontset_unref (IBusFontSet *fontset)
{
    g_object_unref (fontset);
}

IBusRequisitionEx *
ibus_fontset_get_preferred_size_hb (IBusFontSet          *fontset,
                                    const gchar           *text,
                                    cairo_rectangle_int_t *widest)
{
    gchar *copied_text;
    gchar *p;
    FontPerChar *buff;
    IBusCairoLine *cairo_lines = NULL;
    IBusRequisitionEx *req = NULL;
    GString *str = NULL;
    int text_length;
    int i, n = 0;

    g_return_val_if_fail (IBUS_IS_FONTSET (fontset), NULL);
    g_return_val_if_fail (m_fcfontset != NULL, NULL);

    copied_text = g_strdup (text);
    text_length =  g_utf8_strlen (text, -1);
    buff = g_slice_alloc0 (sizeof (FontPerChar) * text_length);
    str = g_string_new (NULL);

    for (p = copied_text; *p != '\0'; p = g_utf8_next_char (p)) {
        gunichar c = g_utf8_get_char (p);
        gboolean has_glyphs = FALSE;
        buff[n].ch = c;
        if ((c == 0xfe0eu || c == 0xfe0fu) && n > 0) {
            buff[n].fcfont = buff[n-1].fcfont;
            ++n;
            continue;
        }
        i = GPOINTER_TO_INT (g_hash_table_lookup (m_font_index_per_char_table,
                                                  GINT_TO_POINTER (c)));
        if (i > 0) {
            i--;
            if (i >= m_fcfontset->nfont) {
                g_warning ("i:%d >= m_fcfontset->nfont:%d",
                           i, m_fcfontset->nfont);
            } else {
                buff[n].fcfont = m_fcfontset->fonts[i];
                has_glyphs = TRUE;
            }
        }
        for (; i < m_fcfontset->nfont; i++) {
            if (!has_glyphs && g_unichar_iscntrl (c) && !g_unichar_isspace (c))
                break;
            FT_Face ft_face = m_fcfontset->ft_faces[i];
            if (!has_glyphs && ft_face == 0) {
               ft_face = ibus_fontset_get_ftface_from_fcfont (
                       fontset,
                       m_fcfontset->fonts[i]);
               m_fcfontset->ft_faces[i] = ft_face;
            }
            if (has_glyphs || FT_Get_Char_Index (ft_face, c) != 0) {
                FcChar8 *font_file = NULL;
                FcPatternGetString (m_fcfontset->fonts[i], FC_FILE, 0, &font_file);
                buff[n].fcfont = m_fcfontset->fonts[i];
                if (!has_glyphs) {
                    g_hash_table_insert (m_font_index_per_char_table,
                                         GINT_TO_POINTER (c),
                                         GINT_TO_POINTER (i + 1));
                }
                if (n > 0 && buff[n - 1].fcfont != buff[n].fcfont) {
                    get_string_extents_with_font (str->str,
                                                  &buff[n - 1],
                                                  widest,
                                                  &cairo_lines);
                    g_string_free (str, TRUE);
                    str = g_string_new (NULL);
                    g_string_append_unichar (str, c);
                } else {
                    g_string_append_unichar (str, c);
                }
                ++n;
                has_glyphs = TRUE;
                break;
            }
        }
        if (!has_glyphs) {
            if (n > 0) {
                buff[n].fcfont = buff[n - 1].fcfont;
            } else {
                /* Search a font for non-glyph char to draw the code points
                 * likes Pango.
                 */
                for (i = 0; i < m_fcfontset->nfont; i++) {
                    FT_Face ft_face = m_fcfontset->ft_faces[i];
                    if (ft_face == 0) {
                        ft_face = ibus_fontset_get_ftface_from_fcfont (
                                fontset,
                                m_fcfontset->fonts[i]);
                        m_fcfontset->ft_faces[i] = ft_face;
                    }
                    /* Check alphabets instead of space or digits
                     * because 'Noto Emoji Color' font's digits are
                     * white color and cannot change the font color.
                     * the font does not have alphabets.
                     */
                    if (FT_Get_Char_Index (ft_face, 'A') != 0) {
                        FcChar8 *font_file = NULL;
                        FcPatternGetString (m_fcfontset->fonts[i], FC_FILE, 0, &font_file);
                        buff[n].fcfont = m_fcfontset->fonts[i];
                        g_hash_table_insert (m_font_index_per_char_table,
                                             GINT_TO_POINTER (c),
                                             GINT_TO_POINTER (i + 1));
                        has_glyphs = TRUE;
                        break;
                    }
                }
                if (!has_glyphs) {
                    buff[n].fcfont = m_fcfontset->fonts[0];
                    g_hash_table_insert (m_font_index_per_char_table,
                                         GINT_TO_POINTER (c),
                                         GINT_TO_POINTER (1));
                    g_warning ("Not found fonts for unicode %04X at %d in %s",
                               c, n, text);
                }
            }
            n++;
            g_string_append_unichar (str, c);
        }
    }
    if (str->str) {
        get_string_extents_with_font (str->str,
                                      &buff[n - 1],
                                      widest,
                                      &cairo_lines);
        g_string_free (str, TRUE);
    }
    g_slice_free1 (sizeof (FontPerChar) * text_length, buff);
    g_free (copied_text);
    widest->width += XPAD * 2;
    widest->height += YPAD * 2;
    req = g_new0 (IBusRequisitionEx, 1);
    req->width = widest->width;
    req->height = widest->height;
    req->cairo_lines = cairo_lines;
    return req;
}

void
ibus_fontset_draw_cairo_with_requisition_ex (IBusFontSet       *fontset,
                                             cairo_t           *cr,
                                             IBusRequisitionEx *ex)
{
    IBusCairoLine *cairo_lines;
    int i;

    g_return_if_fail (IBUS_IS_FONTSET (fontset));
    g_return_if_fail (cr != NULL);
    g_return_if_fail (ex != NULL);

    cairo_lines = ex->cairo_lines;
    g_return_if_fail (cairo_lines != NULL);

    for (i = 0; cairo_lines[i].scaled_font; i++) {
        const cairo_glyph_t *glyphs = (cairo_glyph_t *) cairo_lines[i].glyphs;
        guint num_glyphs = cairo_lines[i].num_glyphs;

        cairo_ft_scaled_font_lock_face (cairo_lines[i].scaled_font);
        cairo_set_scaled_font (cr, cairo_lines[i].scaled_font);
        if (num_glyphs > 0 && glyphs[0].index & PANGO_GLYPH_UNKNOWN_FLAG) {
            _cairo_show_unknown_glyphs (cr, glyphs, num_glyphs,
                                        ex->width, ex->height);
        } else {
            cairo_show_glyphs (cr, glyphs, num_glyphs);
        }
        cairo_ft_scaled_font_unlock_face (cairo_lines[i].scaled_font);
    }
}
