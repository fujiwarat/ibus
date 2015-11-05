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
#include <linux/keyboard.h>

#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "fbterm.h"
#include "keymap.h"
#include "keyobject.h"
#include "immessage.h"
#include "inputcontext.h"

#define MESSAGE_BUFFER_SIZE    10240
#define MESSAGE(obj)           ((Message *) (obj))
#define MESSAGE_OFFSET(member) ((size_t)(&(((Message *)0)->member)))

struct _IBusFbTerm {
    IBusBus            *bus;
    FbTermInputContext *input_context;
    int                 fd;
    GIOChannel         *iochannel;
    FbTermKeyObject    *key_object;
    gboolean            is_active;
    guint32             modifier_state;
};

static void
bus_disconnected_cb (IBusBus  *bus,
                      gpointer  data)
{
    IBusFbTerm *ibus_fbterm = (IBusFbTerm *)data;

    g_warning ("Connection closed by ibus-daemon");
    g_object_unref (ibus_fbterm->bus);
    ibus_fbterm->bus = NULL;
    exit(EXIT_SUCCESS);
}

static void
sighandler (int sig)
{
    exit(EXIT_FAILURE);
}

static int
get_im_socket ()
{
    static int fd = 0;
    const gchar *im_socket_str;

    if (fd)
      return fd;

    im_socket_str = g_getenv ("FBTERM_IM_SOCKET");
    if (im_socket_str == NULL)
      return 0;

    fd = strtoll (im_socket_str, NULL, 0);

    return fd;
}

static guint32
linux_keysym_to_ibus_keyval (guint16 keysym,
                             gchar   keycode)
{
    guint32 kval = KVAL (keysym);
    guint32 keyval = 0;

    switch (KTYP (keysym)) {
    case KT_LATIN:
    case KT_LETTER:
        keyval = linux_to_x[kval];
        break;
    case KT_FN:
        if (kval <= 19)
            keyval = IBUS_KEY_F1 + kval;
        else switch (keysym) {
        case K_FIND:
            keyval = IBUS_KEY_Home; /* or IBUS_KEY_Find */
            break;
        case K_INSERT:
            keyval = IBUS_KEY_Insert;
            break;
        case K_REMOVE:
            keyval = IBUS_KEY_Delete;
            break;
        case K_SELECT:
            keyval = IBUS_KEY_End; /* or IBUS_KEY_Select */
            break;
        case K_PGUP:
            keyval = IBUS_KEY_Prior;
            break;
        case K_PGDN:
            keyval = IBUS_KEY_Next;
            break;
        case K_HELP:
            keyval = IBUS_KEY_Help;
            break;
        case K_DO:
            keyval = IBUS_KEY_Execute;
            break;
        case K_PAUSE:
            keyval = IBUS_KEY_Pause;
            break;
        case K_MACRO:
            keyval = IBUS_KEY_Menu;
            break;
        default:;
        }
        break;
    case KT_SPEC:
        switch (keysym) {
        case K_ENTER:
            keyval = IBUS_KEY_Return;
            break;
        case K_BREAK:
            keyval = IBUS_KEY_Break;
            break;
        case K_CAPS:
            keyval = IBUS_KEY_Caps_Lock;
            break;
        case K_NUM:
            keyval = IBUS_KEY_Num_Lock;
            break;
        case K_HOLD:
            keyval = IBUS_KEY_Scroll_Lock;
            break;
        case K_COMPOSE:
            keyval = IBUS_KEY_Multi_key;
            break;
        default:;
        }
        break;
    case KT_PAD:
        switch (keysym) {
        case K_PPLUS:
            keyval = IBUS_KEY_KP_Add;
            break;
        case K_PMINUS:
            keyval = IBUS_KEY_KP_Subtract;
            break;
        case K_PSTAR:
            keyval = IBUS_KEY_KP_Multiply;
            break;
        case K_PSLASH:
            keyval = IBUS_KEY_KP_Divide;
            break;
        case K_PENTER:
            keyval = IBUS_KEY_KP_Enter;
            break;
        case K_PCOMMA:
            keyval = IBUS_KEY_KP_Separator;
            break;
        case K_PDOT:
            keyval = IBUS_KEY_KP_Decimal;
            break;
        case K_PPLUSMINUS:
            keyval = IBUS_KEY_KP_Subtract;
            break;
        default:
            if (kval <= 9)
                keyval = IBUS_KEY_KP_0 + kval;
        }
        break;

        /*
         * KT_DEAD keys are for accelerated diacritical creation.
         */
    case KT_DEAD:
        switch (keysym) {
        case K_DGRAVE:
            keyval = IBUS_KEY_dead_grave;
            break;
        case K_DACUTE:
            keyval = IBUS_KEY_dead_acute;
            break;
        case K_DCIRCM:
            keyval = IBUS_KEY_dead_circumflex;
            break;
        case K_DTILDE:
            keyval = IBUS_KEY_dead_tilde;
            break;
        case K_DDIERE:
            keyval = IBUS_KEY_dead_diaeresis;
            break;
        default:;
        }
        break;
    case KT_CUR:
        switch (keysym) {
        case K_DOWN:
            keyval = IBUS_KEY_Down;
            break;
        case K_LEFT:
            keyval = IBUS_KEY_Left;
            break;
        case K_RIGHT:
            keyval = IBUS_KEY_Right;
            break;
        case K_UP:
            keyval = IBUS_KEY_Up;
            break;
        default:;
        }
        break;
    case KT_SHIFT:
        switch (keysym) {
        case K_ALTGR:
            keyval = IBUS_KEY_Alt_R;
            break;
        case K_ALT:
            keyval = (keycode == 0x64 ?  IBUS_KEY_Alt_R : IBUS_KEY_Alt_L);
            break;
        case K_CTRL:
            keyval = (keycode == 0x61 ? 
                      IBUS_KEY_Control_R : IBUS_KEY_Control_L);
            break;
        case K_CTRLL:
            keyval = IBUS_KEY_Control_L;
            break;
        case K_CTRLR:
            keyval = IBUS_KEY_Control_R;
            break;
        case K_SHIFT:
            keyval = (keycode == 0x36 ?  IBUS_KEY_Shift_R : IBUS_KEY_Shift_L);
            break;
        case K_SHIFTL:
            keyval = IBUS_KEY_Shift_L;
            break;
        case K_SHIFTR:
            keyval = IBUS_KEY_Shift_R;
            break;
        default:;
        }
        break;
        /*
         * KT_ASCII keys accumulate a 3 digit decimal number that gets
         * emitted when the shift state changes. We can't emulate that.
         */
    case KT_ASCII:
        break;
    case KT_LOCK:
        if (keysym == K_SHIFTLOCK)
            keyval = IBUS_KEY_Shift_Lock;
        break;
    default:;
    }

    return keyval;
}

