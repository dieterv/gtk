/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "gtkstylepropertyprivate.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo-gobject.h>

#include "gtkcssprovider.h"
#include "gtkcssparserprivate.h"
#include "gtkcsstypesprivate.h"

/* the actual parsers we have */
#include "gtkanimationdescription.h"
#include "gtkbindings.h"
#include "gtk9slice.h"
#include "gtkgradient.h"
#include "gtkshadowprivate.h"
#include "gtkthemingengine.h"
#include "gtktypebuiltins.h"

static GHashTable *parse_funcs = NULL;
static GHashTable *print_funcs = NULL;
static GHashTable *properties = NULL;

static void
register_conversion_function (GType             type,
                              GtkStyleParseFunc parse,
                              GtkStylePrintFunc print)
{
  if (parse)
    g_hash_table_insert (parse_funcs, GSIZE_TO_POINTER (type), parse);
  if (print)
    g_hash_table_insert (print_funcs, GSIZE_TO_POINTER (type), print);
}

/*** IMPLEMENTATIONS ***/

static gboolean 
rgba_value_parse (GtkCssParser *parser,
                  GFile        *base,
                  GValue       *value)
{
  GtkSymbolicColor *symbolic;
  GdkRGBA rgba;

  symbolic = _gtk_css_parser_read_symbolic_color (parser);
  if (symbolic == NULL)
    return FALSE;

  if (gtk_symbolic_color_resolve (symbolic, NULL, &rgba))
    {
      g_value_set_boxed (value, &rgba);
      gtk_symbolic_color_unref (symbolic);
    }
  else
    {
      g_value_unset (value);
      g_value_init (value, GTK_TYPE_SYMBOLIC_COLOR);
      g_value_take_boxed (value, symbolic);
    }

  return TRUE;
}

static void
rgba_value_print (const GValue *value,
                  GString      *string)
{
  const GdkRGBA *rgba = g_value_get_boxed (value);

  if (rgba == NULL)
    g_string_append (string, "none");
  else
    {
      char *s = gdk_rgba_to_string (rgba);
      g_string_append (string, s);
      g_free (s);
    }
}

static gboolean 
color_value_parse (GtkCssParser *parser,
                   GFile        *base,
                   GValue       *value)
{
  GtkSymbolicColor *symbolic;
  GdkRGBA rgba;

  symbolic = _gtk_css_parser_read_symbolic_color (parser);
  if (symbolic == NULL)
    return FALSE;

  if (gtk_symbolic_color_resolve (symbolic, NULL, &rgba))
    {
      GdkColor color;

      color.red = rgba.red * 65535. + 0.5;
      color.green = rgba.green * 65535. + 0.5;
      color.blue = rgba.blue * 65535. + 0.5;

      g_value_set_boxed (value, &color);
    }
  else
    {
      g_value_unset (value);
      g_value_init (value, GTK_TYPE_SYMBOLIC_COLOR);
      g_value_take_boxed (value, symbolic);
    }

  return TRUE;
}

static void
color_value_print (const GValue *value,
                   GString      *string)
{
  const GdkColor *color = g_value_get_boxed (value);

  if (color == NULL)
    g_string_append (string, "none");
  else
    {
      char *s = gdk_color_to_string (color);
      g_string_append (string, s);
      g_free (s);
    }
}

static gboolean
symbolic_color_value_parse (GtkCssParser *parser,
                            GFile        *base,
                            GValue       *value)
{
  GtkSymbolicColor *symbolic;

  symbolic = _gtk_css_parser_read_symbolic_color (parser);
  if (symbolic == NULL)
    return FALSE;

  g_value_take_boxed (value, symbolic);
  return TRUE;
}

static void
symbolic_color_value_print (const GValue *value,
                            GString      *string)
{
  GtkSymbolicColor *symbolic = g_value_get_boxed (value);

  if (symbolic == NULL)
    g_string_append (string, "none");
  else
    {
      char *s = gtk_symbolic_color_to_string (symbolic);
      g_string_append (string, s);
      g_free (s);
    }
}

static gboolean 
font_description_value_parse (GtkCssParser *parser,
                              GFile        *base,
                              GValue       *value)
{
  PangoFontDescription *font_desc;
  char *str;

  str = _gtk_css_parser_read_value (parser);
  if (str == NULL)
    return FALSE;

  font_desc = pango_font_description_from_string (str);
  g_free (str);
  g_value_take_boxed (value, font_desc);
  return TRUE;
}

static void
font_description_value_print (const GValue *value,
                              GString      *string)
{
  const PangoFontDescription *desc = g_value_get_boxed (value);

  if (desc == NULL)
    g_string_append (string, "none");
  else
    {
      char *s = pango_font_description_to_string (desc);
      g_string_append (string, s);
      g_free (s);
    }
}

static gboolean 
boolean_value_parse (GtkCssParser *parser,
                     GFile        *base,
                     GValue       *value)
{
  if (_gtk_css_parser_try (parser, "true", TRUE) ||
      _gtk_css_parser_try (parser, "1", TRUE))
    {
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (_gtk_css_parser_try (parser, "false", TRUE) ||
           _gtk_css_parser_try (parser, "0", TRUE))
    {
      g_value_set_boolean (value, FALSE);
      return TRUE;
    }
  else
    {
      _gtk_css_parser_error (parser, "Expected a boolean value");
      return FALSE;
    }
}

static void
boolean_value_print (const GValue *value,
                     GString      *string)
{
  if (g_value_get_boolean (value))
    g_string_append (string, "true");
  else
    g_string_append (string, "false");
}

static gboolean 
int_value_parse (GtkCssParser *parser,
                 GFile        *base,
                 GValue       *value)
{
  gint i;

  if (!_gtk_css_parser_try_int (parser, &i))
    {
      _gtk_css_parser_error (parser, "Expected a valid integer value");
      return FALSE;
    }

  g_value_set_int (value, i);
  return TRUE;
}

static void
int_value_print (const GValue *value,
                 GString      *string)
{
  g_string_append_printf (string, "%d", g_value_get_int (value));
}

static gboolean 
uint_value_parse (GtkCssParser *parser,
                  GFile        *base,
                  GValue       *value)
{
  guint u;

  if (!_gtk_css_parser_try_uint (parser, &u))
    {
      _gtk_css_parser_error (parser, "Expected a valid unsigned value");
      return FALSE;
    }

  g_value_set_uint (value, u);
  return TRUE;
}

static void
uint_value_print (const GValue *value,
                  GString      *string)
{
  g_string_append_printf (string, "%u", g_value_get_uint (value));
}

static gboolean 
double_value_parse (GtkCssParser *parser,
                    GFile        *base,
                    GValue       *value)
{
  gdouble d;

  if (!_gtk_css_parser_try_double (parser, &d))
    {
      _gtk_css_parser_error (parser, "Expected a number");
      return FALSE;
    }

  g_value_set_double (value, d);
  return TRUE;
}

static void
string_append_double (GString *string,
                      double   d)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_dtostr (buf, sizeof (buf), d);
  g_string_append (string, buf);
}

