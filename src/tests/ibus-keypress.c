/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (c) 2025 Peter Hutterer <peter.hutterer@who-t.net>
 * Copyright (C) 2025 Red Hat, Inc.
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

#include <gtk/gtk.h>
#include <glib/gstdio.h> /* g_access() */
#include "ibus.h"
#include <fcntl.h> /* creat() */
#include <locale.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "ibus-keypress-data.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"
#define EOL   "END_OF_LIFE\n"
#define FAILED_ENGINE "FAILED_ENGINE\n"
#define IO_CHANNEL_BUFF_SIZE 1024
#define READY "READY\n"

typedef enum {
    TEST_PROCESS_KEY_EVENT,
    TEST_CREATE_ENGINE,
    TEST_DELAYED_FOCUS_IN
} TestIDleCategory;

typedef struct _GTestDataMain {
    int    argc;
    char **argv;
} GTestDataMain;

typedef struct _TestIdleData {
    TestIDleCategory category;
    guint            idle_id;
    GMainLoop       *loop;
} TestIdleData;

typedef struct {
    guint source;
    GtkWidget *window;
} WindowDestroyData;

static const gchar *m_arg0;
static const gchar *m_srcdir;
static gchar *m_session_name;
static IBusBus *m_bus;
static IBusEngine *m_engine;
static char *m_engine_is_focused;
static struct uinput_replay_device *m_replay;
static int m_pipe_engine[2];
enum {
    TEST_FD = 0,
    ENGINE_FD = 1,
};

#if GTK_CHECK_VERSION (4, 0, 0)
static void     tp_event_controller_enter_cb (GtkEventController *controller,
                                              gpointer            user_data);
#else
static gboolean tp_window_focus_in_event_cb (GtkWidget     *entry,
                                             GdkEventFocus *event,
                                             gpointer       data);
#endif


gboolean
is_integrated_desktop ()
{
    if (!m_session_name)
        m_session_name = g_strdup (g_getenv ("XDG_SESSION_DESKTOP"));
    if (!m_session_name)
        return FALSE;
    if (!g_ascii_strncasecmp (m_session_name, "gnome", strlen ("gnome")))
       return TRUE;
    return FALSE;
}


gboolean
_wait_for_key_release_cb (gpointer user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;
    /* See ibus-compose:_wait_for_key_release_cb() */
    g_test_message ("Waited 3 seconds for key release event");
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}


static gboolean
ibus_test_engine_io_watch (GIOChannel   *channel,
                           GIOCondition  condition,
                           gpointer      user_data)
{
    GIOStatus status;
    GString *str;
    GError *error = NULL;
    gboolean do_quit = FALSE;
    GMainLoop *loop = (GMainLoop *)user_data;

    if (!(condition & G_IO_ERR)) {
        str = g_string_sized_new (IO_CHANNEL_BUFF_SIZE);
        do status = g_io_channel_read_line_string (channel, str, NULL, &error);
        while (status == G_IO_STATUS_AGAIN);
        if (!g_strcmp0 (str->str, EOL))
            do_quit = TRUE;
        g_string_free (str, TRUE);
        if (do_quit) {
            g_main_loop_quit (loop);
            return G_SOURCE_REMOVE;
        }
    }
    return G_SOURCE_CONTINUE;
}


static gboolean
tp_io_watch_main (GIOChannel   *channel,
                  GIOCondition  condition,
                  gpointer      user_data)
{
    GIOStatus status;
    GString *str;
    GError *error = NULL;
    gboolean do_quit = FALSE;

    if (!(condition & G_IO_ERR)) {
        str = g_string_sized_new (IO_CHANNEL_BUFF_SIZE);
        do status = g_io_channel_read_line_string (channel, str, NULL, &error);
        while (status == G_IO_STATUS_AGAIN);
        if (!g_strcmp0 (str->str, READY)) {
            GtkWidget *window = user_data;
#if GTK_CHECK_VERSION (4, 0, 0)
            gtk_window_present (GTK_WINDOW (window));
#else
            gtk_widget_show_all (window);
#endif
        }
        if (!g_strcmp0 (str->str, FAILED_ENGINE))
            do_quit = TRUE;
        g_string_free (str, TRUE);
        if (do_quit) {
            ibus_quit ();
            return G_SOURCE_REMOVE;
        }
    }
    return G_SOURCE_CONTINUE;
}