static void
wait_message (MessageType type)
{
}

static void
ibus_fbterm_calculate_modifiers (IBusFbTerm *ibus_fbterm,
                                 guint32     keyval,
                                 char        down)
{
    guint32 mask = 0;

    switch (keyval) {
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
        mask = IBUS_SHIFT_MASK;
        break;
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
        mask = IBUS_CONTROL_MASK;
        break;
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_Meta_L:
        mask = IBUS_MOD1_MASK;
        break;
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Hyper_L:
        mask = IBUS_MOD4_MASK;
        break;
    case IBUS_KEY_ISO_Level3_Shift:
    case IBUS_KEY_Mode_switch:
        mask = IBUS_MOD5_MASK;
        break;
    default:;
    }

    if (mask) {
        if (down)
            ibus_fbterm->modifier_state |= mask;
        else
            ibus_fbterm->modifier_state &= ~mask;
    }
}

static gboolean
ibus_fbterm_init_bus (IBusFbTerm *ibus_fbterm)
{
    IBusBus *bus;

    ibus_init ();

    bus = ibus_bus_new ();

    if (!bus)
      return FALSE;

    g_signal_connect (bus, "disconnected",
                      G_CALLBACK (bus_disconnected_cb), ibus_fbterm);
    ibus_fbterm->bus = bus;
    ibus_fbterm->input_context = fbterm_input_context_new (ibus_fbterm, bus);

    return TRUE;
}

static void
ibus_fbterm_connect (IBusFbTerm *ibus_fbterm)
{
    Message msg;
    int length;

    g_return_if_fail (ibus_fbterm != NULL);

    msg.type = Connect;
    msg.len = sizeof (msg);
    msg.raw = 1;

    /* g_io_channel_write_chars() accepts readable chars only. */
    errno = 0;
    length = write (ibus_fbterm->fd, (char *)&msg, msg.len);

    if (length < 0) {
        const gchar *error_message = g_strerror (errno);
        g_warning ("Connection failure %d: %s",
                   errno, error_message ? error_message : "(null)");
    }
}

static void
ibus_fbterm_exit_iochannel (IBusFbTerm *ibus_fbterm)
{
    GError *error = NULL;

    g_io_channel_shutdown (ibus_fbterm->iochannel, TRUE, &error);
    g_io_channel_unref (ibus_fbterm->iochannel);
    ibus_fbterm->iochannel = NULL;
    ibus_quit ();
}