static void
double_value_print (const GValue *value,
                    GString      *string)
{
  string_append_double (string, g_value_get_double (value));
}

static gboolean 
float_value_parse (GtkCssParser *parser,
                   GFile        *base,
                   GValue       *value)
{
  gdouble d;

  if (!_gtk_css_parser_try_double (parser, &d))
    {
      _gtk_css_parser_error (parser, "Expected a number");
      return FALSE;
    }

  g_value_set_float (value, d);
  return TRUE;
}

static void
float_value_print (const GValue *value,
                   GString      *string)
{
  string_append_double (string, g_value_get_float (value));
}

static gboolean 
string_value_parse (GtkCssParser *parser,
                    GFile        *base,
                    GValue       *value)
{
  char *str = _gtk_css_parser_read_string (parser);

  if (str == NULL)
    return FALSE;

  g_value_take_string (value, str);
  return TRUE;
}

static void
string_value_print (const GValue *value,
                    GString      *str)
{
  const char *string;
  gsize len;

  string = g_value_get_string (value);
  g_string_append_c (str, '"');

  do {
    len = strcspn (string, "\"\n\r\f");
    g_string_append (str, string);
    string += len;
    switch (*string)
      {
      case '\0':
        break;
      case '\n':
        g_string_append (str, "\\A ");
        break;
      case '\r':
        g_string_append (str, "\\D ");
        break;
      case '\f':
        g_string_append (str, "\\C ");
        break;
      case '\"':
        g_string_append (str, "\\\"");
        break;
      default:
        g_assert_not_reached ();
        break;
      }
  } while (*string);

  g_string_append_c (str, '"');
}

static gboolean 
theming_engine_value_parse (GtkCssParser *parser,
                            GFile        *base,
                            GValue       *value)
{
  GtkThemingEngine *engine;
  char *str;

  str = _gtk_css_parser_try_ident (parser, TRUE);
  if (str == NULL)
    {
      _gtk_css_parser_error (parser, "Expected a valid theme name");
      return FALSE;
    }

  engine = gtk_theming_engine_load (str);
  if (engine == NULL)
    {
      _gtk_css_parser_error (parser, "Themeing engine '%s' not found", str);
      g_free (str);
      return FALSE;
    }

  g_value_set_object (value, engine);
  g_free (str);
  return TRUE;
}

static void
theming_engine_value_print (const GValue *value,
                            GString      *string)
{
  GtkThemingEngine *engine;
  char *name;

  engine = g_value_get_object (value);
  if (engine == NULL)
    g_string_append (string, "none");
  else
    {
      /* XXX: gtk_theming_engine_get_name()? */
      g_object_get (engine, "name", &name, NULL);
      g_string_append (string, name);
      g_free (name);
    }
}

static gboolean 
animation_description_value_parse (GtkCssParser *parser,
                                   GFile        *base,
                                   GValue       *value)
{
  GtkAnimationDescription *desc;
  char *str;

  str = _gtk_css_parser_read_value (parser);
  if (str == NULL)
    return FALSE;

  desc = _gtk_animation_description_from_string (str);
  g_free (str);

  if (desc == NULL)
    {
      _gtk_css_parser_error (parser, "Invalid animation description");
      return FALSE;
    }
  
  g_value_take_boxed (value, desc);
  return TRUE;
}

static void
animation_description_value_print (const GValue *value,
                                   GString      *string)
{
  GtkAnimationDescription *desc = g_value_get_boxed (value);

  if (desc == NULL)
    g_string_append (string, "none");
  else
    _gtk_animation_description_print (desc, string);
}

static gboolean 
border_value_parse (GtkCssParser *parser,
                    GFile        *base,
                    GValue       *value)
{
  GtkBorder border = { 0, };
  guint i, numbers[4];

  for (i = 0; i < G_N_ELEMENTS (numbers); i++)
    {
      if (!_gtk_css_parser_try_uint (parser, &numbers[i]))
        break;

      /* XXX: shouldn't allow spaces here? */
      _gtk_css_parser_try (parser, "px", TRUE);
    }

  if (i == 0)
    {
      _gtk_css_parser_error (parser, "Expected valid border");
      return FALSE;
    }

  border.top = numbers[0];
  if (i > 1)
    border.right = numbers[1];
  else
    border.right = border.top;
  if (i > 2)
    border.bottom = numbers[2];
  else
    border.bottom = border.top;
  if (i > 3)
    border.left = numbers[3];
  else
    border.left = border.right;

  g_value_set_boxed (value, &border);
  return TRUE;
}

static void
border_value_print (const GValue *value, GString *string)
{
  const GtkBorder *border = g_value_get_boxed (value);

  if (border == NULL)
    g_string_append (string, "none");
  else if (border->left != border->right)
    g_string_append_printf (string, "%d %d %d %d", border->top, border->right, border->bottom, border->left);
  else if (border->top != border->bottom)
    g_string_append_printf (string, "%d %d %d", border->top, border->right, border->bottom);
  else if (border->top != border->left)
    g_string_append_printf (string, "%d %d", border->top, border->right);
  else
    g_string_append_printf (string, "%d", border->top);
}

