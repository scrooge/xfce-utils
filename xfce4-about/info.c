/*  xfce4
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *                2002 Xavier MAILLARD (zedek@fxgsproject.org)
 *                2003 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *                2003,2004 Benedikt Meurer (benny@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LC_MESSAGES
#include <locale.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_LIBGTKHTML
#include <libgtkhtml/gtkhtml.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#define SEARCHPATH	(DATADIR G_DIR_SEPARATOR_S "%F.%L:"	\
                         DATADIR G_DIR_SEPARATOR_S "%F.%l:"	\
                         DATADIR G_DIR_SEPARATOR_S "%F")

#define XFCE_COPYRIGHT	"COPYING"
#define XFCE_AUTHORS	"AUTHORS"
#define XFCE_INFO	"INFO"
#define XFCE_BSDL	"BSD"
#define XFCE_GPL	"GPL"
#define XFCE_LGPL	"LGPL"

#define BORDER 6

static GtkWidget *info;

static void
info_help_cb (GtkWidget * w, gpointer data)
{
    xfce_exec ("xfhelp4", FALSE, FALSE, NULL);
}

#ifdef HAVE_LIBGTKHTML
static void
link_clicked (HtmlDocument * htmldoc, const gchar * url, gpointer data)
{
    const gchar *browser;
    gchar command[2048];

    /*
     * launch the browser
     *
     * XXX - We should probably have something like xfbrowser4, or even
     * better: Have the browser configurable through the settings
     * manager.
     */
    if ((browser = g_getenv ("BROWSER")) != NULL)
	g_snprintf (command, sizeof (command), "%s \"%s\"", browser, url);
    else
	g_snprintf (command, sizeof (command),
		    "ns-remote -remote \"openURL(%s)\"", url);

    xfce_exec (command, FALSE, FALSE, NULL);
}
#endif

static gchar*
replace_version (gchar *buffer)
{
    gchar *dst;
    gchar *bp;

    bp = strstr (buffer, "@VERSION@");
    if (bp != NULL)
    {
        const gchar *version = xfce_version_string ();
        gchar *complete_version;
        
#ifdef RELEASE_LABEL
        if (strlen (RELEASE_LABEL))
            complete_version = g_strdup_printf ("%s (%s)", version, RELEASE_LABEL);
        else
#endif
            complete_version = g_strdup (version);

        gsize n = bp - buffer;

        dst = g_new (gchar, strlen (buffer) + strlen (complete_version) + 1);
        memcpy (dst, buffer, n);
        memcpy (dst + n, complete_version, strlen (complete_version));
        strcpy (dst + n + strlen (complete_version), buffer + n + strlen ("@VERSION@"));
        g_free (complete_version);
        g_free (buffer);
        
        return dst;
    }

    return buffer;
}

static void
add_page (GtkNotebook * notebook, const gchar * name, const gchar * filename,
	  gboolean hscrolling)
{
#ifdef HAVE_LIBGTKHTML
    HtmlDocument *htmldoc;
    gboolean usehtml;
#endif
    /*gchar buffer[PATH_MAX + 1];*/
    GtkTextBuffer *textbuffer;
    GtkWidget *textview;
    GtkWidget *label;
    GtkWidget *view;
    GtkWidget *sw;
    GError *err;
    gchar *path;
    gchar *hfilename;
    gchar *buf;
    int n;

    err = NULL;

    label = gtk_label_new (name);
    gtk_widget_show (label);

#ifdef HAVE_LIBGTKHTML
    /* try to find a html file first */
    hfilename =
	g_strconcat (DATADIR, G_DIR_SEPARATOR_S, filename, ".html", NULL);
    path = xfce_get_file_localized (hfilename);
    /* xfce_get_path_localized(buffer, sizeof(buffer), SEARCHPATH,
       hfilename, G_FILE_TEST_IS_REGULAR); */
    g_free (hfilename);

    if (path != NULL)
    {
	usehtml = TRUE;
    }
    else
    {
	/* fallback to plain text files */
	usehtml = FALSE;
#endif

	hfilename = g_strconcat (DATADIR, G_DIR_SEPARATOR_S, filename, NULL);
	path = xfce_get_file_localized (hfilename);
	g_free (hfilename);
	/* xfce_get_path_localized(buffer, sizeof(buffer),
	   SEARCHPATH, filename, G_FILE_TEST_IS_REGULAR); */
#ifdef HAVE_LIBGTKHTML
    }
#endif

    if (g_file_get_contents (path, &buf, &n, &err))
    {
        buf = replace_version (buf);
        g_free (path);
    }

    if (err != NULL)
    {
        xfce_err ("%s", err->message);
        g_error_free (err);
    }
    else
    {
        view = gtk_frame_new (NULL);
        gtk_container_set_border_width (GTK_CONTAINER (view), BORDER);
        gtk_frame_set_shadow_type (GTK_FRAME (view), GTK_SHADOW_IN);
        gtk_widget_show (view);

        sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                hscrolling ? GTK_POLICY_AUTOMATIC :
                GTK_POLICY_NEVER,
                GTK_POLICY_AUTOMATIC);
        gtk_widget_show (sw);
        gtk_container_add (GTK_CONTAINER (view), sw);

#ifdef HAVE_LIBGTKHTML
        if (usehtml)
        {
            htmldoc = html_document_new ();
            html_document_open_stream (htmldoc, "text/html");
            html_document_write_stream (htmldoc, buf, strlen (buf));
            html_document_close_stream (htmldoc);

            textview = html_view_new ();
            html_view_set_document (HTML_VIEW (textview), htmldoc);

            /* connect callbacks */
            g_signal_connect (G_OBJECT (htmldoc), "link_clicked",
                  G_CALLBACK (link_clicked), NULL);

            /* resize window */
            gtk_window_set_default_size (GTK_WINDOW (info), 615, 530);
        }
        else
        {
#endif
            textbuffer = gtk_text_buffer_new (NULL);
            gtk_text_buffer_set_text (textbuffer, buf, strlen (buf));

            textview = gtk_text_view_new_with_buffer (textbuffer);
            gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
            gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), BORDER);
            gtk_text_view_set_right_margin (GTK_TEXT_VIEW (textview), BORDER);
