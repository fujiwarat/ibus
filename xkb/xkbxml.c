/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2010 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "xkbxml.h"
#include "ibuscomponent.h"

#ifndef XKB_RULES_XML_FILE
#define XKB_RULES_XML_FILE "/usr/share/X11/xkb/rules/evdev.xml"
#endif

#define IBUS_XKB_CONFIG_REGISTRY_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_XKB_CONFIG_REGISTRY, IBusXKBConfigRegistryPrivate))
#define IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_XKB_LAYOUT_CONFIG, IBusXKBLayoutConfigPrivate))

typedef struct _IBusXKBConfigRegistryPrivate IBusXKBConfigRegistryPrivate;
typedef struct _IBusXKBLayoutConfigPrivate IBusXKBLayoutConfigPrivate;

enum {
    PROP_0,
    PROP_SYSTEM_CONFIG_FILE,
};

struct _IBusXKBConfigRegistryPrivate {
    GHashTable *layout_list;
    GHashTable *layout_lang;
    GHashTable *layout_desc;
    GHashTable *variant_desc;
};

struct _IBusXKBLayoutConfigPrivate {
    gchar *system_config_file;
    GList *preload_layouts;
};

/* functions prototype */
static void         ibus_xkb_config_registry_destroy
                                           (IBusXKBConfigRegistry *xkb_config);
static void         ibus_xkb_layout_config_destroy
                                           (IBusXKBLayoutConfig *xkb_layout_config);

G_DEFINE_TYPE (IBusXKBConfigRegistry, ibus_xkb_config_registry, IBUS_TYPE_OBJECT)
G_DEFINE_TYPE (IBusXKBLayoutConfig, ibus_xkb_layout_config, IBUS_TYPE_OBJECT)

static void
parse_xkb_xml_languagelist_node (IBusXKBConfigRegistryPrivate *priv,
                                 XMLNode *parent_node,
                                 const gchar *layout_name)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    GList *lang_list = NULL;

    g_assert (node != NULL);
    g_assert (layout_name != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "iso639Id") == 0) {
            lang_list = g_list_append (lang_list,
                                       (gpointer) g_strdup (sub_node->text));
            continue;
        }
    }
    if (lang_list == NULL) {
        /* some nodes have no lang */
        return;
    }
    if (g_hash_table_lookup (priv->layout_lang, layout_name) != NULL) {
        g_warning ("duplicated name %s exists", layout_name);
        return;
    }
    g_hash_table_insert (priv->layout_lang,
                         (gpointer) g_strdup (layout_name),
                         (gpointer) lang_list);
}

static const gchar *
parse_xkb_xml_configitem_node (IBusXKBConfigRegistryPrivate *priv,
                               XMLNode *parent_node)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    gchar *name = NULL;
    gchar *description = NULL;

    g_assert (node != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "name") == 0) {
            name = sub_node->text;
            continue;
        }
        if (g_strcmp0 (sub_node->name, "description") == 0) {
            description = sub_node->text;
            continue;
        }
        if (g_strcmp0 (sub_node->name, "languageList") == 0) {
            if (name == NULL) {
                g_warning ("layout name is NULL in node %s", node->name);
                continue;
            }
            parse_xkb_xml_languagelist_node (priv, sub_node, name);
            continue;
        }
    }
    if (name == NULL) {
        g_warning ("No name in layout node");
        return NULL;
    }
    if (g_hash_table_lookup (priv->layout_desc, name) != NULL) {
        g_warning ("duplicated name %s exists", name);
        return name;
    }
    g_hash_table_insert (priv->layout_desc,
                         (gpointer) g_strdup (name),
                         (gpointer) g_strdup (description));

    return name;
}