static gboolean 
gradient_value_parse (GtkCssParser *parser,
                      GFile        *base,
                      GValue       *value)
{
  GtkGradient *gradient;
  cairo_pattern_type_t type;
  gdouble coords[6];
  guint i;

  if (!_gtk_css_parser_try (parser, "-gtk-gradient", TRUE))
    {
      _gtk_css_parser_error (parser,
                             "Expected '-gtk-gradient'");
      return FALSE;
    }

  if (!_gtk_css_parser_try (parser, "(", TRUE))
    {
      _gtk_css_parser_error (parser,
                             "Expected '(' after '-gtk-gradient'");
      return FALSE;
    }

  /* Parse gradient type */
  if (_gtk_css_parser_try (parser, "linear", TRUE))
    type = CAIRO_PATTERN_TYPE_LINEAR;
  else if (_gtk_css_parser_try (parser, "radial", TRUE))
    type = CAIRO_PATTERN_TYPE_RADIAL;
  else
    {
      _gtk_css_parser_error (parser,
                             "Gradient type must be 'radial' or 'linear'");
      return FALSE;
    }

  /* Parse start/stop position parameters */
  for (i = 0; i < 2; i++)
    {
      if (! _gtk_css_parser_try (parser, ",", TRUE))
        {
          _gtk_css_parser_error (parser,
                                 "Expected ','");
          return FALSE;
        }

      if (_gtk_css_parser_try (parser, "left", TRUE))
        coords[i * 3] = 0;
      else if (_gtk_css_parser_try (parser, "right", TRUE))
        coords[i * 3] = 1;
      else if (_gtk_css_parser_try (parser, "center", TRUE))
        coords[i * 3] = 0.5;
      else if (!_gtk_css_parser_try_double (parser, &coords[i * 3]))
        {
          _gtk_css_parser_error (parser,
                                 "Expected a valid X coordinate");
          return FALSE;
        }

      if (_gtk_css_parser_try (parser, "top", TRUE))
        coords[i * 3 + 1] = 0;
      else if (_gtk_css_parser_try (parser, "bottom", TRUE))
        coords[i * 3 + 1] = 1;
      else if (_gtk_css_parser_try (parser, "center", TRUE))
        coords[i * 3 + 1] = 0.5;
      else if (!_gtk_css_parser_try_double (parser, &coords[i * 3 + 1]))
        {
          _gtk_css_parser_error (parser,
                                 "Expected a valid Y coordinate");
          return FALSE;
        }

      if (type == CAIRO_PATTERN_TYPE_RADIAL)
        {
          /* Parse radius */
          if (! _gtk_css_parser_try (parser, ",", TRUE))
            {
              _gtk_css_parser_error (parser,
                                     "Expected ','");
              return FALSE;
            }

          if (! _gtk_css_parser_try_double (parser, &coords[(i * 3) + 2]))
            {
              _gtk_css_parser_error (parser,
                                     "Expected a numer for the radius");
              return FALSE;
            }
        }
    }

  if (type == CAIRO_PATTERN_TYPE_LINEAR)
    gradient = gtk_gradient_new_linear (coords[0], coords[1], coords[3], coords[4]);
  else
    gradient = gtk_gradient_new_radial (coords[0], coords[1], coords[2],
                                        coords[3], coords[4], coords[5]);

  while (_gtk_css_parser_try (parser, ",", TRUE))
    {
      GtkSymbolicColor *color;
      gdouble position;

      if (_gtk_css_parser_try (parser, "from", TRUE))
        {
          position = 0;

          if (!_gtk_css_parser_try (parser, "(", TRUE))
            {
              gtk_gradient_unref (gradient);
              _gtk_css_parser_error (parser,
                                     "Expected '('");
              return FALSE;
            }

        }
      else if (_gtk_css_parser_try (parser, "to", TRUE))
        {
          position = 1;

          if (!_gtk_css_parser_try (parser, "(", TRUE))
            {
              gtk_gradient_unref (gradient);
              _gtk_css_parser_error (parser,
                                     "Expected '('");
              return FALSE;
            }

        }
      else if (_gtk_css_parser_try (parser, "color-stop", TRUE))
        {
          if (!_gtk_css_parser_try (parser, "(", TRUE))
            {
              gtk_gradient_unref (gradient);
              _gtk_css_parser_error (parser,
                                     "Expected '('");
              return FALSE;
            }

          if (!_gtk_css_parser_try_double (parser, &position))
            {
              gtk_gradient_unref (gradient);
              _gtk_css_parser_error (parser,
                                     "Expected a valid number");
              return FALSE;
            }

          if (!_gtk_css_parser_try (parser, ",", TRUE))
            {
              gtk_gradient_unref (gradient);
              _gtk_css_parser_error (parser,
                                     "Expected a comma");
              return FALSE;
            }
        }
      else
        {
          gtk_gradient_unref (gradient);
          _gtk_css_parser_error (parser,
                                 "Not a valid color-stop definition");
          return FALSE;
        }

      color = _gtk_css_parser_read_symbolic_color (parser);
      if (color == NULL)
        {
          gtk_gradient_unref (gradient);
          return FALSE;
        }

      gtk_gradient_add_color_stop (gradient, position, color);
      gtk_symbolic_color_unref (color);

      if (!_gtk_css_parser_try (parser, ")", TRUE))
        {
          gtk_gradient_unref (gradient);
          _gtk_css_parser_error (parser,
                                 "Expected ')'");
          return FALSE;
        }
    }

  if (!_gtk_css_parser_try (parser, ")", TRUE))
    {
      gtk_gradient_unref (gradient);
      _gtk_css_parser_error (parser,
                             "Expected ')'");
      return FALSE;
    }

  g_value_take_boxed (value, gradient);
  return TRUE;
}

static void
gradient_value_print (const GValue *value,
                      GString      *string)
{
  GtkGradient *gradient = g_value_get_boxed (value);

  if (gradient == NULL)
    g_string_append (string, "none");
  else
    {
      char *s = gtk_gradient_to_string (gradient);
      g_string_append (string, s);
      g_free (s);
    }
}

static GFile *
gtk_css_parse_url (GtkCssParser *parser,
                   GFile        *base)
{
  gchar *path;
  GFile *file;

  if (_gtk_css_parser_try (parser, "url", FALSE))
    {
      if (!_gtk_css_parser_try (parser, "(", TRUE))
        {
          _gtk_css_parser_skip_whitespace (parser);
          if (_gtk_css_parser_try (parser, "(", TRUE))
            {
              GError *error;
              
              error = g_error_new_literal (GTK_CSS_PROVIDER_ERROR,
                                           GTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                           "Whitespace between 'url' and '(' is not allowed");
                             
              _gtk_css_parser_take_error (parser, error);
            }
          else
            {
              _gtk_css_parser_error (parser, "Expected '(' after 'url'");
              return NULL;
            }
        }

      path = _gtk_css_parser_read_string (parser);
      if (path == NULL)
        return NULL;

      if (!_gtk_css_parser_try (parser, ")", TRUE))
        {
          _gtk_css_parser_error (parser, "No closing ')' found for 'url'");
          g_free (path);
          return NULL;
        }
    }
  else
    {
      path = _gtk_css_parser_try_name (parser, TRUE);
      if (path == NULL)
        {
          _gtk_css_parser_error (parser, "Not a valid url");
          return NULL;
        }
    }

  file = g_file_resolve_relative_path (base, path);
  g_free (path);

  return file;
}

