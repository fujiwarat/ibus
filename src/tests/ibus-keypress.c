#include <glib.h>
#include <glib/gstdio.h> /* g_access() */
#include "ibus.h"
#include <locale.h>
#include <stdlib.h>

#include "uinput-replay.h"


typedef enum {
    TEST_SET_FOCUS,
    TEST_END_WINDOW,
    TEST_CREATE_ENGINE,
    TEST_DELAYED_FOCUS_IN
} TestIDleCategory;

typedef struct _TestIdleData {
    TestIDleCategory category;
    guint            idle_id;
} TestIdleData;


static const gchar *m_arg0;
static const gchar *m_srcdir;
static gchar *m_tmpfile_window_focus;
static gchar *m_tmpfile_result;
static GPid m_pid_window;
static gchar *m_session_name;
static IBusBus *m_bus;
static IBusEngine *m_engine;
static GMainLoop *m_loop;
static char *m_engine_is_focused;


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
    g_test_message ("Wait for 3 seconds for key release event");
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}


gboolean
idle_cb (gpointer user_data)
{
    TestIdleData *data = (TestIdleData *)user_data;
    static int n = 0;
    gboolean terminate_program = FALSE;

    g_assert (data);
    switch (data->category) {
    case TEST_SET_FOCUS:
        if (g_access (m_tmpfile_window_focus, 0) != -1) {
            data->idle_id = 0;
            n = 0;
            g_unlink (m_tmpfile_window_focus);
            g_main_loop_quit (m_loop);
            return G_SOURCE_REMOVE;
        }
        if (n++ < 600) {
            if (!(n % 30))
                g_test_message ("Waiting for focus signal %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("enter focus is timeout.");
        g_main_loop_quit (m_loop);
        break;
    case TEST_END_WINDOW:
        if (g_access (m_tmpfile_result, 0) != -1) {
            data->idle_id = 0;
            n = 0;
            g_main_loop_quit (m_loop);
            g_test_message ("Terminate window");
            return G_SOURCE_REMOVE;
        }
        if (n++ < 600) {
            if (!(n % 30))
                g_message ("Waiting for closing window %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("Closing window is timeout.");
        terminate_program = TRUE;
        break;
    case TEST_CREATE_ENGINE:
        g_test_fail_printf ("\"create-engine\" signal is timeout.");
        data->idle_id = 0;
        terminate_program = TRUE;
        break;
    case TEST_DELAYED_FOCUS_IN:
        if (m_engine_is_focused) {
            data->idle_id = 0;
            n = 0;
            g_main_loop_quit (m_loop);
            return G_SOURCE_REMOVE;
        }
        if (n++ < 30) {
            if (!(n % 5))
                g_test_message ("Waiting for \"focus-in\" signal %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("\"focus-in\" signal is timeout.");
        g_main_loop_quit (m_loop);
        terminate_program = TRUE;
        break;
    default:
        g_test_fail_printf ("Idle func is called by wrong category:%d.",
                            data->category);
        break;
    }
    if (terminate_program)
        ibus_quit ();
    g_main_loop_quit (m_loop);
    return G_SOURCE_REMOVE;
}


static void
exec_keypress (void)
{
    gchar *datadir;
    char *recording;
    struct uinput_replay_device *replay;

    g_assert (m_arg0);
    if (m_srcdir) {
        datadir = g_strdup (m_srcdir);
    } else {
        datadir = g_path_get_dirname (m_arg0);
        if (g_str_has_suffix (datadir, "/.libs"))
            *(datadir + strlen (datadir) - 6) = '\0';
    }

    recording = g_build_filename (datadir, "libinput-test.yml", NULL);
    g_free (datadir);
    replay = uinput_replay_create_device (recording, NULL);
    g_free (recording);

    if (!replay) {
        g_warning ("Failed to create uinput device");
        return;
    }

    g_test_message ("Started keypress");
    uinput_replay_device_replay(replay);
    g_test_message ("Finished keypress");
}


static void
check_result (void)
{
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    g_assert (m_tmpfile_result);
    if (!g_file_get_contents (m_tmpfile_result, &contents, &length, &error)) {
        g_test_fail_printf ("Failed to open %s: %s",
                            m_tmpfile_result, error->message);
        g_error_free (error);
    } else if (length == 0) {
        g_test_fail_printf ("No result files");
    } else if (g_strstr_len (contents, length, "FAIL")) {
        g_test_fail_printf ("%s", contents);
    } else {
        g_test_message ("%s", contents);
    }
    g_free (contents);
    g_unlink (m_tmpfile_result);
}


static void
set_engine_cb (GObject      *object,
               GAsyncResult *res,
               gpointer      user_data)
{
    IBusBus *bus = IBUS_BUS (object);
    GError *error = NULL;
    static TestIdleData data = { .category = TEST_END_WINDOW, .idle_id = 0 };

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_critical ("set engine failed: %s", error->message);
        g_error_free (error);
        return;
    }

    /* See ibus-compose:set_engine_cb() */
    if (is_integrated_desktop () && g_getenv ("IBUS_DAEMON_WITH_SYSTEMD")) {
        g_test_message ("Start tiny \"focus-in\" signal test");
        data.category = TEST_DELAYED_FOCUS_IN;
        data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
        g_main_loop_run (m_loop);
        if (data.idle_id != 0)
            return;
        g_test_message ("End tiny \"focus-in\" signal test");
        data.category = TEST_END_WINDOW;
    }

    exec_keypress ();
    data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
    if (data.idle_id != 0)
        return;
    g_spawn_close_pid (m_pid_window);
    check_result ();
    ibus_quit ();
}


static void
set_engine (gpointer user_data)
{
    g_test_message ("set_engine() is calling");
    g_assert (m_bus != NULL);
    ibus_bus_set_global_engine_async (m_bus,
                                      "xkbtest:us::eng",
                                      -1,
                                      NULL,
                                      set_engine_cb,
                                      user_data);
}


static void
exec_window (void)
{
    gchar *build_dir;
    gchar *window_path = NULL;
    gchar *argv[6];
    GError *error = NULL;

    g_assert (m_arg0);
    g_assert (m_tmpfile_window_focus);
    g_assert (m_tmpfile_result);
    build_dir = g_path_get_dirname (m_arg0);
    if (g_str_has_suffix (build_dir, "/.libs"))
        *(build_dir + strlen (build_dir) - 6) = '\0';
    window_path = g_build_filename (build_dir, "testwindow", NULL);
    argv[0] = window_path;
    argv[1] = "--focus-ipc";
    argv[2] = m_tmpfile_window_focus;
    argv[3] = "--result";
    argv[4] = m_tmpfile_result;
    argv[5] = NULL;
    if (!g_spawn_async (NULL,
                        argv,
                        NULL,
                        G_SPAWN_SEARCH_PATH,
                        NULL,
                        NULL,
                        &m_pid_window,
                        &error)) {
        g_test_fail_printf ("Failed to run testwindow: %s", error->message);
        g_error_free (error);
    }
    g_free (window_path);
    g_free (build_dir);
}


static void
test_init (void)
{
    char *tty_name = ttyname (STDIN_FILENO);
    GMainLoop *loop;
    static guint idle_id = 0;

    if (idle_id) {
        g_test_incomplete ("Test is called twice due to a timeout.");
        return;
    }

    loop = g_main_loop_new (NULL, TRUE);
    g_test_message ("Test on %s", tty_name ? tty_name : "(null)");
    if (tty_name && g_strstr_len (tty_name, -1, "pts")) {
        idle_id = g_timeout_add_seconds (3, _wait_for_key_release_cb, loop);
        g_main_loop_run (loop);
    }
    g_main_loop_unref (loop);
}


static void
engine_focus_in_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                    gchar      *object_path,
                    gchar      *client,
#endif
                    gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("engine_focus_in_cb %s %s", object_path, client);
    m_engine_is_focused = g_strdup (client);
#else
    g_test_message ("engine_focus_in_cb");
    m_engine_is_focused = g_strdup ("No named");
#endif
}


static void
engine_focus_out_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                     gchar      *object_path,
#endif
                     gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("engine_focus_out_cb %s", object_path);
#else
    g_test_message ("engine_focus_out_cb");
#endif
    g_clear_pointer (&m_engine_is_focused, g_free);
}


static gboolean
engine_process_key_event_cb (IBusEngine *engine,
                             guint       keyval,
                             guint       keycode,
                             guint       state,
                             gpointer    user_data)
{
    g_test_message ("engine_process_key_event_cb keyval:%X keycode:%u state:%x",
                    keyval, keycode, state);
    return FALSE;
}


static IBusEngine *
create_engine_cb (IBusFactory *factory,
                  const gchar *name,
                  gpointer     user_data)
{
    static int i = 1;
    gchar *engine_path =
            g_strdup_printf ("/org/freedesktop/IBus/engine/simpletest/%d",
                             i++);
    TestIdleData *data = (TestIdleData *)user_data;

    g_assert (data);
    /* TEST_CREATE_ENGINE timeout */
    if (!data->idle_id)
        return NULL;
    /* Don't reset idle_id to avoid duplicated register_ibus_engine(). */
    g_source_remove (data->idle_id);
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
                      G_CALLBACK (engine_focus_in_cb), NULL);
#ifdef IBUS_FOCUS_IN_ID
    g_signal_connect (m_engine, "focus-out-id",
#else
    g_signal_connect (m_engine, "focus-out",
#endif
                      G_CALLBACK (engine_focus_out_cb), NULL);
    g_signal_connect (m_engine, "process-key-event",
                      G_CALLBACK (engine_process_key_event_cb), NULL);
    return m_engine;
}


static gboolean
register_ibus_engine ()
{
    static TestIdleData data = { .category = TEST_CREATE_ENGINE, .idle_id = 0 };
    IBusFactory *factory;
    IBusComponent *component;
    IBusEngineDesc *desc;

    if (data.idle_id) {
        g_test_incomplete ("Test is called twice due to a timeout.");
        return TRUE;
    }
    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_critical ("ibus-daemon is not running.");
        return FALSE;
    }
    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    data.idle_id = g_timeout_add_seconds (60, idle_cb, &data);
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (create_engine_cb), &data);

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

    return TRUE;
}


