/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
#include <stdlib.h>
#include "ibusenginedesc.h"
#include "ibusxml.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_NAME,
    PROP_LONGNAME,
    PROP_DESCRIPTION,
    PROP_LANGUAGE,
    PROP_LICENSE,
    PROP_AUTHOR,
    PROP_ICON,
    PROP_LAYOUT,
    PROP_HOTKEYS,
    PROP_RANK,
};

/* IBusEngineDescPriv */
struct _IBusEngineDescPrivate {
    gchar      *hotkeys;
};
typedef struct _IBusEngineDescPrivate IBusEngineDescPrivate;

#define IBUS_ENGINE_DESC_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_ENGINE_DESC, IBusEngineDescPrivate))

// static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void         ibus_engine_desc_set_property   (IBusEngineDesc         *desc,
                                                     guint                   prop_id,
                                                     const GValue           *value,
                                                     GParamSpec             *pspec);
static void         ibus_engine_desc_get_property   (IBusEngineDesc         *desc,
                                                     guint                   prop_id,
                                                     GValue                 *value,
                                                     GParamSpec             *pspec);
static void         ibus_engine_desc_destroy        (IBusEngineDesc         *desc);
static gboolean     ibus_engine_desc_serialize      (IBusEngineDesc         *desc,
                                                     IBusMessageIter        *iter);
static gboolean     ibus_engine_desc_deserialize    (IBusEngineDesc         *desc,
                                                     IBusMessageIter        *iter);
static gboolean     ibus_engine_desc_copy           (IBusEngineDesc         *dest,
                                                     const IBusEngineDesc   *src);
static gboolean     ibus_engine_desc_parse_xml_node (IBusEngineDesc         *desc,
                                                     XMLNode                *node);

G_DEFINE_TYPE (IBusEngineDesc, ibus_engine_desc, IBUS_TYPE_SERIALIZABLE)