static gboolean 
pattern_value_parse (GtkCssParser *parser,
                     GFile        *base,
                     GValue       *value)
{
  if (_gtk_css_parser_begins_with (parser, '-'))
    {
      g_value_unset (value);
      g_value_init (value, GTK_TYPE_GRADIENT);
      return gradient_value_parse (parser, base, value);
    }
  else
    {
      GError *error = NULL;
      gchar *path;
      GdkPixbuf *pixbuf;
      GFile *file;
      cairo_surface_t *surface;
      cairo_pattern_t *pattern;
      cairo_t *cr;
      cairo_matrix_t matrix;

      file = gtk_css_parse_url (parser, base);
      if (file == NULL)
        return FALSE;

      path = g_file_get_path (file);
      g_object_unref (file);

      pixbuf = gdk_pixbuf_new_from_file (path, &error);
      g_free (path);
      if (pixbuf == NULL)
        {
          _gtk_css_parser_take_error (parser, error);
          return FALSE;
        }

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            gdk_pixbuf_get_width (pixbuf),
                                            gdk_pixbuf_get_height (pixbuf));
      cr = cairo_create (surface);
      gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
      cairo_paint (cr);
      pattern = cairo_pattern_create_for_surface (surface);

      cairo_matrix_init_scale (&matrix,
                               gdk_pixbuf_get_width (pixbuf),
                               gdk_pixbuf_get_height (pixbuf));
      cairo_pattern_set_matrix (pattern, &matrix);

      cairo_surface_destroy (surface);
      cairo_destroy (cr);
      g_object_unref (pixbuf);

      g_value_take_boxed (value, pattern);
    }
  
  return TRUE;
}

static gboolean
shadow_value_parse (GtkCssParser *parser,
                    GFile *base,
                    GValue *value)
{
  gboolean inset;
  gdouble hoffset, voffset, blur, spread;
  GtkSymbolicColor *color;
  GtkShadow *shadow;

  shadow = _gtk_shadow_new ();

  do
    {
      inset = _gtk_css_parser_try (parser, "inset", TRUE);

      if (!_gtk_css_parser_try_double (parser, &hoffset) ||
          !_gtk_css_parser_try_double (parser, &voffset))
        {
          _gtk_css_parser_error (parser, "Horizontal and vertical offsets are required");
          _gtk_shadow_unref (shadow);
          return FALSE;
        }

      if (!_gtk_css_parser_try_double (parser, &blur))
        blur = 0;

      if (!_gtk_css_parser_try_double (parser, &spread))
        spread = 0;

      /* XXX: the color is optional and UA-defined if it's missing,
       * but it doesn't really make sense for us...
       */
      color = _gtk_css_parser_read_symbolic_color (parser);

      if (color == NULL)
        {
          _gtk_shadow_unref (shadow);
          return FALSE;
        }

      _gtk_shadow_append (shadow,
                          hoffset, voffset,
                          blur, spread,
                          inset, color);

      gtk_symbolic_color_unref (color);

    }
  while (_gtk_css_parser_try (parser, ",", TRUE));

  g_value_take_boxed (value, shadow);
  return TRUE;
}

static void
shadow_value_print (const GValue *value,
                    GString      *string)
{
  GtkShadow *shadow;

  shadow = g_value_get_boxed (value);

  if (shadow == NULL)
    g_string_append (string, "none");
  else
    _gtk_shadow_print (shadow, string);
}

static gboolean 
slice_value_parse (GtkCssParser *parser,
                   GFile        *base,
                   GValue       *value)
{
  gdouble distance_top, distance_bottom;
  gdouble distance_left, distance_right;
  GtkSliceSideModifier mods[2];
  GdkPixbuf *pixbuf;
  Gtk9Slice *slice;
  GFile *file;
  GError *error = NULL;
  gint i;
  char *path;

  /* Parse image url */
  file = gtk_css_parse_url (parser, base);
  if (!file)
      return FALSE;

  if (!_gtk_css_parser_try_double (parser, &distance_top) ||
      !_gtk_css_parser_try_double (parser, &distance_right) ||
      !_gtk_css_parser_try_double (parser, &distance_bottom) ||
      !_gtk_css_parser_try_double (parser, &distance_left))
    {
      _gtk_css_parser_error (parser, "Expected a number");
      g_object_unref (file);
      return FALSE;
    }

  for (i = 0; i < 2; i++)
    {
      if (_gtk_css_parser_try (parser, "stretch", TRUE))
        mods[i] = GTK_SLICE_STRETCH;
      else if (_gtk_css_parser_try (parser, "repeat", TRUE))
        mods[i] = GTK_SLICE_REPEAT;
      else if (i == 0)
        {
          mods[1] = mods[0] = GTK_SLICE_STRETCH;
          break;
        }
      else
        mods[i] = mods[0];
    }

  path = g_file_get_path (file);
  g_object_unref (file);
  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  g_free (path);
  if (!pixbuf)
    {
      _gtk_css_parser_take_error (parser, error);
      return FALSE;
    }

  slice = _gtk_9slice_new (pixbuf,
                           distance_top, distance_bottom,
                           distance_left, distance_right,
                           mods[0], mods[1]);
  g_object_unref (pixbuf);

  g_value_take_boxed (value, slice);
  return TRUE;
}

static gboolean 
enum_value_parse (GtkCssParser *parser,
                  GFile        *base,
                  GValue       *value)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  char *str;

  str = _gtk_css_parser_try_ident (parser, TRUE);
  if (str == NULL)
    {
      _gtk_css_parser_error (parser, "Expected an identifier");
      return FALSE;
    }

  enum_class = g_type_class_ref (G_VALUE_TYPE (value));
  enum_value = g_enum_get_value_by_nick (enum_class, str);

  if (enum_value)
    g_value_set_enum (value, enum_value->value);
  else
    _gtk_css_parser_error (parser,
                           "Unknown value '%s' for enum type '%s'",
                           str, g_type_name (G_VALUE_TYPE (value)));
  
  g_type_class_unref (enum_class);
  g_free (str);

  return enum_value != NULL;
}

static void
enum_value_print (const GValue *value,
                  GString      *string)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (G_VALUE_TYPE (value));
  enum_value = g_enum_get_value (enum_class, g_value_get_enum (value));

  g_string_append (string, enum_value->value_nick);

  g_type_class_unref (enum_class);
}

static gboolean 
flags_value_parse (GtkCssParser *parser,
                   GFile        *base,
                   GValue       *value)
{
  GFlagsClass *flags_class;
  GFlagsValue *flag_value;
  guint flags = 0;
  char *str;

  flags_class = g_type_class_ref (G_VALUE_TYPE (value));

  do {
    str = _gtk_css_parser_try_ident (parser, TRUE);
    if (str == NULL)
      {
        _gtk_css_parser_error (parser, "Expected an identifier");
        g_type_class_unref (flags_class);
        return FALSE;
      }

      flag_value = g_flags_get_value_by_nick (flags_class, str);
      if (!flag_value)
        {
          _gtk_css_parser_error (parser,
                                 "Unknown flag value '%s' for type '%s'",
                                 str, g_type_name (G_VALUE_TYPE (value)));
          /* XXX Do we want to return FALSE here? We can get
           * forward-compatibility for new values this way
           */
          g_free (str);
          g_type_class_unref (flags_class);
          return FALSE;
        }

      g_free (str);
    }
  while (_gtk_css_parser_try (parser, ",", FALSE));

  g_type_class_unref (flags_class);

  g_value_set_enum (value, flags);

  return TRUE;
}

