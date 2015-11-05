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

#ifndef __FBTERM_KEY_MAP_H_
#define __FBTERM_KEY_MAP_H_

#include <glib.h>
#include <ibus.h>

static guint32 linux_to_x[256] = {
    0,   0,   0,   0,
    0,   0,   0,   0,
    IBUS_KEY_BackSpace,   IBUS_KEY_Tab,     IBUS_KEY_Linefeed,    0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   IBUS_KEY_Escape,
    0,   0,   0,   0,
    IBUS_KEY_space,   IBUS_KEY_exclam,  IBUS_KEY_quotedbl,    IBUS_KEY_numbersign,
    IBUS_KEY_dollar,  IBUS_KEY_percent, IBUS_KEY_ampersand,   IBUS_KEY_apostrophe,
    IBUS_KEY_parenleft,   IBUS_KEY_parenright,  IBUS_KEY_asterisk,    IBUS_KEY_plus,
    IBUS_KEY_comma,   IBUS_KEY_minus,   IBUS_KEY_period,  IBUS_KEY_slash,
    IBUS_KEY_0,       IBUS_KEY_1,       IBUS_KEY_2,       IBUS_KEY_3,
    IBUS_KEY_4,       IBUS_KEY_5,       IBUS_KEY_6,       IBUS_KEY_7,
    IBUS_KEY_8,       IBUS_KEY_9,       IBUS_KEY_colon,   IBUS_KEY_semicolon,
    IBUS_KEY_less,    IBUS_KEY_equal,   IBUS_KEY_greater, IBUS_KEY_question,
    IBUS_KEY_at,      IBUS_KEY_A,       IBUS_KEY_B,       IBUS_KEY_C,
    IBUS_KEY_D,       IBUS_KEY_E,       IBUS_KEY_F,       IBUS_KEY_G,
    IBUS_KEY_H,       IBUS_KEY_I,       IBUS_KEY_J,       IBUS_KEY_K,
    IBUS_KEY_L,       IBUS_KEY_M,       IBUS_KEY_N,       IBUS_KEY_O,
    IBUS_KEY_P,       IBUS_KEY_Q,       IBUS_KEY_R,       IBUS_KEY_S,
    IBUS_KEY_T,       IBUS_KEY_U,       IBUS_KEY_V,       IBUS_KEY_W,
    IBUS_KEY_X,       IBUS_KEY_Y,       IBUS_KEY_Z,       IBUS_KEY_bracketleft,
    IBUS_KEY_backslash,   IBUS_KEY_bracketright,IBUS_KEY_asciicircum, IBUS_KEY_underscore,
    IBUS_KEY_grave,   IBUS_KEY_a,       IBUS_KEY_b,       IBUS_KEY_c,
    IBUS_KEY_d,       IBUS_KEY_e,       IBUS_KEY_f,       IBUS_KEY_g,
    IBUS_KEY_h,       IBUS_KEY_i,       IBUS_KEY_j,       IBUS_KEY_k,
    IBUS_KEY_l,       IBUS_KEY_m,       IBUS_KEY_n,       IBUS_KEY_o,
    IBUS_KEY_p,       IBUS_KEY_q,       IBUS_KEY_r,       IBUS_KEY_s,
    IBUS_KEY_t,       IBUS_KEY_u,       IBUS_KEY_v,       IBUS_KEY_w,
    IBUS_KEY_x,       IBUS_KEY_y,       IBUS_KEY_z,       IBUS_KEY_braceleft,
    IBUS_KEY_bar,     IBUS_KEY_braceright,  IBUS_KEY_asciitilde,  IBUS_KEY_BackSpace,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    IBUS_KEY_nobreakspace,IBUS_KEY_exclamdown,  IBUS_KEY_cent,    IBUS_KEY_sterling,
    IBUS_KEY_currency,    IBUS_KEY_yen,     IBUS_KEY_brokenbar,   IBUS_KEY_section,
    IBUS_KEY_diaeresis,   IBUS_KEY_copyright,   IBUS_KEY_ordfeminine, IBUS_KEY_guillemotleft,
    IBUS_KEY_notsign, IBUS_KEY_hyphen,  IBUS_KEY_registered,  IBUS_KEY_macron,
    IBUS_KEY_degree,  IBUS_KEY_plusminus,   IBUS_KEY_twosuperior, IBUS_KEY_threesuperior,
    IBUS_KEY_acute,   IBUS_KEY_mu,      IBUS_KEY_paragraph,   IBUS_KEY_periodcentered,
    IBUS_KEY_cedilla, IBUS_KEY_onesuperior, IBUS_KEY_masculine,   IBUS_KEY_guillemotright,
    IBUS_KEY_onequarter,  IBUS_KEY_onehalf, IBUS_KEY_threequarters,IBUS_KEY_questiondown,
    IBUS_KEY_Agrave,  IBUS_KEY_Aacute,  IBUS_KEY_Acircumflex, IBUS_KEY_Atilde,
    IBUS_KEY_Adiaeresis,  IBUS_KEY_Aring,   IBUS_KEY_AE,      IBUS_KEY_Ccedilla,
    IBUS_KEY_Egrave,  IBUS_KEY_Eacute,  IBUS_KEY_Ecircumflex, IBUS_KEY_Ediaeresis,
    IBUS_KEY_Igrave,  IBUS_KEY_Iacute,  IBUS_KEY_Icircumflex, IBUS_KEY_Idiaeresis,
    IBUS_KEY_ETH,     IBUS_KEY_Ntilde,  IBUS_KEY_Ograve,  IBUS_KEY_Oacute,
    IBUS_KEY_Ocircumflex, IBUS_KEY_Otilde,  IBUS_KEY_Odiaeresis,  IBUS_KEY_multiply,
    IBUS_KEY_Ooblique,    IBUS_KEY_Ugrave,  IBUS_KEY_Uacute,  IBUS_KEY_Ucircumflex,
    IBUS_KEY_Udiaeresis,  IBUS_KEY_Yacute,  IBUS_KEY_THORN,   IBUS_KEY_ssharp,
    IBUS_KEY_agrave,  IBUS_KEY_aacute,  IBUS_KEY_acircumflex, IBUS_KEY_atilde,
    IBUS_KEY_adiaeresis,  IBUS_KEY_aring,   IBUS_KEY_ae,      IBUS_KEY_ccedilla,
    IBUS_KEY_egrave,  IBUS_KEY_eacute,  IBUS_KEY_ecircumflex, IBUS_KEY_ediaeresis,
    IBUS_KEY_igrave,  IBUS_KEY_iacute,  IBUS_KEY_icircumflex, IBUS_KEY_idiaeresis,
    IBUS_KEY_eth,     IBUS_KEY_ntilde,  IBUS_KEY_ograve,  IBUS_KEY_oacute,
    IBUS_KEY_ocircumflex, IBUS_KEY_otilde,  IBUS_KEY_odiaeresis,  IBUS_KEY_division,
    IBUS_KEY_oslash,  IBUS_KEY_ugrave,  IBUS_KEY_uacute,  IBUS_KEY_ucircumflex,
    IBUS_KEY_udiaeresis,  IBUS_KEY_yacute,  IBUS_KEY_thorn,   IBUS_KEY_ydiaeresis
};

#endif