static const gchar *
parse_xkb_xml_variant_configitem_node (IBusXKBConfigRegistryPrivate *priv,
                            XMLNode *parent_node,
                            const gchar *layout_name)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    gchar *name = NULL;
    gchar *description = NULL;
    gchar *variant_lang_name = NULL;

    g_assert (node != NULL);
    g_assert (layout_name != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "name") == 0) {
            name = sub_node->text;
            continue;
        }
        if (g_strcmp0 (sub_node->name, "description") == 0) {
            description = sub_node->text;
            continue;
        }
        if (g_strcmp0 (sub_node->name, "languageList") == 0) {
            if (name == NULL) {
                g_warning ("layout name is NULL in node %s", node->name);
                continue;
            }
            variant_lang_name = g_strdup_printf ("%s(%s)", layout_name, name);
            parse_xkb_xml_languagelist_node (priv, sub_node, variant_lang_name);
            g_free (variant_lang_name);
            continue;
        }
    }
    if (name == NULL) {
        g_warning ("No name in layout node");
        return NULL;
    }
    if (g_hash_table_lookup (priv->variant_desc, name) != NULL) {
        /* This is an expected case. */
        return name;
    }
    g_hash_table_insert (priv->variant_desc,
                         (gpointer) g_strdup (name),
                         (gpointer) g_strdup (description));
    return name;
}

static const gchar *
parse_xkb_xml_variant_node (IBusXKBConfigRegistryPrivate *priv,
                            XMLNode *parent_node,
                            const gchar *layout_name)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    const gchar *variant_name = NULL;

    g_assert (node != NULL);
    g_assert (layout_name != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "configItem") == 0) {
            variant_name = parse_xkb_xml_variant_configitem_node (priv, sub_node, layout_name);
            continue;
        }
    }
    return variant_name;
}

static GList *
parse_xkb_xml_variantlist_node (IBusXKBConfigRegistryPrivate *priv,
                                XMLNode *parent_node,
                                const gchar *layout_name,
                                GList *variant_list)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    const gchar *variant_name = NULL;

    g_assert (node != NULL);
    g_assert (layout_name != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "variant") == 0) {
            variant_name = parse_xkb_xml_variant_node (priv, sub_node, layout_name);
            if (variant_name != NULL) {
                variant_list = g_list_append (variant_list,
                                              (gpointer) g_strdup (variant_name));
            }
            continue;
        }
    }
    return variant_list;
}

static void
parse_xkb_xml_layout_node (IBusXKBConfigRegistryPrivate *priv,
                           XMLNode *parent_node)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;
    const gchar *name = NULL;
    GList *variant_list = NULL;

    g_assert (node != NULL);
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "configItem") == 0) {
            name = parse_xkb_xml_configitem_node (priv, sub_node);
            continue;
        }
        if (g_strcmp0 (sub_node->name, "variantList") == 0) {
            if (name == NULL) {
                g_warning ("layout name is NULL in node %s", node->name);
                continue;
            }
            variant_list = parse_xkb_xml_variantlist_node (priv, sub_node,
                                                           name,
                                                           variant_list);
            continue;
        }
    }
    if (g_hash_table_lookup (priv->layout_list, name) != NULL) {
        g_warning ("duplicated name %s exists", name);
        return;
    }
    g_hash_table_insert (priv->layout_list,
                         (gpointer) g_strdup (name),
                         (gpointer) variant_list);
}

static void
parse_xkb_xml_top_node (IBusXKBConfigRegistryPrivate *priv,
                        XMLNode *parent_node)
{
    XMLNode *node = parent_node;
    XMLNode *sub_node;
    GList *p;

    g_assert (priv != NULL);
    g_assert (node != NULL);

    if (g_strcmp0 (node->name, "xkbConfigRegistry") != 0) {
        g_warning ("node has no xkbConfigRegistry name");
        return;
    }
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "layoutList") == 0) {
            break;
        }
    }
    if (p == NULL) {
        g_warning ("xkbConfigRegistry node has no layoutList node");
        return;
    }
    node = sub_node;
    for (p = node->sub_nodes; p; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "layout") == 0) {
            parse_xkb_xml_layout_node (priv, sub_node);
            continue;
        }
    }
}