gboolean
ibus_test_engine_idle_cb (gpointer user_data)
{
    TestIdleData *data = (TestIdleData *)user_data;
    static int n = 0;
    gboolean terminate_program = FALSE;

    g_assert (data);
    switch (data->category) {
    case TEST_PROCESS_KEY_EVENT:
        if (n++ < 30000) {
            if (!(n % 10))
                fprintf (stderr, "Waiting for process_key_event %dth times\n", n);
            return G_SOURCE_CONTINUE;
        }
        fprintf (stderr, "process_key_event is timeout.\n");
        g_main_loop_quit (data->loop);
        break;
    case TEST_CREATE_ENGINE:
        g_test_fail_printf ("\"create-engine\" signal is timeout.");
        terminate_program = TRUE;
        g_main_loop_quit (data->loop);
        break;
    case TEST_DELAYED_FOCUS_IN:
        if (m_engine_is_focused) {
            data->idle_id = 0;
            n = 0;
            g_main_loop_quit (data->loop);
            return G_SOURCE_REMOVE;
        }
        if (n++ < 10) {
            g_test_message ("Waiting for \"focus-in\" signal %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("\"focus-in\" signal is timeout.");
        g_main_loop_quit (data->loop);
        terminate_program = TRUE;
        n = 0;
        break;
    default:
        g_test_fail_printf ("Idle func is called by wrong category:%d.",
                            data->category);
        break;
    }
    if (terminate_program)
        ibus_quit ();
    return G_SOURCE_REMOVE;
}


static void
ibus_test_engine_engine_focus_in_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                    gchar      *object_path,
                    gchar      *client,
#endif
                    gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("Engine:focus-in-cb(%s, %s)", object_path, client);
    m_engine_is_focused = g_strdup (client);
#else
    g_test_message ("Engine:focus-in-cb()");
    m_engine_is_focused = g_strdup ("No named");
#endif
}


static void
ibus_test_engine_engine_focus_out_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                     gchar      *object_path,
#endif
                     gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("Engine:focus-out-cb(%s, %s)", object_path, client);
#else
    g_test_message ("Engine:focus-out-cb()");
#endif
    g_clear_pointer (&m_engine_is_focused, g_free);
}


static gboolean
ibus_test_engine_engine_process_key_event_cb (IBusEngine *engine,
                                              guint       keyval,
                                              guint       keycode,
                                              guint       state,
                                              gpointer    user_data)
{
    g_debug ("Engine:process-key-event(keyval:%X, keycode:%u, state:%x)",
             keyval, keycode, state);
    return FALSE;
}


