#include <locale.h>
#include <gdk/gdk.h>

static void
test_color_parse (void)
{
  GdkRGBA color;
  GdkRGBA expected;
  gboolean res;

  res = gdk_rgba_parse ("foo", &color);
  g_assert (!res);

  res = gdk_rgba_parse ("", &color);
  g_assert (!res);

  expected.red = 0.4;
  expected.green = 0.3;
  expected.blue = 0.2;
  expected.alpha = 0.1;
  res = gdk_rgba_parse ("rgba(0.4,0.3,0.2,0.1)", &color);
  g_assert (res);
  g_assert (gdk_rgba_equal (&color, &expected));

  res = gdk_rgba_parse ("rgba ( 0.4 ,  0.3  ,   0.2 ,  0.1     )", &color);
  g_assert (res);
  g_assert (gdk_rgba_equal (&color, &expected));

  expected.red = 0.4;
  expected.green = 0.3;
  expected.blue = 0.2;
  expected.alpha = 1.0;
  res = gdk_rgba_parse ("rgb(0.4,0.3,0.2)", &color);
  g_assert (res);
  g_assert (gdk_rgba_equal (&color, &expected));

  expected.red = 1.0;
  expected.green = 0.0;
  expected.blue = 0.0;
  expected.alpha = 1.0;
  res = gdk_rgba_parse ("red", &color);
  g_assert (res);
  g_assert (gdk_rgba_equal (&color, &expected));

  expected.red = 0.0;
  expected.green = 0x8080 / 65535.;
  expected.blue = 1.0;
  expected.alpha = 1.0;
  res = gdk_rgba_parse ("#0080ff", &color);
  g_assert (res);
  g_assert (gdk_rgba_equal (&color, &expected));
}

static void
test_color_to_string (void)
{
  GdkRGBA rgba;
  GdkRGBA out;
  gchar *res;
  gchar *res_de;
  gchar *res_en;
  gchar *orig;

  rgba.red = 1.0;
  rgba.green = 0.5;
  rgba.blue = 0.1;
  rgba.alpha = 1.0;

  orig = g_strdup (setlocale (LC_ALL, NULL));
  res = gdk_rgba_to_string (&rgba);
  gdk_rgba_parse (res, &out);
  g_assert (gdk_rgba_equal (&rgba, &out));

  setlocale (LC_ALL, "de_DE.utf-8");
  res_de = gdk_rgba_to_string (&rgba);
  g_assert_cmpstr (res, ==, res_de);

  setlocale (LC_ALL, "en_US.utf-8");
  res_en = gdk_rgba_to_string (&rgba);
  g_assert_cmpstr (res, ==, res_en);

  g_free (res);
  g_free (res_de);
  g_free (res_en);

  setlocale (LC_ALL, orig);
  g_free (orig);
}

int
main (int argc, char *argv[])
{
        g_test_init (&argc, &argv, NULL);
        gdk_init (&argc, &argv);

        g_test_add_func ("/color/parse", test_color_parse);
        g_test_add_func ("/color/to-string", test_color_to_string);

        return g_test_run ();
}