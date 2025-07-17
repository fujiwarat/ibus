#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_close() */
#include <fcntl.h> /* creat() */
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static gchar *m_tmpfile_window_focus;
static gchar *m_tmpfile_result;
static GString *m_result;
static gchar *m_session_name;
static gboolean m_list_toplevel;

static const gunichar test_results[][60] = {
   { 'a', '<', 'b', '>', 'c', '?', 'd', ':', 'e', '"', 'f', '{', 'g', '|', 0 },
#if 0
   { '~', 'A', '!', 'B', '@', 'C', '#', 'D', '(', 'E', ')', 'F', '+', 'G', 0 },
#endif
   { 0 }
};

static const GOptionEntry entries[] =
{
    { "focus-ipc", 'i', 0, G_OPTION_ARG_STRING, &m_tmpfile_window_focus,
      "Specify the IPC file to notify focus-in", "IPC" },
    { "result", 'r', 0, G_OPTION_ARG_STRING, &m_tmpfile_result,
      "Specify the RESULT file to log the test result", "RESULT" },
    { NULL }
};

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


static void
window_destroy_cb (void)
{
    GError *error = NULL;
    gchar *str = g_string_free (m_result, FALSE);

#if GTK_CHECK_VERSION (4, 0, 0)
    m_list_toplevel = FALSE;
#else
    gtk_main_quit ();
#endif
    g_clear_pointer (&m_session_name, g_free);
    if (!str)
        return;
    if (m_tmpfile_result &&
        !g_file_set_contents (m_tmpfile_result, str, -1, &error)) {
        g_test_fail_printf ("Failed to save %s: %s",
                            m_tmpfile_result, error->message);
        g_error_free (error);
    } else if (!m_tmpfile_result) {
        g_test_message ("%s", str);
    }
    g_free (str);
}


static gboolean
event_controller_enter_delay (gpointer user_data)
{
    GtkEventController *controller = (GtkEventController *)user_data;
    GtkWidget *text = gtk_event_controller_get_widget (controller);
    static int i = 0;

    if (gtk_widget_get_realized (text)) {
        int fd;
        GError *error = NULL;
        if (m_tmpfile_window_focus) {
            fd = g_creat (m_tmpfile_window_focus,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (!g_close (fd, &error)) {
                g_warning ("Failed to create %s IPC file: %s",
                           m_tmpfile_window_focus, error->message);
                g_error_free (error);
            }
        }
        return G_SOURCE_REMOVE;
    }
    if (i++ == 10) {
        g_test_fail_printf ("Window is not realized with %d times", i);
        window_destroy_cb ();
        return G_SOURCE_REMOVE;
    }
    g_test_message ("event_controller_enter_delay %d", i);
    return G_SOURCE_CONTINUE;
}


static void
event_controller_enter_cb (GtkEventController *controller,
                           gpointer            user_data)
{
    static guint id = 0;

    g_test_message ("EventController emits \"enter\" signal");
    if (id)
        return;
    /* See ibus-compose:event_controller_enter_cb() */
    if (is_integrated_desktop ()) {
        id = g_timeout_add_seconds (3,
                                    event_controller_enter_delay,
                                    controller);
    } else {
        id = g_idle_add (event_controller_enter_delay, controller);
    }
}


static void
window_inserted_text_cb (GtkEntryBuffer *buffer,
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

    g_test_message ("Window emits \"inserted-text\" %s %u", chars, nchars);
    g_assert (nchars == 1);
    /* NULL is inserted in GTK3 */
    if (!(code = g_utf8_get_char (chars)))
        return;
    if (code != test_results[i][j++]) {
        test = RED "FAIL" NC;
        g_string_append_printf (m_result,
                                "%05d:%05d %s expected: %04X typed: %04X\n",
                                i, j - 1,
                                test,
                                test_results[i][j - 1],
                                code);
    } else if (test_results[i][j]) {
        return;
    }

    g_string_append (m_result, GREEN "PASS" NC " ");
    for (k = 0; k < j; k++) {
        g_string_append_printf (m_result, "%lc(%X) ",
                                test_results[i][k], test_results[i][k]);
    }
    g_string_append (m_result, "\n");

    ++i;
    j = 0;
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_editable_set_text (GTK_EDITABLE (entry), "");
#else
    gtk_entry_set_text (GTK_ENTRY (entry), "");
#endif
    if (!test_results[i][j]) {
       g_assert (!j);
       window_destroy_cb ();
    }
}


static void
create_window ()
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
                      G_CALLBACK (window_destroy_cb), NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    controller = gtk_event_controller_focus_new ();
    g_signal_connect (controller, "enter",
                      G_CALLBACK (event_controller_enter_cb), NULL);
    gtk_widget_add_controller (window, controller);
#else
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (window_focus_in_event_cb), NULL);
#endif
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (window_inserted_text_cb), entry);
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_window_set_child (GTK_WINDOW (window), entry);
    gtk_window_set_focus (GTK_WINDOW (window), entry);
    gtk_window_present (GTK_WINDOW (window));
#else
    gtk_container_add (GTK_CONTAINER (window), entry);
    gtk_widget_show_all (window);
#endif
}


int
main (int argc, char *argv[])
{
    GOptionContext *context;
    GError *error = NULL;

    g_test_init (&argc, &argv, NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_init ();
#else
    gtk_init (&argc, &argv);
#endif

    context = g_option_context_new ("- test window");
    g_option_context_add_main_entries (context, entries, "testwindow");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");

#if GTK_CHECK_VERSION (4, 0, 0)
    m_list_toplevel = TRUE;
#endif
    m_result = g_string_new (NULL);
    create_window ();

#if GTK_CHECK_VERSION (4, 0, 0)
    while (m_list_toplevel)
        g_main_context_iteration (NULL, TRUE);
#else
    gtk_main ();
#endif

    return EXIT_SUCCESS;
}