static void
free_lang_list (GList *list)
{
    GList *l = list;
    while (l) {
        g_free (l->data);
        l->data = NULL;
        l = l->next;
    }
    g_list_free (list);
}

static void
parse_xkb_config_registry_file (IBusXKBConfigRegistryPrivate *priv,
                                const gchar *file)
{
    XMLNode *node;

    g_assert (file != NULL);

    priv->layout_list = g_hash_table_new_full (g_str_hash,
                                               (GEqualFunc) g_str_equal,
                                               (GDestroyNotify) g_free,
                                               (GDestroyNotify) free_lang_list);
    priv->layout_desc = g_hash_table_new_full (g_str_hash,
                                               (GEqualFunc) g_str_equal,
                                               (GDestroyNotify) g_free,
                                               (GDestroyNotify) g_free);
    priv->layout_lang = g_hash_table_new_full (g_str_hash,
                                               (GEqualFunc) g_str_equal,
                                               (GDestroyNotify) g_free,
                                               (GDestroyNotify) free_lang_list);
    priv->variant_desc = g_hash_table_new_full (g_str_hash,
                                               (GEqualFunc) g_str_equal,
                                               (GDestroyNotify) g_free,
                                               (GDestroyNotify) g_free);
    node = ibus_xml_parse_file (file);
    parse_xkb_xml_top_node (priv, node);
    ibus_xml_free (node);
}

static GList *
parse_xkblayoutconfig_file (gchar *path)
{
    XMLNode *node = NULL;
    XMLNode *sub_node;
    XMLNode *sub_sub_node;
    GList *p;
    GList *retval = NULL;
    gchar **array;
    int i;

    node = ibus_xml_parse_file (path);
    if (node == NULL) {
        return NULL;
    }
    if (g_strcmp0 (node->name, "xkblayout") != 0) {
        ibus_xml_free (node);
        return NULL;
    }
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "config") == 0) {
            GList *pp;
            for (pp = sub_node->sub_nodes; pp != NULL; pp = pp->next) {
                sub_sub_node = (XMLNode *) pp->data;
                if  (g_strcmp0 (sub_sub_node->name, "preload_layouts") == 0) {
                    if (sub_sub_node->text != NULL) {
                        array = g_strsplit ((gchar *) sub_sub_node->text,
                                            ",", -1);
                        for (i = 0; array[i]; i++) {
                            retval = g_list_append (retval, g_strdup (array[i]));
                        }
                        g_strfreev (array);
                        break;
                    }
                }
            }
        }
        if (retval != NULL) {
            break;
        }
    }

    ibus_xml_free (node);
    return retval;
}

static void
parse_xkb_layout_config (IBusXKBLayoutConfigPrivate *priv)
{
    gchar *basename;
    gchar *user_config;
    GList *list = NULL;

    g_return_if_fail (priv->system_config_file != NULL);

    basename = g_path_get_basename (priv->system_config_file);
    user_config = g_build_filename (g_get_user_config_dir (),
                                    "ibus", "xkb",
                                    basename, NULL);
    g_free (basename);
    list = parse_xkblayoutconfig_file (user_config);
    g_free (user_config);
    if (list) {
        priv->preload_layouts = list;
        return;
    }
    list = parse_xkblayoutconfig_file (priv->system_config_file);
    priv->preload_layouts = list;
}

static void
ibus_xkb_config_registry_init (IBusXKBConfigRegistry *xkb_config)
{
    IBusXKBConfigRegistryPrivate *priv;
    const gchar *file = XKB_RULES_XML_FILE;

    priv = IBUS_XKB_CONFIG_REGISTRY_GET_PRIVATE (xkb_config);
    parse_xkb_config_registry_file (priv, file);
}

