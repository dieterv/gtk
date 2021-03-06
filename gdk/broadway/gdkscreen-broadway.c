 /*
 * gdkscreen-broadway.c
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gdkscreen-broadway.h"

#include "gdkscreen.h"
#include "gdkdisplay.h"
#include "gdkdisplay-broadway.h"

#include <glib.h>

#include <stdlib.h>
#include <string.h>

static void         gdk_broadway_screen_dispose     (GObject		  *object);
static void         gdk_broadway_screen_finalize    (GObject		  *object);

G_DEFINE_TYPE (GdkBroadwayScreen, gdk_broadway_screen, GDK_TYPE_SCREEN)

static void
gdk_broadway_screen_init (GdkBroadwayScreen *screen)
{
  screen->width = 1024;
  screen->height = 768;
}

static GdkDisplay *
gdk_broadway_screen_get_display (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return GDK_BROADWAY_SCREEN (screen)->display;
}

static gint
gdk_broadway_screen_get_width (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_BROADWAY_SCREEN (screen)->width;
}

static gint
gdk_broadway_screen_get_height (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_BROADWAY_SCREEN (screen)->height;
}

static gint
gdk_broadway_screen_get_width_mm (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return gdk_screen_get_width (screen) * 25.4 / 96;
}

static gint
gdk_broadway_screen_get_height_mm (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return gdk_screen_get_height (screen) * 25.4 / 96;
}

static gint
gdk_broadway_screen_get_number (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 0;
}

static GdkWindow *
gdk_broadway_screen_get_root_window (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return GDK_BROADWAY_SCREEN (screen)->root_window;
}

void
_gdk_broadway_screen_size_changed (GdkScreen *screen, BroadwayInputScreenResizeNotify *msg)
{
  GdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (screen);
  gint width, height;

  width = gdk_screen_get_width (screen);
  height = gdk_screen_get_height (screen);

  broadway_screen->width   = msg->width;
  broadway_screen->height  = msg->height;

  if (width != gdk_screen_get_width (screen) ||
      height != gdk_screen_get_height (screen))
    g_signal_emit_by_name (screen, "size-changed");
}

static void
gdk_broadway_screen_dispose (GObject *object)
{
  GdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (object);

  if (broadway_screen->root_window)
    _gdk_window_destroy (broadway_screen->root_window, TRUE);

  G_OBJECT_CLASS (gdk_broadway_screen_parent_class)->dispose (object);
}

static void
gdk_broadway_screen_finalize (GObject *object)
{
  GdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (object);
  gint          i;

  if (broadway_screen->root_window)
    g_object_unref (broadway_screen->root_window);

  /* Visual Part */
  for (i = 0; i < broadway_screen->nvisuals; i++)
    g_object_unref (broadway_screen->visuals[i]);
  g_free (broadway_screen->visuals);

  G_OBJECT_CLASS (gdk_broadway_screen_parent_class)->finalize (object);
}

static gint
gdk_broadway_screen_get_n_monitors (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 1;
}

static gint
gdk_broadway_screen_get_primary_monitor (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 0;
}

static gint
gdk_broadway_screen_get_monitor_width_mm (GdkScreen *screen,
					  gint       monitor_num)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num == 0, -1);

  return gdk_screen_get_width_mm (screen);
}

static gint
gdk_broadway_screen_get_monitor_height_mm (GdkScreen *screen,
					   gint       monitor_num)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num == 0, -1);

  return gdk_screen_get_height_mm (screen);
}

static gchar *
gdk_broadway_screen_get_monitor_plug_name (GdkScreen *screen,
					   gint       monitor_num)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (monitor_num == 0, NULL);

  return g_strdup ("browser");
}

static void
gdk_broadway_screen_get_monitor_geometry (GdkScreen    *screen,
					  gint          monitor_num,
					  GdkRectangle *dest)
{
  GdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (screen);

  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail (monitor_num == 0);

  if (dest)
    {
      dest->x = 0;
      dest->y = 0;
      dest->width = broadway_screen->width;
      dest->height = broadway_screen->height;
    }
}