static IBusEngine *
ibus_test_engine_create_engine_cb (IBusFactory *factory,
                                   const gchar *name,
                                   gpointer     user_data)
{
    static int i = 1;
    gchar *engine_path;
    TestIdleData *data = (TestIdleData *)user_data;

    g_test_message ("Factory:create-engine()");
    g_assert (data);
    if (!data->idle_id)
        return NULL;

    engine_path = g_strdup_printf ("/org/freedesktop/IBus/engine/simpletest/%d",
                                   i++);
    m_engine = (IBusEngine *)g_object_new (
            IBUS_TYPE_ENGINE_SIMPLE,
            "engine-name",      name,
            "object-path",      engine_path,
            "connection",       ibus_bus_get_connection (m_bus),
#ifdef IBUS_FOCUS_IN_ID
            "has-focus-id",     TRUE,
#endif
            NULL);
    g_free (engine_path);

    m_engine_is_focused = NULL;
#ifdef IBUS_FOCUS_IN_ID
    g_signal_connect (m_engine, "focus-in-id",
#else
    g_signal_connect (m_engine, "focus-in",
#endif
                      G_CALLBACK (ibus_test_engine_engine_focus_in_cb), NULL);
#ifdef IBUS_FOCUS_IN_ID
    g_signal_connect (m_engine, "focus-out-id",
#else
    g_signal_connect (m_engine, "focus-out",
#endif
                      G_CALLBACK (ibus_test_engine_engine_focus_out_cb), NULL);
    g_signal_connect (m_engine, "process-key-event",
                      G_CALLBACK (ibus_test_engine_engine_process_key_event_cb), NULL);

    g_source_remove (data->idle_id);
    data->idle_id = 0;
    return m_engine;
}

static int
ibus_test_engine_register_real (gpointer user_data)
{
    TestIdleData *data = (TestIdleData *)user_data;
    IBusFactory *factory;
    IBusComponent *component;
    IBusEngineDesc *desc;

    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    data->idle_id = g_timeout_add_seconds (20, ibus_test_engine_idle_cb, data);
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (ibus_test_engine_create_engine_cb), data);

    component = ibus_component_new (
            "org.freedesktop.IBus.SimpleTest",
            "Simple Engine Test",
            "0.0.1",
            "GPL",
            "Takao Fujiwara <takao.fujiwara1@gmail.com>",
            "https://github.com/ibus/ibus/wiki",
            "",
            "ibus");
    desc = ibus_engine_desc_new (
            "xkbtest:us::eng",
            "XKB Test",
            "XKB Test",
            "en",
            "GPL",
            "Takao Fujiwara <takao.fujiwara1@gmail.com>",
            "ibus-engine",
            "us");
    ibus_component_add_engine (component, desc);
    ibus_bus_register_component (m_bus, component);

    write (m_pipe_engine[ENGINE_FD], READY, sizeof (READY));

    return TRUE;
}


static int
ibus_test_engine_register (gpointer user_data)
{
    if (!ibus_test_engine_register_real (user_data))
        return G_SOURCE_CONTINUE;

    return G_SOURCE_REMOVE;
}


static void
ibus_test_engine_exec()
{
    static TestIdleData _data = { .category = TEST_CREATE_ENGINE,
                                  .idle_id = 0 };
    GIOChannel *channel;

    _data.loop = g_main_loop_new (NULL, TRUE);

    g_assert ((channel =  g_io_channel_unix_new (m_pipe_engine[ENGINE_FD])));
    g_io_channel_set_buffer_size (channel, IO_CHANNEL_BUFF_SIZE);
    g_io_add_watch (channel, G_IO_IN | G_IO_ERR, ibus_test_engine_io_watch, _data.loop);

    /* IBusBus should not be generated in g_idle_add() */
    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_critical ("ibus-daemon is not running.");
        exit (EXIT_FAILURE);
    }

    g_idle_add (ibus_test_engine_register, &_data);
    g_main_loop_run (_data.loop);
    g_io_channel_unref (channel);
    if (_data.idle_id) {
        write (m_pipe_engine[ENGINE_FD], FAILED_ENGINE, sizeof (FAILED_ENGINE));
        fsync (m_pipe_engine[ENGINE_FD]);
        close (m_pipe_engine[ENGINE_FD]);
        exit (EXIT_FAILURE);
    }
    close (m_pipe_engine[ENGINE_FD]);
    exit (EXIT_SUCCESS);
}


static void
tp_window_destroy_cb (void)
{
    ibus_quit ();
}


static gboolean
tp_destroy_window (gpointer user_data)
{
    WindowDestroyData *data = user_data;

    data->source = 0;

    g_info ("Destroying window after timeout");
    gtk_window_destroy (GTK_WINDOW (data->window));

    data->window = NULL;

    write (m_pipe_engine[TEST_FD], EOL, sizeof (EOL));
    fsync (m_pipe_engine[TEST_FD]);

    return G_SOURCE_REMOVE;
}


static struct uinput_replay_device *
tp_create_keypress (void)
{
    GError *error = NULL;
    struct uinput_replay_device *device;

#ifdef UINPUT_REPLAY_DEVICE_EMBED_DATA
    /* FIXME: Need a script to convert libinput-test.yml to GResource. */
    m_replay = uinput_replay_create_keyboard (&error);
#else
    gchar *datadir;
    char *recording;

    g_assert (m_arg0);
    if (m_srcdir) {
        datadir = g_strdup (m_srcdir);
    } else {
        datadir = g_path_get_dirname (m_arg0);
        if (g_str_has_suffix (datadir, "/.libs"))
            *(datadir + strlen (datadir) - 6) = '\0';
    }

    if (!(recording = g_build_filename (datadir, "libinput-test.yml", NULL))) {
        g_test_fail_printf ("Failed to allocate YAML file");
        return NULL;
    }
    g_free (datadir);
    device = uinput_replay_create_device (recording, &error);
    g_free (recording);
#endif

    if (!device) {
        g_test_fail_printf ("Failed to create uinput device: %s",
                            error->message);
        g_error_free (error);
    }
    return device;
}