static void
flags_value_print (const GValue *value,
                   GString      *string)
{
  GFlagsClass *flags_class;
  guint i, flags;

  flags_class = g_type_class_ref (G_VALUE_TYPE (value));
  flags = g_value_get_flags (value);

  for (i = 0; i < flags_class->n_values; i++)
    {
      GFlagsValue *flags_value = &flags_class->values[i];

      if (flags & flags_value->value)
        {
          if (string->len != 0)
            g_string_append (string, ", ");

          g_string_append (string, flags_value->value_nick);
        }
    }

  g_type_class_unref (flags_class);
}

static gboolean 
bindings_value_parse (GtkCssParser *parser,
                      GFile        *base,
                      GValue       *value)
{
  GPtrArray *array;
  GtkBindingSet *binding_set;
  char *name;

  array = g_ptr_array_new ();

  do {
      name = _gtk_css_parser_try_ident (parser, TRUE);
      if (name == NULL)
        {
          _gtk_css_parser_error (parser, "Not a valid binding name");
          g_ptr_array_free (array, TRUE);
          return FALSE;
        }

      binding_set = gtk_binding_set_find (name);

      if (!binding_set)
        {
          _gtk_css_parser_error (parser, "No binding set named '%s'", name);
          g_free (name);
          continue;
        }

      g_ptr_array_add (array, binding_set);
      g_free (name);
    }
  while (_gtk_css_parser_try (parser, ",", TRUE));

  g_value_take_boxed (value, array);

  return TRUE;
}

static void
bindings_value_print (const GValue *value,
                      GString      *string)
{
  GPtrArray *array;
  guint i;

  array = g_value_get_boxed (value);

  for (i = 0; i < array->len; i++)
    {
      GtkBindingSet *binding_set = g_ptr_array_index (array, i);

      if (i > 0)
        g_string_append (string, ", ");
      g_string_append (string, binding_set->set_name);
    }
}

static gboolean 
border_corner_radius_value_parse (GtkCssParser *parser,
                                  GFile        *base,
                                  GValue       *value)
{
  GtkCssBorderCornerRadius corner;

  if (!_gtk_css_parser_try_double (parser, &corner.horizontal))
    {
      _gtk_css_parser_error (parser, "Expected a number");
      return FALSE;
    }
  else if (corner.horizontal < 0)
    goto negative;

  if (!_gtk_css_parser_try_double (parser, &corner.vertical))
    corner.vertical = corner.horizontal;
  else if (corner.vertical < 0)
    goto negative;

  g_value_set_boxed (value, &corner);
  return TRUE;

negative:
  _gtk_css_parser_error (parser, "Border radius values cannot be negative");
  return FALSE;
}

static void
border_corner_radius_value_print (const GValue *value,
                                  GString      *string)
{
  GtkCssBorderCornerRadius *corner;

  corner = g_value_get_boxed (value);

  if (corner == NULL)
    {
      g_string_append (string, "none");
      return;
    }

  string_append_double (string, corner->horizontal);
  if (corner->horizontal != corner->vertical)
    {
      g_string_append_c (string, ' ');
      string_append_double (string, corner->vertical);
    }
}

static gboolean 
border_radius_value_parse (GtkCssParser *parser,
                           GFile        *base,
                           GValue       *value)
{
  GtkCssBorderRadius border;

  if (!_gtk_css_parser_try_double (parser, &border.top_left.horizontal))
    {
      _gtk_css_parser_error (parser, "Expected a number");
      return FALSE;
    }
  else if (border.top_left.horizontal < 0)
    goto negative;

  if (_gtk_css_parser_try_double (parser, &border.top_right.horizontal))
    {
      if (border.top_right.horizontal < 0)
        goto negative;
      if (_gtk_css_parser_try_double (parser, &border.bottom_right.horizontal))
        {
          if (border.bottom_right.horizontal < 0)
            goto negative;
          if (!_gtk_css_parser_try_double (parser, &border.bottom_left.horizontal))
            border.bottom_left.horizontal = border.top_right.horizontal;
          else if (border.bottom_left.horizontal < 0)
            goto negative;
        }
      else
        {
          border.bottom_right.horizontal = border.top_left.horizontal;
          border.bottom_left.horizontal = border.top_right.horizontal;
        }
    }
  else
    {
      border.top_right.horizontal = border.top_left.horizontal;
      border.bottom_right.horizontal = border.top_left.horizontal;
      border.bottom_left.horizontal = border.top_left.horizontal;
    }

  if (_gtk_css_parser_try (parser, "/", TRUE))
    {
      if (!_gtk_css_parser_try_double (parser, &border.top_left.vertical))
        {
          _gtk_css_parser_error (parser, "Expected a number");
          return FALSE;
        }
      else if (border.top_left.vertical < 0)
        goto negative;

      if (_gtk_css_parser_try_double (parser, &border.top_right.vertical))
        {
          if (border.top_right.vertical < 0)
            goto negative;
          if (_gtk_css_parser_try_double (parser, &border.bottom_right.vertical))
            {
              if (border.bottom_right.vertical < 0)
                goto negative;
              if (!_gtk_css_parser_try_double (parser, &border.bottom_left.vertical))
                border.bottom_left.vertical = border.top_right.vertical;
              else if (border.bottom_left.vertical < 0)
                goto negative;
            }
          else
            {
              border.bottom_right.vertical = border.top_left.vertical;
              border.bottom_left.vertical = border.top_right.vertical;
            }
        }
      else
        {
          border.top_right.vertical = border.top_left.vertical;
          border.bottom_right.vertical = border.top_left.vertical;
          border.bottom_left.vertical = border.top_left.vertical;
        }
    }
  else
    {
      border.top_left.vertical = border.top_left.horizontal;
      border.top_right.vertical = border.top_right.horizontal;
      border.bottom_right.vertical = border.bottom_right.horizontal;
      border.bottom_left.vertical = border.bottom_left.horizontal;
    }

  /* border-radius is an int property for backwards-compat reasons */
  g_value_unset (value);
  g_value_init (value, GTK_TYPE_CSS_BORDER_RADIUS);
  g_value_set_boxed (value, &border);

  return TRUE;

negative:
  _gtk_css_parser_error (parser, "Border radius values cannot be negative");
  return FALSE;
}