#ifdef HAVE_LIBGTKHTML
        }
#endif

        gtk_widget_show (textview);
        gtk_container_add (GTK_CONTAINER (sw), textview);

        gtk_notebook_append_page (notebook, view, label);

        g_free (buf);
    }
}

int
main (int argc, char **argv)
{
    GtkWidget *header;
    GtkWidget *vbox, *vbox2;
    GtkWidget *notebook;
    GtkWidget *buttonbox;
    GtkWidget *info_ok_button;
    GtkWidget *info_help_button;
    GdkPixbuf *logo_pb;
    char *text;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    gtk_init (&argc, &argv);

    info = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (info), _("About Xfce 4"));
    gtk_dialog_set_has_separator (GTK_DIALOG (info), FALSE);
    gtk_window_stick (GTK_WINDOW (info));

    logo_pb = xfce_themed_icon_load ("xfce4-logo", 48);
    gtk_window_set_icon (GTK_WINDOW (info), logo_pb);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info)->vbox), vbox2, TRUE, TRUE, 0);

    /* header with logo */
    text =
	g_strdup_printf
	("%s\n<span size=\"smaller\" style=\"italic\">%s</span>",
	 _("Xfce Desktop Environment"),
	 _("Copyright 2002-2004 by Olivier Fourdan"));
    header = xfce_create_header (logo_pb, text);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (vbox2), header, FALSE, FALSE, 0);
    g_free (text);
    g_object_unref (logo_pb);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);

    /* the notebook */
    notebook = gtk_notebook_new ();
    gtk_widget_show (notebook);
    gtk_widget_set_size_request (notebook, -1, 270);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

    /* add pages */
#ifdef VENDOR_INFO
    add_page (GTK_NOTEBOOK (notebook), VENDOR_INFO, VENDOR_INFO, FALSE);
#endif
    add_page (GTK_NOTEBOOK (notebook), _("Info"), XFCE_INFO, FALSE);
    add_page (GTK_NOTEBOOK (notebook), _("Credits"), XFCE_AUTHORS, FALSE);
    add_page (GTK_NOTEBOOK (notebook), _("Copyright"), XFCE_COPYRIGHT, TRUE);
    add_page (GTK_NOTEBOOK (notebook), _("BSDL"), XFCE_BSDL, TRUE);
    add_page (GTK_NOTEBOOK (notebook), _("LGPL"), XFCE_LGPL, TRUE);
    add_page (GTK_NOTEBOOK (notebook), _("GPL"), XFCE_GPL, TRUE);

    /* buttons */
    buttonbox = GTK_DIALOG (info)->action_area;

    info_help_button = gtk_button_new_from_stock (GTK_STOCK_HELP);
    gtk_widget_show (info_help_button);
    gtk_box_pack_start (GTK_BOX (buttonbox), info_help_button, FALSE, FALSE, 0);

    info_ok_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_widget_show (info_ok_button);
    gtk_box_pack_start (GTK_BOX (buttonbox), info_ok_button, FALSE, FALSE, 0);

    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttonbox),
					info_help_button, TRUE);

    g_signal_connect (info, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (info, "destroy-event",
		      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (info_ok_button, "clicked",
		      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (info_help_button, "clicked",
		      G_CALLBACK (info_help_cb), NULL);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (info));
    gtk_widget_show (info);

    gtk_main ();

    return (EXIT_SUCCESS);
}