static int
tp_exec_keypress (gpointer unused)
{
    g_test_message ("Started keypress");
#ifdef UINPUT_REPLAY_DEVICE_EMBED_DATA
    guint prev_time = 0;
    /* Because uinput doesn't take that long (for our recordings anyway) we can
     * simply create and run this here this here. */
    for (size_t i = 0; i < G_N_ELEMENTS (test_data); ++i) {
        uinput_replay_device_replay_event (m_replay,
                                           &test_data[0][i],
                                           &prev_time);
    }
#else
    g_assert (&test_data[0][0]);
    uinput_replay_device_replay (m_replay);
#endif
    g_test_message ("Finished keypress");

    return G_SOURCE_REMOVE;
}


static void
tp_set_engine_cb (GObject      *object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
    IBusBus *bus = IBUS_BUS (object);
    GError *error = NULL;
    static TestIdleData data = { .category = TEST_PROCESS_KEY_EVENT,
                                 .idle_id = 0 };
    int i;

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_critical ("set engine failed: %s", error->message);
        g_error_free (error);
        return;
    }

#if 0 /* FIXME */
    /* See ibus-compose:set_engine_cb() */
    if (is_integrated_desktop () && g_getenv ("IBUS_DAEMON_WITH_SYSTEMD")) {
        g_test_message ("Start tiny \"focus-in\" signal test");
        for (i = 0; i < 3; i++) {
            data.category = TEST_DELAYED_FOCUS_IN;
            data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
            g_main_loop_run (m_loop);
            if (data.idle_id != 0)
                return;
        }
        g_test_message ("End tiny \"focus-in\" signal test");
        data.category = TEST_PROCESS_KEY_EVENT;
    }
#endif

    g_idle_add(tp_exec_keypress, NULL);
}


static void
tp_set_engine (gpointer user_data)
{
    g_test_message ("setting engine");
    g_assert (m_bus != NULL);
    ibus_bus_set_global_engine_async (m_bus,
                                      "xkbtest:us::eng",
                                      -1,
                                      NULL,
                                      tp_set_engine_cb,
                                      user_data);
}


#if GTK_CHECK_VERSION (4, 0, 0)
static gboolean
tp_event_controller_enter_delay (gpointer user_data)
{
    GtkEventController *controller = (GtkEventController *)user_data;
    GtkWidget *text = gtk_event_controller_get_widget (controller);
    static int i = 0;

    if (gtk_widget_get_realized (text)) {
        tp_set_engine (user_data);
        return G_SOURCE_REMOVE;
    }
    if (i++ == 10) {
        g_test_fail_printf ("Window is not realized with %d times", i);
        ibus_quit ();
        return G_SOURCE_REMOVE;
    }
    g_test_message ("%s %d", __func__, i);
    return G_SOURCE_CONTINUE;
}


static void
tp_event_controller_enter_cb (GtkEventController *controller,
                           gpointer            user_data)
{
    static guint id = 0;

    g_test_message ("EventController:enter()");
    if (id)
        return;
    /* See ibus-compose:event_controller_enter_cb() */
    if (is_integrated_desktop ()) {
        id = g_timeout_add_seconds (1,
                                    tp_event_controller_enter_delay,
                                    controller);
    } else {
        id = g_idle_add (tp_event_controller_enter_delay, controller);
    }
}

#else

static gboolean
tp_window_focus_in_event_cb (GtkWidget     *entry,
                          GdkEventFocus *event,
                          gpointer       user_data)
{
    g_test_message ("Entry:focus-in()");
    tp_set_engine (entry);
    return FALSE;
}
#endif


static void
tp_window_inserted_text_cb (GtkEntryBuffer *buffer,
                         guint           position,
                         const gchar    *chars,
                         guint           nchars,
                         gpointer        user_data)
{
    GtkWidget *entry = user_data;
    static int i = 0;
    static int j = 0;
    int k;
    gunichar code;
    const gchar *test;

    g_test_message ("Entry:inserted-text(chars:%s, nchars:%u)", chars, nchars);
    g_assert (nchars == 1);
    /* NULL is inserted in GTK3 */
    if (!(code = g_utf8_get_char (chars)))
        return;
    if (code != test_results[i][j++]) {
        test = RED "FAIL" NC;
        g_test_fail_printf ("%05d:%05d %s expected: %04X typed: %04X",
                            i, j - 1,
                            test,
                            test_results[i][j - 1],
                            code);
    } else if (test_results[i][j]) {
        return;
    } else {
        g_print (GREEN "PASS" NC " ");
        for (k = 0; k < j; k++)
            g_print ("%lc(%X) ", test_results[i][k], test_results[i][k]);
        g_print ("\n");
    }

    ++i;
    j = 0;
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_editable_set_text (GTK_EDITABLE (entry), "");
#else
    gtk_entry_set_text (GTK_ENTRY (entry), "");
#endif
    if (!test_results[i][j]) {
       g_assert (!j);

       write (m_pipe_engine[TEST_FD], EOL, sizeof (EOL));
       fsync (m_pipe_engine[TEST_FD]);
       close (m_pipe_engine[TEST_FD]);
       ibus_quit ();
    }
}


