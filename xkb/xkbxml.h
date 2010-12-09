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
#ifndef __IBUS_XKB_H_
#define __IBUS_XKB_H_

#include "ibuscomponent.h"

/*
 * Type macros.
 */
/* define IBusXKBConfigRegistry macros */
#define IBUS_TYPE_XKB_CONFIG_REGISTRY                   \
    (ibus_xkb_config_registry_get_type ())
#define IBUS_XKB_CONFIG_REGISTRY(obj)                   \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_XKB_CONFIG_REGISTRY, IBusXKBConfigRegistry))
#define IBUS_XKB_CONFIG_REGISTRY_CLASS(klass)           \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_XKB_CONFIG_REGISTRY, IBusXKBConfigRegistryClass))
#define IBUS_IS_XKB_CONFIG_REGISTRY(obj)                \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_XKB_CONFIG_REGISTRY))
#define IBUS_IS_XKB_CONFIG_REGISTRY_CLASS(klass)        \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_XKB_CONFIG_REGISTRY))
#define IBUS_XKB_CONFIG_REGISTRY_GET_CLASS(obj)         \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_XKB_CONFIG_REGISTRY, IBusXKBConfigRegistryClass))

/* define IBusXKBLayoutConfig macros */
#define IBUS_TYPE_XKB_LAYOUT_CONFIG                     \
    (ibus_xkb_layout_config_get_type ())
#define IBUS_XKB_LAYOUT_CONFIG(obj)                     \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_XKB_LAYOUT_CONFIG, IBusXKBLayoutConfig))
#define IBUS_XKB_LAYOUT_CONFIG_CLASS(klass)             \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_XKB_LAYOUT_CONFIG, IBusXKBLayoutConfigClass))
#define IBUS_IS_XKB_LAYOUT_CONFIG(obj)                  \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_XKB_LAYOUT_CONFIG))
#define IBUS_IS_XKB_LAYOUT_CONFIG_CLASS(klass)          \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_XKB_LAYOUT_CONFIG))
#define IBUS_XKB_LAYOUT_CONFIG_GET_CLASS(obj)           \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_XKB_LAYOUT_CONFIG, IBusXKBLayoutConfigClass))

G_BEGIN_DECLS

typedef struct _IBusXKBConfigRegistry IBusXKBConfigRegistry;
typedef struct _IBusXKBConfigRegistryClass IBusXKBConfigRegistryClass;
typedef struct _IBusXKBLayoutConfig IBusXKBLayoutConfig;
typedef struct _IBusXKBLayoutConfigClass IBusXKBLayoutConfigClass;

struct _IBusXKBConfigRegistry {
    IBusObject parent;
};

struct _IBusXKBConfigRegistryClass {
    IBusObjectClass parent;
    /* signals */
    /*< private >*/
    /* padding */
    gpointer pdummy[8];
};

struct _IBusXKBLayoutConfig {
    IBusObject parent;
};

struct _IBusXKBLayoutConfigClass {
    IBusObjectClass parent;
    /* signals */
    /*< private >*/
    /* padding */
    gpointer pdummy[8];
};


GType            ibus_xkb_config_registry_get_type
                                                 (void);
/**
 * ibus_xkb_config_registry_new:
 * @returns: A newly allocated IBusXKBConfigRegistry
 *
 * New an IBusXKBConfigRegistry.
 */
IBusXKBConfigRegistry *
                 ibus_xkb_config_registry_new
                                                 (void);
/**
 * ibus_xkb_config_registry_get_layout_list:
 * @xkb_config: An IBusXKBConfigRegistry.
 * @returns: A const GHashTable
 *
 * a const GHashTable
 */
const GHashTable *
                 ibus_xkb_config_registry_get_layout_list
                                                 (IBusXKBConfigRegistry *xkb_config);
/**
 * ibus_xkb_config_registry_get_layout_lang:
 * @xkb_config: An IBusXKBConfigRegistry.
 * @returns: A const GHashTable
 *
 * a const GHashTable
 */
const GHashTable *
                 ibus_xkb_config_registry_get_layout_lang
                                                 (IBusXKBConfigRegistry *xkb_config);
/**
 * ibus_xkb_config_registry_get_layout_desc:
 * @xkb_config: An IBusXKBConfigRegistry.
 * @returns: A const GHashTable
 *
 * a const GHashTable
 */
const GHashTable *
                 ibus_xkb_config_registry_get_layout_desc
                                                 (IBusXKBConfigRegistry *xkb_config);
/**
 * ibus_xkb_config_registry_get_variant_desc:
 * @xkb_config: An IBusXKBConfigRegistry.
 * @returns: A const GHashTable
 *
 * a const GHashTable
 */
const GHashTable *
                 ibus_xkb_config_registry_get_variant_desc
                                                 (IBusXKBConfigRegistry *xkb_config);
/**
 * ibus_xkb_component_new:
 * @returns: A newly allocated IBusComponent.
 *
 * New an IBusComponent.
 */
IBusComponent   *ibus_xkb_component_new          (void);

/**
 * ibus_xkb_engine_desc_new:
 * @lang: Language (e.g. zh, jp) supported by this input method engine.
 * @layout: Keyboard layout
 * @layout_desc: Keyboard layout description for engine description
 * @variant: Keyboard variant
 * @variant_desc: Keyboard variant description for engine description
 * @returns: A newly allocated IBusEngineDesc.
 *
 * New a IBusEngineDesc.
 */
IBusEngineDesc  *ibus_xkb_engine_desc_new        (const gchar *lang,
                                                  const gchar *layout,
                                                  const gchar *layout_desc,
                                                  const gchar *variant,
                                                  const gchar *variant_desc);

GType            ibus_xkb_layout_config_get_type (void);

/**
 * ibus_xkb_layout_config_new:
 * @returns: A newly allocated IBusXKBLayoutConfig
 *
 * New an IBusXKBLayoutConfig
 */
IBusXKBLayoutConfig *
                 ibus_xkb_layout_config_new      (const gchar *system_config_file);

/**
 * ibus_xkb_layout_config_get_preload_layouts:
 * @xkb_layout_config: An IBusXKBLayoutConfig.
 * @returns: A const GList
 *
 * a const GList
 */
const GList *    ibus_xkb_layout_config_get_preload_layouts
                                                 (IBusXKBLayoutConfig *xkb_layout_config);

G_END_DECLS
#endif