static void
test_keypress (void)
{
    static TestIdleData data = { .category = TEST_SET_FOCUS,
                                 .idle_id = 0 };

    if (!register_ibus_engine ())
        return;
    exec_window ();
    data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
    if (data.idle_id != 0)
        return;
    set_engine (NULL);
    ibus_main ();
    g_free (m_session_name);
    g_free (m_tmpfile_window_focus);
    g_free (m_tmpfile_result);
}


int
main (int argc, char *argv[])
{
    int fd;
    GError *error = NULL;

    setlocale (LC_ALL, "");

    m_arg0 = argv[0];
    m_srcdir = argv[1];

    if (!g_strcmp0 (argv[1], "--username")) {
        g_print ("%s\n", g_get_user_name ());
        return EXIT_SUCCESS;
    }

    m_tmpfile_window_focus = g_strdup ("/tmp/ibus-keypress_engine_XXXXXX.log");
    errno = 0;
    if ((fd = g_mkstemp (m_tmpfile_window_focus)) == -1) {
        /* g_warning() before g_test_init() */
        g_warning ("mkstemp is failed: %s", g_strerror (errno));
        g_unlink (m_tmpfile_window_focus);
        exit (EXIT_FAILURE);
    }
    g_close (fd, &error);
    g_unlink (m_tmpfile_window_focus);

    m_tmpfile_result = g_strdup ("/tmp/ibus-keypress_result_XXXXXX.log");
    errno = 0;
    if ((fd = g_mkstemp (m_tmpfile_result)) == -1) {
        g_warning ("mkstemp is failed: %s", g_strerror (errno));
        g_unlink (m_tmpfile_result);
        exit (EXIT_FAILURE);
    }
    g_close (fd, &error);
    g_unlink (m_tmpfile_result);

    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");

    g_test_init (&argc, &argv, NULL);
    ibus_init ();

    g_test_add_func ("/ibus-keypress/test-init", test_init);
    m_loop = g_main_loop_new (NULL, TRUE);
    g_test_add_func ("/ibus-keypress/keyrepss", test_keypress);

    return g_test_run ();
}