static void
border_radius_value_print (const GValue *value,
                           GString      *string)
{
  GtkCssBorderRadius *border;

  border = g_value_get_boxed (value);

  if (border == NULL)
    {
      g_string_append (string, "none");
      return;
    }

  string_append_double (string, border->top_left.horizontal);
  if (border->top_left.horizontal != border->top_right.horizontal ||
      border->top_left.horizontal != border->bottom_right.horizontal ||
      border->top_left.horizontal != border->bottom_left.horizontal)
    {
      g_string_append_c (string, ' ');
      string_append_double (string, border->top_right.horizontal);
      if (border->top_left.horizontal != border->bottom_right.horizontal ||
          border->top_right.horizontal != border->bottom_left.horizontal)
        {
          g_string_append_c (string, ' ');
          string_append_double (string, border->bottom_right.horizontal);
          if (border->top_right.horizontal != border->bottom_left.horizontal)
            {
              g_string_append_c (string, ' ');
              string_append_double (string, border->bottom_left.horizontal);
            }
        }
    }

  if (border->top_left.horizontal != border->top_left.vertical ||
      border->top_right.horizontal != border->top_right.vertical ||
      border->bottom_right.horizontal != border->bottom_right.vertical ||
      border->bottom_left.horizontal != border->bottom_left.vertical)
    {
      g_string_append (string, " / ");
      string_append_double (string, border->top_left.vertical);
      if (border->top_left.vertical != border->top_right.vertical ||
          border->top_left.vertical != border->bottom_right.vertical ||
          border->top_left.vertical != border->bottom_left.vertical)
        {
          g_string_append_c (string, ' ');
          string_append_double (string, border->top_right.vertical);
          if (border->top_left.vertical != border->bottom_right.vertical ||
              border->top_right.vertical != border->bottom_left.vertical)
            {
              g_string_append_c (string, ' ');
              string_append_double (string, border->bottom_right.vertical);
              if (border->top_right.vertical != border->bottom_left.vertical)
                {
                  g_string_append_c (string, ' ');
                  string_append_double (string, border->bottom_left.vertical);
                }
            }
        }

    }
}

/*** PACKING ***/

static GParameter *
unpack_border (const GValue *value,
               guint        *n_params,
               const char   *top,
               const char   *left,
               const char   *bottom,
               const char   *right)
{
  GParameter *parameter = g_new0 (GParameter, 4);
  GtkBorder *border = g_value_get_boxed (value);

  parameter[0].name = top;
  g_value_init (&parameter[0].value, G_TYPE_INT);
  g_value_set_int (&parameter[0].value, border->top);
  parameter[1].name = left;
  g_value_init (&parameter[1].value, G_TYPE_INT);
  g_value_set_int (&parameter[1].value, border->left);
  parameter[2].name = bottom;
  g_value_init (&parameter[2].value, G_TYPE_INT);
  g_value_set_int (&parameter[2].value, border->bottom);
  parameter[3].name = right;
  g_value_init (&parameter[3].value, G_TYPE_INT);
  g_value_set_int (&parameter[3].value, border->right);

  *n_params = 4;
  return parameter;
}

static void
pack_border (GValue             *value,
             GtkStyleProperties *props,
             GtkStateFlags       state,
             const char         *top,
             const char         *left,
             const char         *bottom,
             const char         *right)
{
  GtkBorder border;
  int t, l, b, r;

  gtk_style_properties_get (props,
                            state,
                            top, &t,
                            left, &l,
                            bottom, &b,
                            right, &r,
                            NULL);

  border.top = t;
  border.left = l;
  border.bottom = b;
  border.right = r;

  g_value_set_boxed (value, &border);
}

static GParameter *
unpack_border_width (const GValue *value,
                     guint        *n_params)
{
  return unpack_border (value, n_params,
                        "border-top-width", "border-left-width",
                        "border-bottom-width", "border-right-width");
}

static void
pack_border_width (GValue             *value,
                   GtkStyleProperties *props,
                   GtkStateFlags       state)
{
  pack_border (value, props, state,
               "border-top-width", "border-left-width",
               "border-bottom-width", "border-right-width");
}

static GParameter *
unpack_padding (const GValue *value,
                guint        *n_params)
{
  return unpack_border (value, n_params,
                        "padding-top", "padding-left",
                        "padding-bottom", "padding-right");
}

static void
pack_padding (GValue             *value,
              GtkStyleProperties *props,
              GtkStateFlags       state)
{
  pack_border (value, props, state,
               "padding-top", "padding-left",
               "padding-bottom", "padding-right");
}

static GParameter *
unpack_margin (const GValue *value,
               guint        *n_params)
{
  return unpack_border (value, n_params,
                        "margin-top", "margin-left",
                        "margin-bottom", "margin-right");
}

static void
pack_margin (GValue             *value,
             GtkStyleProperties *props,
             GtkStateFlags       state)
{
  pack_border (value, props, state,
               "margin-top", "margin-left",
               "margin-bottom", "margin-right");
}

static GParameter *
unpack_border_radius (const GValue *value,
                      guint        *n_params)
{
  GParameter *parameter = g_new0 (GParameter, 4);
  GtkCssBorderRadius *border;
  
  if (G_VALUE_HOLDS_BOXED (value))
    border = g_value_get_boxed (value);
  else
    border = NULL;

  parameter[0].name = "border-top-left-radius";
  g_value_init (&parameter[0].value, GTK_TYPE_CSS_BORDER_CORNER_RADIUS);
  parameter[1].name = "border-top-right-radius";
  g_value_init (&parameter[1].value, GTK_TYPE_CSS_BORDER_CORNER_RADIUS);
  parameter[2].name = "border-bottom-right-radius";
  g_value_init (&parameter[2].value, GTK_TYPE_CSS_BORDER_CORNER_RADIUS);
  parameter[3].name = "border-bottom-left-radius";
  g_value_init (&parameter[3].value, GTK_TYPE_CSS_BORDER_CORNER_RADIUS);
  if (border)
    {
      g_value_set_boxed (&parameter[0].value, &border->top_left);
      g_value_set_boxed (&parameter[1].value, &border->top_right);
      g_value_set_boxed (&parameter[2].value, &border->bottom_right);
      g_value_set_boxed (&parameter[3].value, &border->bottom_left);
    }

  *n_params = 4;
  return parameter;
}

static void
pack_border_radius (GValue             *value,
                    GtkStyleProperties *props,
                    GtkStateFlags       state)
{
  GtkCssBorderCornerRadius *top_left;

  /* NB: We are an int property, so we have to resolve to an int here.
   * So we just resolve to an int. We pick one and stick to it.
   * Lesson learned: Don't query border-radius shorthand, query the 
   * real properties instead. */
  gtk_style_properties_get (props,
                            state,
                            "border-top-left-radius", &top_left,
                            NULL);

  if (top_left)
    g_value_set_int (value, top_left->horizontal);

  g_free (top_left);
}

/*** API ***/

