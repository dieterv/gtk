/*
 * Copyright © 2010 Intel Corporation
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

#include "gdkselection.h"
#include "gdkproperty.h"
#include "gdkprivate.h"

#include <string.h>

GdkWindow *
_gdk_wayland_display_get_selection_owner (GdkDisplay *display,
					  GdkAtom     selection)
{
  return NULL;
}

gboolean
_gdk_wayland_display_set_selection_owner (GdkDisplay *display,
					  GdkWindow  *owner,
					  GdkAtom     selection,
					  guint32     time,
					  gboolean    send_event)
{
  fprintf(stderr, "set selection owner: atom %ld, owner %p\n",
	  selection, owner);

  return TRUE;
}

void
_gdk_wayland_display_send_selection_notify (GdkDisplay *dispay,
					    GdkWindow        *requestor,
					    GdkAtom          selection,
					    GdkAtom          target,
					    GdkAtom          property,
					    guint32          time)
{
}

gint
_gdk_wayland_display_get_selection_property (GdkDisplay  *display,
					     GdkWindow   *requestor,
					     guchar     **data,
					     GdkAtom     *ret_type,
					     gint        *ret_format)
{
  return 0;
}

void
_gdk_wayland_display_convert_selection (GdkDisplay *display,
					GdkWindow  *requestor,
					GdkAtom     selection,
					GdkAtom     target,
					guint32     time)
{
}

gint
_gdk_wayland_display_text_property_to_utf8_list (GdkDisplay    *display,
						 GdkAtom        encoding,
						 gint           format,
						 const guchar  *text,
						 gint           length,
						 gchar       ***list)
{
  return 0;
}

gchar *
_gdk_wayland_display_utf8_to_string_target (GdkDisplay  *display,
					    const gchar *str)
{
  return NULL;
}