static void
ibus_xkb_config_registry_destroy (IBusXKBConfigRegistry *xkb_config)
{
    IBusXKBConfigRegistryPrivate *priv;

    g_return_if_fail (xkb_config != NULL);

    priv = IBUS_XKB_CONFIG_REGISTRY_GET_PRIVATE (xkb_config);

    g_hash_table_destroy (priv->layout_list);
    priv->layout_list = NULL;
    g_hash_table_destroy (priv->layout_lang);
    priv->layout_lang= NULL;
    g_hash_table_destroy (priv->layout_desc);
    priv->layout_desc= NULL;
    g_hash_table_destroy (priv->variant_desc);
    priv->variant_desc = NULL;

    IBUS_OBJECT_CLASS(ibus_xkb_config_registry_parent_class)->destroy (IBUS_OBJECT (xkb_config));
}

static void
ibus_xkb_config_registry_class_init (IBusXKBConfigRegistryClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IBusXKBConfigRegistryPrivate));

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_xkb_config_registry_destroy;
}

static void
ibus_xkb_layout_config_init (IBusXKBLayoutConfig *xkb_layout_config)
{
    IBusXKBLayoutConfigPrivate *priv;

    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);
    priv->system_config_file = NULL;
    priv->preload_layouts = NULL;
}

static GObject *
ibus_xkb_layout_config_constructor (GType type,
                                    guint n_construct_params,
                                    GObjectConstructParam *construct_params)
{
    GObject *obj;
    IBusXKBLayoutConfig *xkb_layout_config;
    IBusXKBLayoutConfigPrivate *priv;

    obj = G_OBJECT_CLASS (ibus_xkb_layout_config_parent_class)->constructor (type, n_construct_params, construct_params);
    xkb_layout_config = IBUS_XKB_LAYOUT_CONFIG (obj);
    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);
    parse_xkb_layout_config (priv);

    return obj;
}

static void
ibus_xkb_layout_config_destroy (IBusXKBLayoutConfig *xkb_layout_config)
{
    IBusXKBLayoutConfigPrivate *priv;

    g_return_if_fail (xkb_layout_config != NULL);

    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);

    g_free (priv->system_config_file);
    priv->system_config_file = NULL;
    free_lang_list (priv->preload_layouts);
    priv->preload_layouts = NULL;
}

static void
ibus_xkb_layout_config_set_property (IBusXKBLayoutConfig *xkb_layout_config,
                                     guint                prop_id,
                                     const GValue        *value,
                                     GParamSpec          *pspec)
{
    IBusXKBLayoutConfigPrivate *priv;

    g_return_if_fail (xkb_layout_config != NULL);
    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);

    switch (prop_id) {
    case PROP_SYSTEM_CONFIG_FILE:
        g_assert (priv->system_config_file == NULL);
        priv->system_config_file = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (xkb_layout_config, prop_id, pspec);
    }
}

static void
ibus_xkb_layout_config_get_property (IBusXKBLayoutConfig *xkb_layout_config,
                                     guint                prop_id,
                                     GValue              *value,
                                     GParamSpec          *pspec)
{
    IBusXKBLayoutConfigPrivate *priv;

    g_return_if_fail (xkb_layout_config != NULL);
    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);

    switch (prop_id) {
    case PROP_SYSTEM_CONFIG_FILE:
        g_value_set_string (value, priv->system_config_file);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (xkb_layout_config, prop_id, pspec);

    }
}