static void
css_string_funcs_init (void)
{
  if (G_LIKELY (parse_funcs != NULL))
    return;

  parse_funcs = g_hash_table_new (NULL, NULL);
  print_funcs = g_hash_table_new (NULL, NULL);

  register_conversion_function (GDK_TYPE_RGBA,
                                rgba_value_parse,
                                rgba_value_print);
  register_conversion_function (GDK_TYPE_COLOR,
                                color_value_parse,
                                color_value_print);
  register_conversion_function (GTK_TYPE_SYMBOLIC_COLOR,
                                symbolic_color_value_parse,
                                symbolic_color_value_print);
  register_conversion_function (PANGO_TYPE_FONT_DESCRIPTION,
                                font_description_value_parse,
                                font_description_value_print);
  register_conversion_function (G_TYPE_BOOLEAN,
                                boolean_value_parse,
                                boolean_value_print);
  register_conversion_function (G_TYPE_INT,
                                int_value_parse,
                                int_value_print);
  register_conversion_function (G_TYPE_UINT,
                                uint_value_parse,
                                uint_value_print);
  register_conversion_function (G_TYPE_DOUBLE,
                                double_value_parse,
                                double_value_print);
  register_conversion_function (G_TYPE_FLOAT,
                                float_value_parse,
                                float_value_print);
  register_conversion_function (G_TYPE_STRING,
                                string_value_parse,
                                string_value_print);
  register_conversion_function (GTK_TYPE_THEMING_ENGINE,
                                theming_engine_value_parse,
                                theming_engine_value_print);
  register_conversion_function (GTK_TYPE_ANIMATION_DESCRIPTION,
                                animation_description_value_parse,
                                animation_description_value_print);
  register_conversion_function (GTK_TYPE_BORDER,
                                border_value_parse,
                                border_value_print);
  register_conversion_function (GTK_TYPE_GRADIENT,
                                gradient_value_parse,
                                gradient_value_print);
  register_conversion_function (CAIRO_GOBJECT_TYPE_PATTERN,
                                pattern_value_parse,
                                NULL);
  register_conversion_function (GTK_TYPE_9SLICE,
                                slice_value_parse,
                                NULL);
  register_conversion_function (GTK_TYPE_SHADOW,
                                shadow_value_parse,
                                shadow_value_print);
  register_conversion_function (G_TYPE_ENUM,
                                enum_value_parse,
                                enum_value_print);
  register_conversion_function (G_TYPE_FLAGS,
                                flags_value_parse,
                                flags_value_print);
}

gboolean
_gtk_style_property_parse_value (const GtkStyleProperty *property,
                                 GValue                 *value,
                                 GtkCssParser           *parser,
                                 GFile                  *base)
{
  GtkStyleParseFunc func;

  g_return_val_if_fail (value != NULL, FALSE);
  g_return_val_if_fail (parser != NULL, FALSE);

  css_string_funcs_init ();

  if (property)
    {
      if (_gtk_css_parser_try (parser, "none", TRUE))
        {
          /* Insert the default value, so it has an opportunity
           * to override other style providers when merged
           */
          g_param_value_set_default (property->pspec, value);
          return TRUE;
        }
      else if (property->property_parse_func)
        {
          GError *error = NULL;
          char *value_str;
          gboolean success;
          
          value_str = _gtk_css_parser_read_value (parser);
          if (value_str == NULL)
            return FALSE;
          
          success = (*property->property_parse_func) (value_str, value, &error);

          g_free (value_str);

          return success;
        }

      func = property->parse_func;
    }
  else
    func = NULL;

  if (func == NULL)
    func = g_hash_table_lookup (parse_funcs,
                                GSIZE_TO_POINTER (G_VALUE_TYPE (value)));
  if (func == NULL)
    func = g_hash_table_lookup (parse_funcs,
                                GSIZE_TO_POINTER (g_type_fundamental (G_VALUE_TYPE (value))));

  if (func == NULL)
    {
      _gtk_css_parser_error (parser,
                             "Cannot convert to type '%s'",
                             g_type_name (G_VALUE_TYPE (value)));
      return FALSE;
    }

  return (*func) (parser, base, value);
}

void
_gtk_style_property_print_value (const GtkStyleProperty *property,
                                 const GValue           *value,
                                 GString                *string)
{
  GtkStylePrintFunc func;

  css_string_funcs_init ();

  if (property)
    func = property->print_func;
  else
    func = NULL;

  if (func == NULL)
    func = g_hash_table_lookup (print_funcs,
                                GSIZE_TO_POINTER (G_VALUE_TYPE (value)));
  if (func == NULL)
    func = g_hash_table_lookup (print_funcs,
                                GSIZE_TO_POINTER (g_type_fundamental (G_VALUE_TYPE (value))));

  if (func == NULL)
    {
      char *s = g_strdup_value_contents (value);
      g_string_append (string, s);
      g_free (s);
      return;
    }
  
  func (value, string);
}

gboolean
_gtk_style_property_is_shorthand  (const GtkStyleProperty *property)
{
  g_return_val_if_fail (property != NULL, FALSE);

  return property->pack_func != NULL;
}

GParameter *
_gtk_style_property_unpack (const GtkStyleProperty *property,
                            const GValue           *value,
                            guint                  *n_params)
{
  g_return_val_if_fail (property != NULL, NULL);
  g_return_val_if_fail (property->unpack_func != NULL, NULL);
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (n_params != NULL, NULL);

  return property->unpack_func (value, n_params);
}

void
_gtk_style_property_pack (const GtkStyleProperty *property,
                          GtkStyleProperties     *props,
                          GtkStateFlags           state,
                          GValue                 *value)
{
  g_return_if_fail (property != NULL);
  g_return_if_fail (property->pack_func != NULL);
  g_return_if_fail (GTK_IS_STYLE_PROPERTIES (props));
  g_return_if_fail (G_IS_VALUE (value));

  property->pack_func (value, props, state);
}

