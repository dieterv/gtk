/* GAIL - The GNOME Accessibility Implementation Library
 * Copyright 2011 Red Hat, Inc.
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

#include <gtk/gtk.h>
#include "gtklinkbuttonaccessible.h"

typedef struct _GtkLinkButtonAccessibleLink GtkLinkButtonAccessibleLink;
typedef struct _GtkLinkButtonAccessibleLinkClass GtkLinkButtonAccessibleLinkClass;

struct _GtkLinkButtonAccessibleLink
{
  AtkHyperlink parent;

  GtkLinkButtonAccessible *button;
};

struct _GtkLinkButtonAccessibleLinkClass
{
  AtkHyperlinkClass parent_class;
};

static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLinkButtonAccessibleLink, gtk_link_button_accessible_link, ATK_TYPE_HYPERLINK,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static AtkHyperlink *
gtk_link_button_accessible_link_new (GtkLinkButtonAccessible *button)
{
  GtkLinkButtonAccessibleLink *link;

  link = g_object_new (gtk_link_button_accessible_link_get_type (), NULL);
  link->button = button;

  return ATK_HYPERLINK (link);
}

static gchar *
gtk_link_button_accessible_link_get_uri (AtkHyperlink *link,
                               gint          i)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)link;
  GtkWidget *widget;
  const gchar *uri;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (l->button));
  uri = gtk_link_button_get_uri (GTK_LINK_BUTTON (widget));

  return g_strdup (uri);
}

static gint
gtk_link_button_accessible_link_get_n_anchors (AtkHyperlink *link)
{
  return 1;
}

static gboolean
gtk_link_button_accessible_link_is_valid (AtkHyperlink *link)
{
  return TRUE;
}

static AtkObject *
gtk_link_button_accessible_link_get_object (AtkHyperlink *link,
                                  gint          i)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)link;

  return ATK_OBJECT (l->button);
}

static gint
gtk_link_button_accessible_link_get_start_index (AtkHyperlink *link)
{
  return 0;
}

static gint
gtk_link_button_accessible_link_get_end_index (AtkHyperlink *link)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)link;

  return atk_text_get_character_count (ATK_TEXT (l->button));
}

static void
gtk_link_button_accessible_link_init (GtkLinkButtonAccessibleLink *link)
{
}

static void
gtk_link_button_accessible_link_class_init (GtkLinkButtonAccessibleLinkClass *class)
{
  AtkHyperlinkClass *hyperlink_class = ATK_HYPERLINK_CLASS (class);

  hyperlink_class->get_uri = gtk_link_button_accessible_link_get_uri;
  hyperlink_class->get_n_anchors = gtk_link_button_accessible_link_get_n_anchors;
  hyperlink_class->is_valid = gtk_link_button_accessible_link_is_valid;
  hyperlink_class->get_object = gtk_link_button_accessible_link_get_object;
  hyperlink_class->get_start_index = gtk_link_button_accessible_link_get_start_index;
  hyperlink_class->get_end_index = gtk_link_button_accessible_link_get_end_index;
}

static gboolean
gtk_link_button_accessible_link_do_action (AtkAction *action,
                                 gint       i)
{
  GtkLinkButtonAccessibleLink *link = (GtkLinkButtonAccessibleLink *)action;
  GtkWidget *widget;

  widget = GTK_WIDGET (link->button);
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!gtk_widget_is_sensitive (widget) || !gtk_widget_get_visible (widget))
    return FALSE;

  gtk_button_clicked (GTK_BUTTON (widget));

  return TRUE;
}

static gint
gtk_link_button_accessible_link_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
gtk_link_button_accessible_link_get_name (AtkAction *action,
                                          gint       i)
{
  g_return_val_if_fail (i == 0, NULL);

  return "activate";
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = gtk_link_button_accessible_link_do_action;
  iface->get_n_actions = gtk_link_button_accessible_link_get_n_actions;
  iface->get_name = gtk_link_button_accessible_link_get_name;
}

static gboolean
activate_link (GtkLinkButton *button, AtkHyperlink *link)
{
  g_signal_emit_by_name (link, "link-activated");

  return FALSE;
}

static AtkHyperlink *
gtk_link_button_accessible_get_hyperlink (AtkHyperlinkImpl *impl)
{
  GtkLinkButtonAccessible *button = GTK_LINK_BUTTON_ACCESSIBLE (impl);

  if (!button->link)
    {
      button->link = gtk_link_button_accessible_link_new (button);
      g_signal_connect (gtk_accessible_get_widget (GTK_ACCESSIBLE (button)),
                        "activate-link", G_CALLBACK (activate_link), button->link);
    }

  return g_object_ref (button->link);
}

static void atk_hypertext_impl_interface_init (AtkHyperlinkImplIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLinkButtonAccessible, gtk_link_button_accessible, GAIL_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_HYPERLINK_IMPL, atk_hypertext_impl_interface_init))

static void
gtk_link_button_accessible_init (GtkLinkButtonAccessible *button)
{
}

static void
gtk_link_button_accessible_finalize (GObject *object)
{
  GtkLinkButtonAccessible *button = GTK_LINK_BUTTON_ACCESSIBLE (object);

  if (button->link)
    g_object_unref (button->link);

  G_OBJECT_CLASS (gtk_link_button_accessible_parent_class)->finalize (object);
}

static void
gtk_link_button_accessible_class_init (GtkLinkButtonAccessibleClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = gtk_link_button_accessible_finalize;
}

static void
atk_hypertext_impl_interface_init (AtkHyperlinkImplIface *iface)
{
  iface->get_hyperlink = gtk_link_button_accessible_get_hyperlink;
}