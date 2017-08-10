/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2017 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2017 Red Hat, Inc.
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
#include "server.h"

#include <gio/gio.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "dbusimpl.h"
#include "ibusimpl.h"
#include "global.h"


static BusIBusShare *server = NULL;
static GMainLoop *mainloop = NULL;
static BusDBusImpl *dbus = NULL;
static BusIBusImpl *ibus = NULL;
static gchar *address = NULL;
static gboolean _restart = FALSE;

static void
_restart_server (void)
{
    gchar *exe;
    gint fd;

    exe = g_strdup_printf ("/proc/%d/exe", getpid ());
    exe = g_file_read_link (exe, NULL);

    if (exe == NULL)
        exe = BINDIR "/ibus-daemon";

    /* close all fds except stdin, stdout, stderr */
    for (fd = 3; fd <= sysconf (_SC_OPEN_MAX); fd ++) {
        close (fd);
    }

    _restart = FALSE;
    execv (exe, g_argv);

    /* If the server binary is replaced while the server is running,
     * "readlink /proc/[pid]/exe" might return a path with " (deleted)"
     * suffix. */
    const gchar suffix[] = " (deleted)";
    if (g_str_has_suffix (exe, suffix)) {
        exe [strlen (exe) - sizeof (suffix) + 1] = '\0';
        execv (exe, g_argv);
    }
    g_warning ("execv %s failed!", g_argv[0]);
    exit (-1);
}

/**
 * bus_new_connection_cb:
 * @user_data: always NULL.
 * @returns: TRUE when the function can handle the connection.
 *
 * Handle incoming connections.
 */
static gboolean
bus_new_connection_cb (GDBusServer     *server,
                       GDBusConnection *dbus_connection,
                       gpointer         user_data)
{
    BusConnection *connection = bus_connection_new (dbus_connection);
    bus_dbus_impl_new_connection (dbus, connection);

    if (g_object_is_floating (connection)) {
        /* bus_dbus_impl_new_connection couldn't handle the connection. just delete the connection and return TRUE
         * (so that other connection handler will not handle the deleted connection.) */
        ibus_object_destroy ((IBusObject *)connection);
        g_object_unref (connection);
    }
    return TRUE;
}

void
bus_server_init (void)
{
    dbus = bus_dbus_impl_get_default ();
    ibus = bus_ibus_impl_get_default ();
    bus_dbus_impl_register_object (dbus, (IBusService *)ibus);

    server = bus_ibus_share_get_default ();
    address = g_strdup ("unix:abstract=/tmp/dbus-no-longer-used");
    ibus_write_address (address);
}

int
bus_server_start_with_socketpair (const gchar *guid) {
    int sv[2];
    GError *error = NULL;
    GSocket *socket;
    GSocketConnection *socket_connection;
    GDBusConnection *connection;
    gboolean claimed;

    errno = 0;
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, sv) != 0)
        g_error ("Failed to generate socket: %s", strerror (errno));
    if (sv[0] <= 0 || sv[1] <= 0)
        g_error ("Wrong socket descriptors : %d, %d", sv[0], sv[1]);
    socket = g_socket_new_from_fd (sv[1], &error);
    g_assert_no_error (error);
    socket_connection = g_socket_connection_factory_create_connection (socket);
    g_assert (socket_connection != NULL);
    g_object_unref (socket);
    connection = g_dbus_connection_new_sync (G_IO_STREAM (socket_connection),
                                             guid,
                                             G_DBUS_CONNECTION_FLAGS_NONE,
                                             NULL,
                                             NULL,
                                             &error);
    g_dbus_connection_set_exit_on_close (connection, TRUE);
    g_assert_no_error (error);
    g_object_unref (socket_connection);
    claimed = bus_new_connection_cb (NULL, connection, NULL);
    if (claimed)
        g_dbus_connection_start_message_processing (connection);
    g_object_unref (connection);

    return sv[0];
}

const gchar *
bus_server_get_address (void)
{
    return address;
}

void
bus_server_run (void)
{
    g_return_if_fail (server);

    /* create and run main loop */
    mainloop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (mainloop);

    ibus_object_destroy (IBUS_OBJECT (server));

    /* When _ibus_exit() is called, bus_ibus_impl_destroy() needs
     * to be called so that waitpid() prevents the processes from
     * becoming the daemons. So we run execv() after
     * ibus_object_destroy(ibus) is called here. */
    if (_restart) {
        _restart_server ();

        /* should not reach here */
        g_assert_not_reached ();
    }
}

void
bus_server_quit (gboolean restart)
{
    _restart = restart;
    if (mainloop)
        g_main_loop_quit (mainloop);
}