static void
ibus_fbterm_update_info (IBusFbTerm *ibus_fbterm, Info *info)
{
    fbterm_input_context_update_info (ibus_fbterm->input_context, info);
}

static void
ibus_fbterm_active (IBusFbTerm *ibus_fbterm)
{
    g_assert (ibus_fbterm->key_object);
    ibus_fbterm->is_active = TRUE;
    ibus_fbterm->modifier_state = 0;
    fbterm_key_object_reset (ibus_fbterm->key_object);
}

static void
ibus_fbterm_deactive (IBusFbTerm *ibus_fbterm)
{
    ibus_fbterm->is_active = FALSE;
}

static void
ibus_fbterm_process_key_event (IBusFbTerm  *ibus_fbterm,
                               char         *buff,
                               unsigned int length)
{
    unsigned int i;

    if (!ibus_fbterm->is_active)
        return;

    for (i = 0; i < length; i++) {
        gchar down = !(buff[i] & 0x80);
        gchar code = buff[i] & 0x7f;
        guint16 keysym;
        guint32 keyval;
        gboolean processed;

        if (!code) {
            if (i + 2 >= length)
                break;
            code = (buff[++i] & 0x7f) << 7;
            code |= buff[++i] & 0x7f;
            if (!(buff[i] & 0x80) || !(buff [i - 1] & 0x80))
                continue;
        }

        keysym = fbterm_key_object_keycode_to_keysym (ibus_fbterm->key_object,
                                                      code, down);
        keyval = linux_keysym_to_ibus_keyval (keysym, code);
        if (!keyval)
            continue;

        g_debug ("send key 0x%x, down %d\n", keyval, down);
        processed = ibus_input_context_process_key_event (
                IBUS_INPUT_CONTEXT (ibus_fbterm->input_context),
                keyval,
                code,
                ibus_fbterm->modifier_state | (down ? 0 : IBUS_RELEASE_MASK));
        if (!processed) {
            gchar *string = fbterm_key_object_keysym_to_term_string (
                    ibus_fbterm->key_object,
                    keysym,
                    down);
            if (string)
                ibus_fbterm_put_im_text (ibus_fbterm, string, strlen (string));
        }
        ibus_fbterm_calculate_modifiers (ibus_fbterm, keyval, down);
    }
}

static void
ibus_fbterm_cursor_position (IBusFbTerm   *ibus_fbterm,
                             unsigned int  x,
                             unsigned int  y)
{
    fbterm_input_context_update_cursor_position (ibus_fbterm->input_context,
                                                 (guint16)x,
                                                 (guint16)y);
    fbterm_input_context_calculate_preedit_window (ibus_fbterm->input_context);
}

static gboolean
ibus_fbterm_process_message (IBusFbTerm *ibus_fbterm,
                             Message    *message)
{
    switch (message->type) {
    case Disconnect:
        ibus_fbterm_exit_iochannel (ibus_fbterm);
        return FALSE;
    case FbTermInfo:
        ibus_fbterm_update_info (ibus_fbterm, &message->info);
        break;
    case Active:
        ibus_fbterm_active (ibus_fbterm);
        break;
    case Deactive:
        ibus_fbterm_deactive (ibus_fbterm);
        break;
    case ShowUI:
    case HideUI:
    case SendKey:
        ibus_fbterm_process_key_event (ibus_fbterm,
                                       message->keys,
                                       message->len - MESSAGE_OFFSET (keys));
        break;
    case CursorPosition:
        ibus_fbterm_cursor_position (ibus_fbterm,
                                     message->cursor.x,
                                     message->cursor.y);
        break;
    case TermMode:
    default:;
    }

    return TRUE;
}

static gboolean
ibus_fbterm_process_messages (IBusFbTerm  *ibus_fbterm,
                              const gchar *buff,
                              int          length)
{
    const gchar *p;
    const gchar *end = buff + length;
    gboolean retval = TRUE;

    p = buff;
    while (p < end) {
        Message *message = MESSAGE (p);
        if (message->len > end - p)
            break;
        retval |= ibus_fbterm_process_message (ibus_fbterm, message);
        p += message->len;
    }

    return retval;
}