static GtkWidget *
tp_create_window ()
{
    GtkWidget *window;
    GtkWidget *entry = gtk_entry_new ();
    GtkEntryBuffer *buffer;
#if GTK_CHECK_VERSION (4, 0, 0)
    GtkEventController *controller;

    window = gtk_window_new ();
#else
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif

    g_signal_connect (window, "destroy",
                      G_CALLBACK (tp_window_destroy_cb), NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    controller = gtk_event_controller_focus_new ();
    g_signal_connect (controller, "enter",
                      G_CALLBACK (tp_event_controller_enter_cb), NULL);
    gtk_widget_add_controller (window, controller);
#else
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (tp_window_focus_in_event_cb), NULL);
#endif
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (tp_window_inserted_text_cb), entry);
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_window_set_child (GTK_WINDOW (window), entry);
    gtk_window_set_focus (GTK_WINDOW (window), entry);
#else
    gtk_container_add (GTK_CONTAINER (window), entry);
#endif

    return window;
}


static void
test_init (void)
{
    char *tty_name = ttyname (STDIN_FILENO);
    GMainLoop *loop;
    guint idle_id = 0;

    loop = g_main_loop_new (NULL, TRUE);
    g_test_message ("Test on %s", tty_name ? tty_name : "(null)");
    if (tty_name && g_strstr_len (tty_name, -1, "pts")) {
        idle_id = g_timeout_add_seconds (3, _wait_for_key_release_cb, loop);
        g_main_loop_run (loop);
    }
    g_main_loop_unref (loop);
}


static void
test_keypress (gconstpointer user_data)
{
#if !GTK_CHECK_VERSION (4, 0, 0)
    GTestDataMain *data = (GTestDataMain *)user_data;
#endif
    GIOChannel *channel;
    WindowDestroyData destroy_data = { 0, };

    int rc = socketpair(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0, m_pipe_engine);
    g_assert (rc == 0);

    if (!fork ()) {
        close(m_pipe_engine[TEST_FD]);
        ibus_test_engine_exec ();
        abort(); /* unreachable */
    }
    close(m_pipe_engine[ENGINE_FD]);

    /* FIXME: Calling this before the fork means we never get an ibus_bus_new().
     * let's move this here for now.
     */
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_init ();
#else
    gtk_init (data->argc, data->argv);
#endif

    m_bus = ibus_bus_new ();

    if (!(m_replay = tp_create_keypress ()))
        return;

    if (!m_replay) {
        g_warning ("Failed to create uinput device");
        return;
    }

    GtkWidget *window = tp_create_window ();
    destroy_data.window = window;
    destroy_data.source = g_timeout_add_seconds (30,
                                                 tp_destroy_window,
                                                 &destroy_data);

    g_assert ((channel =  g_io_channel_unix_new (m_pipe_engine[0])));
    g_io_channel_set_buffer_size (channel, IO_CHANNEL_BUFF_SIZE);
    g_io_add_watch (channel, G_IO_IN | G_IO_ERR, tp_io_watch_main, window);

    /* The main test loop */
    ibus_main ();

    uinput_replay_device_destroy (g_steal_pointer (&m_replay));
    g_clear_pointer (&m_session_name, g_free);
    if (destroy_data.source)
	g_source_remove (destroy_data.source);
    if (destroy_data.window)
	gtk_window_destroy (GTK_WINDOW (destroy_data.window));
    close (m_pipe_engine[TEST_FD]);
}


int
main (int argc, char *argv[])
{
    static GTestDataMain data;
    setlocale (LC_ALL, "");

    m_arg0 = argv[0];
    m_srcdir = argv[1];

    /* Avoid a warning of "AT-SPI: Could not obtain desktop path or name"
     * with gtk_main().
     */
    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");
    g_test_init (&argc, &argv, NULL);
    ibus_init ();
    g_test_add_func ("/ibus-keypress/test-init", test_init);
    data.argc = argc;
    data.argv = argv;
    g_test_add_data_func ("/ibus-keypress/keypress", &data, test_keypress);

    return g_test_run ();
}
