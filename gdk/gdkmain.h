/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <gdk/gdk.h> can be included directly."
#endif

#ifndef __GDK_MAIN_H__
#define __GDK_MAIN_H__

#include <gdk/gdktypes.h>

G_BEGIN_DECLS


/* Initialization, exit and events
 */

#define GDK_PRIORITY_EVENTS (G_PRIORITY_DEFAULT)

void                  gdk_parse_args                      (gint           *argc,
                                                           gchar        ***argv);
void                  gdk_init                            (gint           *argc,
                                                           gchar        ***argv);
gboolean              gdk_init_check                      (gint           *argc,
                                                           gchar        ***argv);
void                  gdk_add_option_entries_libgtk_only  (GOptionGroup   *group);
void                  gdk_pre_parse_libgtk_only           (void);

/**
 * gdk_set_locale:
 *
 * Initializes the support for internationalization by calling the <function>setlocale()</function>
 * system call. This function is called by gtk_set_locale() and so GTK+
 * applications should use that instead.
 *
 * The locale to use is determined by the <envar>LANG</envar> environment variable,
 * so to run an application in a certain locale you can do something like this:
 * <informalexample>
 * <programlisting>
 *   export LANG="fr"
 *   ... run application ...
 * </programlisting>
 * </informalexample>
 *
 * If the locale is not supported by X then it is reset to the standard "C"
 * locale.
 *
 * Returns: the resulting locale.
 */
gchar*                gdk_set_locale                      (void);
void                  gdk_enable_multidevice              (void);

G_CONST_RETURN gchar *gdk_get_program_class               (void);
void                  gdk_set_program_class               (const gchar    *program_class);

void                  gdk_notify_startup_complete         (void);
void                  gdk_notify_startup_complete_with_id (const gchar* startup_id);

/* Push and pop error handlers for X errors
 */
void                           gdk_error_trap_push        (void);
/* warn unused because you could use pop_ignored otherwise */
G_GNUC_WARN_UNUSED_RESULT gint gdk_error_trap_pop         (void);
void                           gdk_error_trap_pop_ignored (void);


G_CONST_RETURN gchar *gdk_get_display_arg_name (void);

/**
 * gdk_get_display:
 *
 * Gets the name of the display, which usually comes from the <envar>DISPLAY</envar>
 * environment variable or the <option>--display</option> command line option.
 *
 * Returns: the name of the display.
 */
gchar*	              gdk_get_display          (void);

#ifndef GDK_MULTIDEVICE_SAFE
GdkGrabStatus gdk_pointer_grab       (GdkWindow    *window,
				      gboolean      owner_events,
				      GdkEventMask  event_mask,
				      GdkWindow    *confine_to,
				      GdkCursor    *cursor,
				      guint32       time_);
GdkGrabStatus gdk_keyboard_grab      (GdkWindow    *window,
				      gboolean      owner_events,
				      guint32       time_);
#endif /* GDK_MULTIDEVICE_SAFE */

#ifndef GDK_MULTIHEAD_SAFE

#ifndef GDK_MULTIDEVICE_SAFE
void          gdk_pointer_ungrab     (guint32       time_);
void          gdk_keyboard_ungrab    (guint32       time_);
gboolean      gdk_pointer_is_grabbed (void);
#endif /* GDK_MULTIDEVICE_SAFE */

gint gdk_screen_width  (void) G_GNUC_CONST;
gint gdk_screen_height (void) G_GNUC_CONST;

gint gdk_screen_width_mm  (void) G_GNUC_CONST;
gint gdk_screen_height_mm (void) G_GNUC_CONST;

void gdk_set_double_click_time (guint msec);

void gdk_beep (void);

#endif /* GDK_MULTIHEAD_SAFE */

/**
 * gdk_flush:
 *
 * Flushes the X output buffer and waits until all requests have been processed
 * by the server. This is rarely needed by applications. It's main use is for
 * trapping X errors with gdk_error_trap_push() and gdk_error_trap_pop().
 */
void gdk_flush (void);

G_END_DECLS

#endif /* __GDK_MAIN_H__ */