static void
gtk_style_property_init (void)
{
  GParamSpec *pspec;

  if (G_LIKELY (properties))
    return;

  /* stuff is never freed, so no need for free functions */
  properties = g_hash_table_new (g_str_hash, g_str_equal);

  /* note that gtk_style_properties_register_property() calls this function,
   * so make sure we're sanely inited to avoid infloops */

  pspec = g_param_spec_boxed ("color",
                              "Foreground color",
                              "Foreground color",
                              GDK_TYPE_RGBA, 0);
  gtk_style_param_set_inherit (pspec, TRUE);
  gtk_style_properties_register_property (NULL, pspec);

  gtk_style_properties_register_property (NULL,
                                          g_param_spec_boxed ("background-color",
                                                              "Background color",
                                                              "Background color",
                                                              GDK_TYPE_RGBA, 0));

  pspec = g_param_spec_boxed ("font",
                              "Font Description",
                              "Font Description",
                              PANGO_TYPE_FONT_DESCRIPTION, 0);
  gtk_style_param_set_inherit (pspec, TRUE);
  gtk_style_properties_register_property (NULL, pspec);

  pspec = g_param_spec_boxed ("text-shadow",
                              "Text shadow",
                              "Text shadow",
                              GTK_TYPE_SHADOW, 0);
  gtk_style_param_set_inherit (pspec, TRUE);
  gtk_style_properties_register_property (NULL, pspec);

  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("margin-top",
                                                            "margin top",
                                                            "Margin at top",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("margin-left",
                                                            "margin left",
                                                            "Margin at left",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("margin-bottom",
                                                            "margin bottom",
                                                            "Margin at bottom",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("margin-right",
                                                            "margin right",
                                                            "Margin at right",
                                                            0, G_MAXINT, 0, 0));
  _gtk_style_property_register           (g_param_spec_boxed ("margin",
                                                              "Margin",
                                                              "Margin",
                                                              GTK_TYPE_BORDER, 0),
                                          NULL,
                                          unpack_margin,
                                          pack_margin,
                                          NULL,
                                          NULL);
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("padding-top",
                                                            "padding top",
                                                            "Padding at top",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("padding-left",
                                                            "padding left",
                                                            "Padding at left",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("padding-bottom",
                                                            "padding bottom",
                                                            "Padding at bottom",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("padding-right",
                                                            "padding right",
                                                            "Padding at right",
                                                            0, G_MAXINT, 0, 0));
  _gtk_style_property_register           (g_param_spec_boxed ("padding",
                                                              "Padding",
                                                              "Padding",
                                                              GTK_TYPE_BORDER, 0),
                                          NULL,
                                          unpack_padding,
                                          pack_padding,
                                          NULL,
                                          NULL);
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("border-top-width",
                                                            "border top width",
                                                            "Border width at top",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("border-left-width",
                                                            "border left width",
                                                            "Border width at left",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("border-bottom-width",
                                                            "border bottom width",
                                                            "Border width at bottom",
                                                            0, G_MAXINT, 0, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_int ("border-right-width",
                                                            "border right width",
                                                            "Border width at right",
                                                            0, G_MAXINT, 0, 0));
  _gtk_style_property_register           (g_param_spec_boxed ("border-width",
                                                              "Border width",
                                                              "Border width, in pixels",
                                                              GTK_TYPE_BORDER, 0),
                                          NULL,
                                          unpack_border_width,
                                          pack_border_width,
                                          NULL,
                                          NULL);

  _gtk_style_property_register           (g_param_spec_boxed ("border-top-left-radius",
                                                              "Border top left radius",
                                                              "Border radius of top left corner, in pixels",
                                                              GTK_TYPE_CSS_BORDER_CORNER_RADIUS, 0),
                                          NULL,
                                          NULL,
                                          NULL,
                                          border_corner_radius_value_parse,
                                          border_corner_radius_value_print);
  _gtk_style_property_register           (g_param_spec_boxed ("border-top-right-radius",
                                                              "Border top right radius",
                                                              "Border radius of top right corner, in pixels",
                                                              GTK_TYPE_CSS_BORDER_CORNER_RADIUS, 0),
                                          NULL,
                                          NULL,
                                          NULL,
                                          border_corner_radius_value_parse,
                                          border_corner_radius_value_print);
  _gtk_style_property_register           (g_param_spec_boxed ("border-bottom-right-radius",
                                                              "Border bottom right radius",
                                                              "Border radius of bottom right corner, in pixels",
                                                              GTK_TYPE_CSS_BORDER_CORNER_RADIUS, 0),
                                          NULL,
                                          NULL,
                                          NULL,
                                          border_corner_radius_value_parse,
                                          border_corner_radius_value_print);
  _gtk_style_property_register           (g_param_spec_boxed ("border-bottom-left-radius",
                                                              "Border bottom left radius",
                                                              "Border radius of bottom left corner, in pixels",
                                                              GTK_TYPE_CSS_BORDER_CORNER_RADIUS, 0),
                                          NULL,
                                          NULL,
                                          NULL,
                                          border_corner_radius_value_parse,
                                          border_corner_radius_value_print);
  _gtk_style_property_register           (g_param_spec_int ("border-radius",
                                                            "Border radius",
                                                            "Border radius, in pixels",
                                                            0, G_MAXINT, 0, 0),
                                          NULL,
                                          unpack_border_radius,
                                          pack_border_radius,
                                          border_radius_value_parse,
                                          border_radius_value_print);

  gtk_style_properties_register_property (NULL,
                                          g_param_spec_enum ("border-style",
                                                             "Border style",
                                                             "Border style",
                                                             GTK_TYPE_BORDER_STYLE,
                                                             GTK_BORDER_STYLE_NONE, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_boxed ("border-color",
                                                              "Border color",
                                                              "Border color",
                                                              GDK_TYPE_RGBA, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_boxed ("background-image",
                                                              "Background Image",
                                                              "Background Image",
                                                              CAIRO_GOBJECT_TYPE_PATTERN, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_boxed ("border-image",
                                                              "Border Image",
                                                              "Border Image",
                                                              GTK_TYPE_9SLICE, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_object ("engine",
                                                               "Theming Engine",
                                                               "Theming Engine",
                                                               GTK_TYPE_THEMING_ENGINE, 0));
  gtk_style_properties_register_property (NULL,
                                          g_param_spec_boxed ("transition",
                                                              "Transition animation description",
                                                              "Transition animation description",
                                                              GTK_TYPE_ANIMATION_DESCRIPTION, 0));

  /* Private property holding the binding sets */
  _gtk_style_property_register           (g_param_spec_boxed ("gtk-key-bindings",
                                                              "Key bindings",
                                                              "Key bindings",
                                                              G_TYPE_PTR_ARRAY, 0),
                                          NULL,
                                          NULL,
                                          NULL,
                                          bindings_value_parse,
                                          bindings_value_print);
}

const GtkStyleProperty *
_gtk_style_property_lookup (const char *name)
{
  gtk_style_property_init ();

  return g_hash_table_lookup (properties, name);
}

void
_gtk_style_property_register (GParamSpec             *pspec,
                              GtkStylePropertyParser  property_parse_func,
                              GtkStyleUnpackFunc      unpack_func,
                              GtkStylePackFunc        pack_func,
                              GtkStyleParseFunc       parse_func,
                              GtkStylePrintFunc       print_func)
{
  const GtkStyleProperty *existing;
  GtkStyleProperty *node;

  g_return_if_fail ((pack_func == NULL) == (unpack_func == NULL));

  gtk_style_property_init ();

  existing = _gtk_style_property_lookup (pspec->name);
  if (existing != NULL)
    {
      g_warning ("Property \"%s\" was already registered with type %s",
                 pspec->name, g_type_name (existing->pspec->value_type));
      return;
    }

  node = g_slice_new0 (GtkStyleProperty);
  node->pspec = pspec;
  node->property_parse_func = property_parse_func;
  node->pack_func = pack_func;
  node->unpack_func = unpack_func;
  node->parse_func = parse_func;
  node->print_func = print_func;

  g_hash_table_insert (properties, pspec->name, node);
}