static GdkVisual *
gdk_broadway_screen_get_rgba_visual (GdkScreen *screen)
{
  GdkBroadwayScreen *broadway_screen;

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  broadway_screen = GDK_BROADWAY_SCREEN (screen);

  return broadway_screen->rgba_visual;
}

GdkScreen *
_gdk_broadway_screen_new (GdkDisplay *display,
			  gint	 screen_number)
{
  GdkScreen *screen;
  GdkBroadwayScreen *broadway_screen;

  screen = g_object_new (GDK_TYPE_BROADWAY_SCREEN, NULL);

  broadway_screen = GDK_BROADWAY_SCREEN (screen);
  broadway_screen->display = display;
  _gdk_broadway_screen_init_visuals (screen);
  _gdk_broadway_screen_init_root_window (screen);

  return screen;
}

/*
 * It is important that we first request the selection
 * notification, and then setup the initial state of
 * is_composited to avoid a race condition here.
 */
void
_gdk_broadway_screen_setup (GdkScreen *screen)
{
}

static gboolean
gdk_broadway_screen_is_composited (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  return FALSE;
}


static gchar *
gdk_broadway_screen_make_display_name (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return g_strdup ("browser");
}

static GdkWindow *
gdk_broadway_screen_get_active_window (GdkScreen *screen)
{
  return NULL;
}

static GList *
gdk_broadway_screen_get_window_stack (GdkScreen *screen)
{
  return NULL;
}

static void
gdk_broadway_screen_broadcast_client_message (GdkScreen *screen,
					      GdkEvent  *event)
{
}

static gboolean
gdk_broadway_screen_get_setting (GdkScreen   *screen,
				 const gchar *name,
				 GValue      *value)
{
  return FALSE;
}

void
_gdk_broadway_screen_events_init (GdkScreen *screen)
{
}

static void
gdk_broadway_screen_class_init (GdkBroadwayScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkScreenClass *screen_class = GDK_SCREEN_CLASS (klass);

  object_class->dispose = gdk_broadway_screen_dispose;
  object_class->finalize = gdk_broadway_screen_finalize;

  screen_class->get_display = gdk_broadway_screen_get_display;
  screen_class->get_width = gdk_broadway_screen_get_width;
  screen_class->get_height = gdk_broadway_screen_get_height;
  screen_class->get_width_mm = gdk_broadway_screen_get_width_mm;
  screen_class->get_height_mm = gdk_broadway_screen_get_height_mm;
  screen_class->get_number = gdk_broadway_screen_get_number;
  screen_class->get_root_window = gdk_broadway_screen_get_root_window;
  screen_class->get_n_monitors = gdk_broadway_screen_get_n_monitors;
  screen_class->get_primary_monitor = gdk_broadway_screen_get_primary_monitor;
  screen_class->get_monitor_width_mm = gdk_broadway_screen_get_monitor_width_mm;
  screen_class->get_monitor_height_mm = gdk_broadway_screen_get_monitor_height_mm;
  screen_class->get_monitor_plug_name = gdk_broadway_screen_get_monitor_plug_name;
  screen_class->get_monitor_geometry = gdk_broadway_screen_get_monitor_geometry;
  screen_class->is_composited = gdk_broadway_screen_is_composited;
  screen_class->make_display_name = gdk_broadway_screen_make_display_name;
  screen_class->get_active_window = gdk_broadway_screen_get_active_window;
  screen_class->get_window_stack = gdk_broadway_screen_get_window_stack;
  screen_class->broadcast_client_message = gdk_broadway_screen_broadcast_client_message;
  screen_class->get_setting = gdk_broadway_screen_get_setting;
  screen_class->get_rgba_visual = gdk_broadway_screen_get_rgba_visual;
  screen_class->get_system_visual = _gdk_broadway_screen_get_system_visual;
  screen_class->visual_get_best_depth = _gdk_broadway_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = _gdk_broadway_screen_visual_get_best_type;
  screen_class->visual_get_best = _gdk_broadway_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = _gdk_broadway_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = _gdk_broadway_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = _gdk_broadway_screen_visual_get_best_with_both;
  screen_class->query_depths = _gdk_broadway_screen_query_depths;
  screen_class->query_visual_types = _gdk_broadway_screen_query_visual_types;
  screen_class->list_visuals = _gdk_broadway_screen_list_visuals;
}

