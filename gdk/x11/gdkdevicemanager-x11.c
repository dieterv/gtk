/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#include "config.h"

#include "gdkx11devicemanager-core.h"
#include "gdkdevicemanagerprivate-core.h"
#ifdef XINPUT_XFREE
#include "gdkx11devicemanager-xi.h"
#ifdef XINPUT_2
#include "gdkx11devicemanager-xi2.h"
#endif
#endif
#include "gdkinternals.h"
#include "gdkprivate-x11.h"

/* Defines for VCP/VCK, to be used too
 * for the core protocol device manager
 */
#define VIRTUAL_CORE_POINTER_ID 2
#define VIRTUAL_CORE_KEYBOARD_ID 3

GdkDeviceManager *
_gdk_x11_device_manager_new (GdkDisplay *display)
{
  if (!g_getenv ("GDK_CORE_DEVICE_EVENTS"))
    {
#if defined (XINPUT_2) || defined (XINPUT_XFREE)
      int opcode, firstevent, firsterror;
      Display *xdisplay;

      xdisplay = GDK_DISPLAY_XDISPLAY (display);

      if (XQueryExtension (xdisplay, "XInputExtension",
                           &opcode, &firstevent, &firsterror))
        {
#ifdef XINPUT_2
          int major, minor;

          major = 2;
          minor = 0;

          if (!_gdk_disable_multidevice &&
              XIQueryVersion (xdisplay, &major, &minor) != BadRequest)
            {
              GdkX11DeviceManagerXI2 *device_manager_xi2;

              GDK_NOTE (INPUT, g_print ("Creating XI2 device manager\n"));

              device_manager_xi2 = g_object_new (GDK_TYPE_X11_DEVICE_MANAGER_XI2,
                                                 "display", display,
                                                 "opcode", opcode,
                                                 NULL);

              return GDK_DEVICE_MANAGER (device_manager_xi2);
            }
          else
#endif /* XINPUT_2 */
            {
              GDK_NOTE (INPUT, g_print ("Creating XI device manager\n"));

              return g_object_new (GDK_TYPE_X11_DEVICE_MANAGER_XI,
                                   "display", display,
                                   "event-base", firstevent,
                                   NULL);
            }
        }
#endif /* XINPUT_2 || XINPUT_XFREE */
    }

  GDK_NOTE (INPUT, g_print ("Creating core device manager\n"));

  return g_object_new (GDK_TYPE_X11_DEVICE_MANAGER_CORE,
                       "display", display,
                       NULL);
}

/**
 * gdk_x11_device_manager_lookup:
 * @device_manager: a #GdkDeviceManager
 * @device_id: a device ID, as understood by the XInput2 protocol
 *
 * Returns the #GdkDevice that wraps the given device ID.
 *
 * Returns: (transfer none): (allow-none): The #GdkDevice wrapping the device ID,
 *          or %NULL if the given ID doesn't currently represent a device.
 **/
GdkDevice *
gdk_x11_device_manager_lookup (GdkDeviceManager *device_manager,
			       gint              device_id)
{
  GdkDevice *device = NULL;

  g_return_val_if_fail (GDK_IS_DEVICE_MANAGER (device_manager), NULL);

#ifdef XINPUT_2
  if (GDK_IS_X11_DEVICE_MANAGER_XI2 (device_manager))
    device = _gdk_x11_device_manager_xi2_lookup (GDK_X11_DEVICE_MANAGER_XI2 (device_manager),
                                                 device_id);
  else
#endif /* XINPUT_2 */
    if (GDK_IS_X11_DEVICE_MANAGER_CORE (device_manager))
      {
        /* It is a core/xi1 device manager, we only map
         * IDs 2 and 3, matching XI2's Virtual Core Pointer
         * and Keyboard.
         */
        if (device_id == VIRTUAL_CORE_POINTER_ID)
          device = GDK_X11_DEVICE_MANAGER_CORE (device_manager)->core_pointer;
        else if (device_id == VIRTUAL_CORE_KEYBOARD_ID)
          device = GDK_X11_DEVICE_MANAGER_CORE (device_manager)->core_keyboard;
      }

  return device;
}

/**
 * gdk_x11_device_get_id:
 * @device: a #GdkDevice
 *
 * Returns the device ID as seen by XInput2.
 *
 * <note>
 *   If gdk_disable_multidevice() has been called, this function
 *   will respectively return 2/3 for the core pointer and keyboard,
 *   (matching the IDs for the Virtual Core Pointer and Keyboard in
 *   XInput 2), but calling this function on any slave devices (i.e.
 *   those managed via XInput 1.x), will return 0.
 * </note>
 *
 * Returns: the XInput2 device ID.
 **/
gint
gdk_x11_device_get_id (GdkDevice *device)
{
  gint device_id = 0;

  g_return_val_if_fail (GDK_IS_DEVICE (device), 0);

#ifdef XINPUT_2
  if (GDK_IS_X11_DEVICE_XI2 (device))
    device_id = _gdk_x11_device_xi2_get_id (GDK_X11_DEVICE_XI2 (device));
  else
#endif /* XINPUT_2 */
    if (GDK_IS_X11_DEVICE_CORE (device))
      {
        if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
          device_id = VIRTUAL_CORE_KEYBOARD_ID;
        else
          device_id = VIRTUAL_CORE_POINTER_ID;
      }

  return device_id;
}
