/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __GDK_APP_LAUNCH_CONTEXT_PRIVATE_H__
#define __GDK_APP_LAUNCH_CONTEXT_PRIVATE_H__

#include <gio/gio.h>
#include "gdkapplaunchcontext.h"
#include "gdktypes.h"

G_BEGIN_DECLS

#define GDK_APP_LAUNCH_CONTEXT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDK_TYPE_APP_LAUNCH_CONTEXT, GdkAppLaunchContextClass))
#define GDK_IS_APP_LAUNCH_CONTEXT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDK_TYPE_APP_LAUNCH_CONTEXT))
#define GDK_APP_LAUNCH_CONTEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_APP_LAUNCH_CONTEXT, GdkAppLaunchContextClass))


typedef GAppLaunchContextClass GdkAppLaunchContextClass;

struct _GdkAppLaunchContext
{
  GAppLaunchContext parent_instance;

  GdkDisplay *display;
  GdkScreen *screen;
  gint workspace;
  guint32 timestamp;
  GIcon *icon;
  char *icon_name;
};

G_END_DECLS

#endif