static gboolean
iochannel_cb (GIOChannel *iochannel, GIOCondition condition, gpointer data)
{
    IBusFbTerm *ibus_fbterm = (IBusFbTerm *)data;
    int length;
    char buff[MESSAGE_BUFFER_SIZE];

    g_return_if_fail (data != NULL);

    /* g_io_channel_read_chars() accepts readable chars only. */
    errno = 0;
    length = read (ibus_fbterm->fd, buff, MESSAGE_BUFFER_SIZE);

    if (length == -1 && (errno == EAGAIN || errno == EINTR))
        return TRUE;
    else if (length <= 0) {
        ibus_fbterm_exit_iochannel (ibus_fbterm);
        return FALSE;
    }

    return ibus_fbterm_process_messages (ibus_fbterm, buff, length);
}

static gboolean
ibus_fbterm_init_iochannel (IBusFbTerm *ibus_fbterm)
{
    int fd;

    if ((fd = get_im_socket ()) == 0)
        return FALSE;

    ibus_fbterm->fd = fd;
    ibus_fbterm->iochannel = g_io_channel_unix_new (fd);
    g_io_add_watch (ibus_fbterm->iochannel, G_IO_IN | G_IO_HUP | G_IO_ERR,
                    iochannel_cb, ibus_fbterm);
    ibus_fbterm_connect (ibus_fbterm);
    ibus_fbterm->key_object = fbterm_key_object_new ();
    return TRUE;
}

void
ibus_fbterm_put_im_text(IBusFbTerm  *ibus_fbterm,
                        const char  *text,
                        unsigned int length)
{
    g_return_if_fail (ibus_fbterm->fd != -1 && ibus_fbterm->is_active);

    if (!text || !length || (MESSAGE_OFFSET (texts) + length > G_MAXUINT16))
        return;

    char buff[MESSAGE_OFFSET (texts) + length];

    MESSAGE (buff)->type = PutText;
    MESSAGE (buff)->len = sizeof (buff);
    memcpy (MESSAGE (buff)->texts, text, length);

    write (ibus_fbterm->fd, buff, MESSAGE (buff)->len);
}

void
ibus_fbterm_set_im_window (IBusFbTerm  *ibus_fbterm,
                           unsigned int id,
                           guint        x,
                           guint        y,
                           guint        width,
                           guint        height)
{
    Message message;

    g_return_if_fail (ibus_fbterm->fd != -1 && ibus_fbterm->is_active);

    if (id >= NR_IM_WINS)
        return;

    message.type = SetWin;
    message.len = sizeof (message);
    message.win.winid = id;
    message.win.rect.x = x;
    message.win.rect.y = y;
    message.win.rect.w = width;
    message.win.rect.h = height;

    write (ibus_fbterm->fd, (char *)&message, message.len);
    wait_message (AckWin);
}

void
ibus_fbterm_fill_rect (IBusFbTerm    *ibus_fbterm,
                       guint          x,
                       guint          y,
                       guint          width,
                       guint          height,
                       unsigned char  color)
{
    Message message;
    message.type = FillRect;
    message.len = sizeof (message);

    message.fillRect.rect.x = x;
    message.fillRect.rect.y = y;
    message.fillRect.rect.w = width;
    message.fillRect.rect.h = height;
    message.fillRect.color = color;

    write (ibus_fbterm->fd, (char *)&message, message.len);
}

void
ibus_fbterm_draw_text (IBusFbTerm    *ibus_fbterm,
                       guint16        x,
                       guint16        y,
                       unsigned char  fc,
                       unsigned char  bc,
                       const gchar   *text,
                       guint16        length)
{
    char buff[MESSAGE_OFFSET (drawText.texts) + length];

    g_return_if_fail (ibus_fbterm->fd != -1);
    g_return_if_fail (text != NULL && length >= 0);

    MESSAGE (buff)->type = DrawText;
    MESSAGE (buff)->len = sizeof (buff);
    MESSAGE (buff)->drawText.x = x;
    MESSAGE (buff)->drawText.y = y;
    MESSAGE (buff)->drawText.fc = fc;
    MESSAGE (buff)->drawText.bc = bc;
    memcpy (MESSAGE (buff)->drawText.texts, text, length);
    write (ibus_fbterm->fd, buff, MESSAGE (buff)->len);
}

int
main (int argc, char **argv)
{
    IBusFbTerm *ibus_fbterm;

    sleep (10);
    signal (SIGINT, sighandler);
    signal (SIGTERM, sighandler);

    ibus_fbterm = g_slice_new0 (IBusFbTerm);

    if (!ibus_fbterm_init_bus (ibus_fbterm))
        exit (EXIT_FAILURE);

    if (!ibus_fbterm_init_iochannel (ibus_fbterm))
        exit (EXIT_FAILURE);

    ibus_main ();

    g_slice_free (IBusFbTerm, ibus_fbterm);

    exit (EXIT_SUCCESS);
}