static void
ibus_engine_desc_class_init (IBusEngineDescClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (klass);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (klass);

    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_engine_desc_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_engine_desc_get_property;
    object_class->destroy = (IBusObjectDestroyFunc) ibus_engine_desc_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_engine_desc_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_engine_desc_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_engine_desc_copy;

    g_string_append (serializable_class->signature, "ssssssssus");

    g_type_class_add_private (klass, sizeof (IBusEngineDescPrivate));

    /**
     * IBusEngineDesc:hotkeys:
     *
     * The hotkeys of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_HOTKEYS,
                    g_param_spec_string ("hotkeys",
                        "description hotkeys",
                        "The hotkeys of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_engine_desc_init (IBusEngineDesc *desc)
{
    IBusEngineDescPrivate *priv;

    desc->name = NULL;
    desc->longname = NULL;
    desc->description = NULL;
    desc->language = NULL;
    desc->license = NULL;
    desc->author = NULL;
    desc->icon = NULL;
    desc->layout = NULL;
    desc->rank = 0;

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);
    priv->hotkeys = NULL;
}

static void
ibus_engine_desc_destroy (IBusEngineDesc *desc)
{
    IBusEngineDescPrivate *priv;

    g_free (desc->name);
    g_free (desc->longname);
    g_free (desc->description);
    g_free (desc->language);
    g_free (desc->license);
    g_free (desc->author);
    g_free (desc->icon);
    g_free (desc->layout);

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);
    g_free (priv->hotkeys);

    IBUS_OBJECT_CLASS (ibus_engine_desc_parent_class)->destroy (IBUS_OBJECT (desc));
}

static void
ibus_engine_desc_set_property (IBusEngineDesc *desc,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
    IBusEngineDescPrivate *priv;
    priv = IBUS_ENGINE_DESC_GET_PRIVATE (desc);

    switch (prop_id) {
    case PROP_HOTKEYS:
        g_assert (priv->hotkeys == NULL);
        priv->hotkeys = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (desc, prop_id, pspec);
    }
}

const gchar *
ibus_engine_desc_get_hotkeys (IBusEngineDesc *desc)
{
    IBusEngineDescPrivate *priv;
    priv = IBUS_ENGINE_DESC_GET_PRIVATE (desc);

    return priv->hotkeys;
}

static void
ibus_engine_desc_get_property (IBusEngineDesc *desc,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    switch (prop_id) {
    case PROP_HOTKEYS:
        g_value_set_string (value, ibus_engine_desc_get_hotkeys (desc));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (desc, prop_id, pspec);
    }
}

static gboolean
ibus_engine_desc_serialize (IBusEngineDesc  *desc,
                            IBusMessageIter *iter)
{
    gboolean retval;
    IBusEngineDescPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->serialize ((IBusSerializable *)desc, iter);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->name);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->longname);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->description);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->language);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->license);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->author);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->icon);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &desc->layout);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_append (iter, G_TYPE_UINT, &desc->rank);
    g_return_val_if_fail (retval, FALSE);

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);
    retval = ibus_message_iter_append (iter, G_TYPE_STRING, &priv->hotkeys);
    g_return_val_if_fail (retval, FALSE);

    return TRUE;
}

static gboolean
ibus_engine_desc_deserialize (IBusEngineDesc  *desc,
                              IBusMessageIter *iter)
{
    gboolean retval;
    gchar *str;
    IBusEngineDescPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->deserialize ((IBusSerializable *)desc, iter);
    g_return_val_if_fail (retval, FALSE);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->name = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->longname = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->description = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->language = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->license = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->author = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->icon = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    desc->layout = g_strdup (str);

    retval = ibus_message_iter_get (iter, G_TYPE_UINT, &desc->rank);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);
    retval = ibus_message_iter_get (iter, G_TYPE_STRING, &str);
    g_return_val_if_fail (retval, FALSE);
    ibus_message_iter_next (iter);
    priv->hotkeys = g_strdup (str);

    return TRUE;
}

static gboolean
ibus_engine_desc_copy (IBusEngineDesc       *dest,
                       const IBusEngineDesc *src)
{
    gboolean retval;
    IBusEngineDescPrivate *priv_dest;
    IBusEngineDescPrivate *priv_src;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);


    dest->name          = g_strdup (src->name);
    dest->longname      = g_strdup (src->longname);
    dest->description   = g_strdup (src->description);
    dest->language      = g_strdup (src->language);
    dest->license       = g_strdup (src->license);
    dest->author        = g_strdup (src->author);
    dest->icon          = g_strdup (src->icon);
    dest->layout        = g_strdup (src->layout);

    priv_dest = IBUS_ENGINE_DESC_GET_PRIVATE(dest);
    priv_src = IBUS_ENGINE_DESC_GET_PRIVATE(src);

    priv_dest->hotkeys  = g_strdup (priv_src->hotkeys);

    return TRUE;
}

#define g_string_append_indent(string, indent)  \
    {                                           \
        gint i;                                 \
        for (i = 0; i < (indent); i++) {        \
            g_string_append (string, "    ");   \
        }                                       \
    }

void
ibus_engine_desc_output (IBusEngineDesc *desc,
                         GString        *output,
                         gint            indent)
{
    IBusEngineDescPrivate *priv;

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);

    g_string_append_indent (output, indent);
    g_string_append (output, "<engine>\n");
#define OUTPUT_ENTRY(field, element, desc)                                              \
    {                                                                                   \
        gchar *escape_text = g_markup_escape_text (desc->field ? desc->field : "", -1); \
        g_string_append_indent (output, indent + 1);                                    \
        g_string_append_printf (output, "<"element">%s</"element">\n",                  \
                                escape_text);                                           \
        g_free (escape_text);                                                           \
    }
#define OUTPUT_ENTRY_1(name) OUTPUT_ENTRY(name, #name, desc)
#define OUTPUT_ENTRY_2(name) OUTPUT_ENTRY(name, #name, priv)
    OUTPUT_ENTRY_1(name);
    OUTPUT_ENTRY_1(longname);
    OUTPUT_ENTRY_1(description);
    OUTPUT_ENTRY_1(language);
    OUTPUT_ENTRY_1(license);
    OUTPUT_ENTRY_1(author);
    OUTPUT_ENTRY_1(icon);
    OUTPUT_ENTRY_1(layout);
    OUTPUT_ENTRY_2(hotkeys);
    g_string_append_indent (output, indent + 1);
    g_string_append_printf (output, "<rank>%u</rank>\n", desc->rank);
#undef OUTPUT_ENTRY
#undef OUTPUT_ENTRY_1
#undef OUTPUT_ENTRY_2
    g_string_append_indent (output, indent);
    g_string_append (output, "</engine>\n");
}

static gboolean
ibus_engine_desc_parse_xml_node (IBusEngineDesc *desc,
                                XMLNode       *node)
{
    GList *p;
    IBusEngineDescPrivate *priv;

    priv = IBUS_ENGINE_DESC_GET_PRIVATE(desc);
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *) p->data;

#define PARSE_ENTRY(field_name, element_name, desc)             \
        if (g_strcmp0 (sub_node->name, element_name) == 0) {    \
            g_free (desc->field_name);                          \
            desc->field_name = g_strdup (sub_node->text);       \
            continue;                                           \
        }
#define PARSE_ENTRY_1(name) PARSE_ENTRY(name, #name, desc)
#define PARSE_ENTRY_2(name) PARSE_ENTRY(name, #name, priv)
        PARSE_ENTRY_1(name);
        PARSE_ENTRY_1(longname);
        PARSE_ENTRY_1(description);
        PARSE_ENTRY_1(language);
        PARSE_ENTRY_1(license);
        PARSE_ENTRY_1(author);
        PARSE_ENTRY_1(icon);
        PARSE_ENTRY_1(layout);
        PARSE_ENTRY_2(hotkeys);
#undef PARSE_ENTRY
#undef PARSE_ENTRY_1
#undef PARSE_ENTRY_2
        if (g_strcmp0 (sub_node->name , "rank") == 0) {
            desc->rank = atoi (sub_node->text);
            continue;
        }
        g_warning ("<engines> element contains invalidate element <%s>", sub_node->name);
    }
    return TRUE;
}

IBusEngineDesc *
ibus_engine_desc_new (const gchar *name,
                      const gchar *longname,
                      const gchar *description,
                      const gchar *language,
                      const gchar *license,
                      const gchar *author,
                      const gchar *icon,
                      const gchar *layout)
{
    return ibus_engine_desc_new2 (name, longname, description, language,
                                  license, author, icon, layout, "");
}

IBusEngineDesc *
ibus_engine_desc_new2 (const gchar *name,
                       const gchar *longname,
                       const gchar *description,
                       const gchar *language,
                       const gchar *license,
                       const gchar *author,
                       const gchar *icon,
                       const gchar *layout,
                       const gchar *hotkeys)
{
    g_assert (name);
    g_assert (longname);
    g_assert (description);
    g_assert (language);
    g_assert (license);
    g_assert (author);
    g_assert (icon);
    g_assert (layout);
    g_assert (hotkeys);

    IBusEngineDesc *desc;
    desc = (IBusEngineDesc *)g_object_new (IBUS_TYPE_ENGINE_DESC,
                                           "hotkeys", hotkeys,
                                           NULL);

    desc->name          = g_strdup (name);
    desc->longname      = g_strdup (longname);
    desc->description   = g_strdup (description);
    desc->language      = g_strdup (language);
    desc->license       = g_strdup (license);
    desc->author        = g_strdup (author);
    desc->icon          = g_strdup (icon);
    desc->layout        = g_strdup (layout);

    return desc;
}

IBusEngineDesc *
ibus_engine_desc_new_from_xml_node (XMLNode      *node)
{
    g_assert (node);

    IBusEngineDesc *desc;

    if (G_UNLIKELY (g_strcmp0 (node->name, "engine") != 0)) {
        return NULL;
    }

    desc = (IBusEngineDesc *)g_object_new (IBUS_TYPE_ENGINE_DESC, NULL);

    if (!ibus_engine_desc_parse_xml_node (desc, node)) {
        g_object_unref (desc);
        desc = NULL;
    }

    return desc;
}