static void
ibus_xkb_layout_config_class_init (IBusXKBLayoutConfigClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IBusXKBLayoutConfigPrivate));

    gobject_class->constructor = ibus_xkb_layout_config_constructor;
    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_xkb_layout_config_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_xkb_layout_config_get_property;
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_xkb_layout_config_destroy;

    /**
     * IBusProxy:interface:
     *
     * The interface of the proxy object.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SYSTEM_CONFIG_FILE,
                    g_param_spec_string ("system_config_file",
                        "system_config_file",
                        "The system file of xkblayoutconfig",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

IBusXKBConfigRegistry *
ibus_xkb_config_registry_new (void)
{
    IBusXKBConfigRegistry *xkb_config;

    xkb_config = IBUS_XKB_CONFIG_REGISTRY (g_object_new (IBUS_TYPE_XKB_CONFIG_REGISTRY, NULL));
    return xkb_config;
}

#define TABLE_FUNC(field_name) const GHashTable *                       \
ibus_xkb_config_registry_get_##field_name  (IBusXKBConfigRegistry *xkb_config) \
{                                                                       \
    IBusXKBConfigRegistryPrivate *priv;                                 \
                                                                        \
    g_return_val_if_fail (xkb_config != NULL, NULL);                    \
    priv = IBUS_XKB_CONFIG_REGISTRY_GET_PRIVATE (xkb_config);           \
    return priv->field_name;                                            \
}

TABLE_FUNC (layout_list)
TABLE_FUNC (layout_lang)
TABLE_FUNC (layout_desc)
TABLE_FUNC (variant_desc)

#undef TABLE_FUNC

IBusComponent *
ibus_xkb_component_new (void)
{
    IBusComponent *component;

    component = ibus_component_new ("org.freedesktop.IBus.XKB",
                                    "XKB Component",
                                    VERSION,
                                    "LGPL2.1",
                                    "Takao Fujiwara <takao.fujiwara1@gmail.com>",
                                    "http://code.google.com/p/ibus/",
                                    LIBEXECDIR "/ibus-engine-xkb --ibus",
                                    GETTEXT_PACKAGE);

    return component;
}

IBusEngineDesc *
ibus_xkb_engine_desc_new (const gchar *lang,
                          const gchar *layout,
                          const gchar *layout_desc,
                          const gchar *variant,
                          const gchar *variant_desc)
{
    IBusEngineDesc *engine;
    gchar *name = NULL;
    gchar *longname = NULL;
    gchar *desc = NULL;
    gchar *engine_layout = NULL;

    g_return_val_if_fail (lang != NULL && layout != NULL, NULL);

    if (layout_desc && variant_desc) {
        longname = g_strdup_printf ("%s - %s", layout_desc, variant_desc);
    } else if (layout && variant) {
        longname = g_strdup_printf ("%s - %s", layout, variant);
    } else if (layout_desc) {
        longname = g_strdup (layout_desc);
    } else {
        longname = g_strdup (layout);
    }
    if (variant) {
        name = g_strdup_printf ("xkb:layout:%s:%s", layout, variant);
        desc = g_strdup_printf ("XKB %s(%s) keyboard layout", layout, variant);
        engine_layout = g_strdup_printf ("%s(%s)", layout, variant);
    } else {
        name = g_strdup_printf ("xkb:layout:%s", layout);
        desc = g_strdup_printf ("XKB %s keyboard layout", layout);
        engine_layout = g_strdup (layout);
    }

    engine = ibus_engine_desc_new (name,
                                   longname,
                                   desc,
                                   lang,
                                   "LGPL2.1",
                                   "Takao Fujiwara <takao.fujiwara1@gmail.com>",
                                   "ibus-engine",
                                   engine_layout);

    g_free (name);
    g_free (longname);
    g_free (desc);
    g_free (engine_layout);

    return engine;
}

IBusXKBLayoutConfig *
ibus_xkb_layout_config_new (const gchar *system_config_file)
{
    IBusXKBLayoutConfig *xkb_layout_config;

    xkb_layout_config = IBUS_XKB_LAYOUT_CONFIG (g_object_new (IBUS_TYPE_XKB_LAYOUT_CONFIG,
                                                              "system_config_file",
                                                              system_config_file,
                                                              NULL));
    return xkb_layout_config;
}

const GList *
ibus_xkb_layout_config_get_preload_layouts (IBusXKBLayoutConfig *xkb_layout_config)
{
    IBusXKBLayoutConfigPrivate *priv;

    g_return_val_if_fail (xkb_layout_config != NULL, NULL);
    priv = IBUS_XKB_LAYOUT_CONFIG_GET_PRIVATE (xkb_layout_config);
    return (const GList *) priv->preload_layouts;
}
