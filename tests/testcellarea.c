#include <gtk/gtk.h>
#include "cellareascaffold.h"

enum {
  SIMPLE_COLUMN_NAME,
  SIMPLE_COLUMN_ICON,
  SIMPLE_COLUMN_DESCRIPTION,
  N_SIMPLE_COLUMNS
};

static GtkCellRenderer *cell_1 = NULL, *cell_2 = NULL, *cell_3 = NULL;

static GtkTreeModel *
simple_list_model (void)
{
  GtkTreeIter   iter;
  GtkListStore *store = 
    gtk_list_store_new (N_SIMPLE_COLUMNS,
			G_TYPE_STRING,  /* name text */
			G_TYPE_STRING,  /* icon name */
			G_TYPE_STRING); /* description text */

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Alice in wonderland",
		      SIMPLE_COLUMN_ICON, "gtk-execute",
		      SIMPLE_COLUMN_DESCRIPTION, "One pill makes you smaller and the other pill makes you tall",
		      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Highschool Principal",
		      SIMPLE_COLUMN_ICON, "gtk-help",
		      SIMPLE_COLUMN_DESCRIPTION, 
		      "Will make you copy the dictionary if you dont like your math teacher",
		      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Marry Poppins",
		      SIMPLE_COLUMN_ICON, "gtk-yes",
		      SIMPLE_COLUMN_DESCRIPTION, "Supercalifragilisticexpialidocious",
		      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "George Bush",
		      SIMPLE_COLUMN_ICON, "gtk-dialog-warning",
		      SIMPLE_COLUMN_DESCRIPTION, "Please hide your nuclear weapons when inviting "
		      "him to dinner",
		      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 
		      SIMPLE_COLUMN_NAME, "Whinnie the pooh",
		      SIMPLE_COLUMN_ICON, "gtk-stop",
		      SIMPLE_COLUMN_DESCRIPTION, "The most wonderful thing about tiggers, "
		      "is tiggers are wonderful things",
		      -1);

  return (GtkTreeModel *)store;
}

static GtkWidget *
simple_scaffold (void)
{
  GtkTreeModel *model;
  GtkWidget *scaffold;
  GtkCellArea *area;
  GtkCellRenderer *renderer;

  scaffold = cell_area_scaffold_new ();
  gtk_widget_show (scaffold);

  model = simple_list_model ();

  cell_area_scaffold_set_model (CELL_AREA_SCAFFOLD (scaffold), model);

  area = cell_area_scaffold_get_area (CELL_AREA_SCAFFOLD (scaffold));

  cell_1 = renderer = gtk_cell_renderer_text_new ();
  gtk_cell_area_box_pack_start (GTK_CELL_AREA_BOX (area), renderer, FALSE, FALSE);
  gtk_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_NAME);

  cell_2 = renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0F, NULL);
  gtk_cell_area_box_pack_start (GTK_CELL_AREA_BOX (area), renderer, TRUE, FALSE);
  gtk_cell_area_attribute_connect (area, renderer, "stock-id", SIMPLE_COLUMN_ICON);

  cell_3 = renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), 
		"wrap-mode", PANGO_WRAP_WORD,
		"wrap-width", 215,
		NULL);
  gtk_cell_area_box_pack_start (GTK_CELL_AREA_BOX (area), renderer, FALSE, TRUE);
  gtk_cell_area_attribute_connect (area, renderer, "text", SIMPLE_COLUMN_DESCRIPTION);

  return scaffold;
}

static void
orientation_changed (GtkComboBox      *combo,
		     CellAreaScaffold *scaffold)
{
  GtkOrientation orientation = gtk_combo_box_get_active (combo);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (scaffold), orientation);
}

static void
align_cell_2_toggled (GtkToggleButton  *toggle,
		      CellAreaScaffold *scaffold)
{
  GtkCellArea *area = cell_area_scaffold_get_area (scaffold);
  gboolean     align = gtk_toggle_button_get_active (toggle);

  gtk_cell_area_cell_set (area, cell_2, "align", align, NULL);
}

static void
align_cell_3_toggled (GtkToggleButton  *toggle,
		      CellAreaScaffold *scaffold)
{
  GtkCellArea *area = cell_area_scaffold_get_area (scaffold);
  gboolean     align = gtk_toggle_button_get_active (toggle);

  gtk_cell_area_cell_set (area, cell_3, "align", align, NULL);
}

static void
expand_cell_1_toggled (GtkToggleButton  *toggle,
		       CellAreaScaffold *scaffold)
{
  GtkCellArea *area = cell_area_scaffold_get_area (scaffold);
  gboolean     expand = gtk_toggle_button_get_active (toggle);

  gtk_cell_area_cell_set (area, cell_1, "expand", expand, NULL);
}

static void
expand_cell_2_toggled (GtkToggleButton  *toggle,
		       CellAreaScaffold *scaffold)
{
  GtkCellArea *area = cell_area_scaffold_get_area (scaffold);
  gboolean     expand = gtk_toggle_button_get_active (toggle);

  gtk_cell_area_cell_set (area, cell_2, "expand", expand, NULL);
}

static void
expand_cell_3_toggled (GtkToggleButton  *toggle,
		       CellAreaScaffold *scaffold)
{
  GtkCellArea *area = cell_area_scaffold_get_area (scaffold);
  gboolean     expand = gtk_toggle_button_get_active (toggle);

  gtk_cell_area_cell_set (area, cell_3, "expand", expand, NULL);
}

static void
simple_cell_area (void)
{
  GtkWidget *window, *widget;
  GtkWidget *scaffold, *frame, *vbox, *hbox;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  scaffold = simple_scaffold ();

  hbox  = gtk_hbox_new (FALSE, 4);
  frame = gtk_frame_new (NULL);
  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_set_valign (frame, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (frame, GTK_ALIGN_FILL);

  gtk_container_add (GTK_CONTAINER (frame), scaffold);

  gtk_box_pack_end (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  /* Now add some controls */
  vbox  = gtk_vbox_new (FALSE, 4);
  gtk_widget_show (vbox);
  gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  widget = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), "Horizontal");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), "Vertical");
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (widget), "changed",
                    G_CALLBACK (orientation_changed), scaffold);

  widget = gtk_check_button_new_with_label ("Align 2nd Cell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_2_toggled), scaffold);

  widget = gtk_check_button_new_with_label ("Align 3rd Cell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (align_cell_3_toggled), scaffold);


  widget = gtk_check_button_new_with_label ("Expand 1st Cell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_1_toggled), scaffold);

  widget = gtk_check_button_new_with_label ("Expand 2nd Cell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_2_toggled), scaffold);

  widget = gtk_check_button_new_with_label ("Expand 3rd Cell");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
  gtk_widget_show (widget);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  
  g_signal_connect (G_OBJECT (widget), "toggled",
                    G_CALLBACK (expand_cell_3_toggled), scaffold);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  gtk_widget_show (window);
}

int
main (int argc, char *argv[])
{
  gtk_init (NULL, NULL);

  simple_cell_area ();

  gtk_main ();

  return 0;
}