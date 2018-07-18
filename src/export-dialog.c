/*=======================================================================
              ROM bin load / save plugin for the GIMP
                 Copyright 2018 - Others & Nathan Osman (webp plugin base)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
=======================================================================*/

// TODO: rename to settings-dialog.c/h

#include "lib_rom_bin.h"
#include "export-dialog.h"

#include <stdio.h>
#include <string.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

extern const char SAVE_PROCEDURE[];
extern const char LOAD_PROCEDURE[];
extern const char BINARY_NAME[];

// Response structure
struct rom_bin_data {
    int       * response;
    GtkWidget * image_mode_combo;
    int       * image_mode;
};

void on_response(GtkDialog *, gint, gpointer);

void on_response(GtkDialog * dialog,
                 gint response_id,
                 gpointer user_data)
{
    // Read the value of the combo box and store it in user_data
    struct rom_bin_data * data = user_data;

    // Connect to the combo box pointer
    // Then get currently selected string from combo box
    GtkWidget * image_mode_combo = data->image_mode_combo;
    gchar *string = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(image_mode_combo) );

    // TODO: Using the dialog on import is disabled for now- remove support for import prompting?
    // TODO: convert strings to centralized definition
    // Match the string up to an output mode
    if (!(g_strcmp0(string, "1bpp NES")))
        *(data->image_mode) = BIN_MODE_NES_1BPP;
    else if (!(g_strcmp0(string, "2bpp NES")))
        *(data->image_mode) = BIN_MODE_NES_2BPP;
    else if (!(g_strcmp0(string, "2bpp SNES/GB")))
        *(data->image_mode) = BIN_MODE_SNESGB_2BPP;
    else if (!(g_strcmp0(string, "2bpp NGPC")))
        *(data->image_mode) = BIN_MODE_NGPC_2BPP;

    else if (!(g_strcmp0(string, "3bpp SNES")))
        *(data->image_mode) = BIN_MODE_SNES_3BPP;


    else if (!(g_strcmp0(string, "4bpp GBA")))
        *(data->image_mode) = BIN_MODE_GBA_4BPP;
    else if (!(g_strcmp0(string, "4bpp SNES")))
        *(data->image_mode) = BIN_MODE_SNES_4BPP;
    else if (!(g_strcmp0(string, "4bpp GG/SMS/WSC")))
        *(data->image_mode) = BIN_MODE_GGSMSWSC_4BPP;
    else if (!(g_strcmp0(string, "4bpp GEN")))
        *(data->image_mode) = BIN_MODE_GENS_4BPP;

    else if (!(g_strcmp0(string, "8bpp GBA")))
        *(data->image_mode) = BIN_MODE_GBA_8BPP;
    else if (!(g_strcmp0(string, "8bpp SNES")))
        *(data->image_mode) = BIN_MODE_SNES_8BPP;
    else
        *(data->image_mode) = -1; //

    g_print( "Selected: >> %s <<\n", ( string ? string : "NULL" ) );

    // Free string
    g_free( string );


    // Quit the loop
    gtk_main_quit();

    if(response_id == GTK_RESPONSE_OK)
        *(data->response) = 1;
}

int export_dialog(int * image_mode, const gchar * name)
{
    int response = 0;
    struct rom_bin_data data;
    GtkWidget * dialog;
    GtkWidget * vbox;
    GtkWidget * label;

    GtkWidget * image_mode_combo;


    // Create the export dialog

    if(!strcmp(name, SAVE_PROCEDURE)) {
        // If Exporting use the export dialog convenience function
        dialog = gimp_export_dialog_new("ROM bin",
                                        BINARY_NAME,
                                        SAVE_PROCEDURE);
    }
    else {

        // If Opening then make an equivalent dialog
        gchar     *title  = g_strconcat ("Open Image as ", "ROM bin", NULL);

        dialog = gimp_dialog_new (title, BINARY_NAME,
                                  NULL, 0,
                                  gimp_standard_help_func, LOAD_PROCEDURE,
                                  "_Cancel", GTK_RESPONSE_CANCEL,
                                  "_Open", GTK_RESPONSE_OK,
                                  NULL);

        gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                 GTK_RESPONSE_OK,
                                                 GTK_RESPONSE_CANCEL,
                                                 -1);

        gimp_window_set_transient (GTK_WINDOW (dialog));

        g_free (title);
    }


    // Create the VBox
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       vbox, TRUE, TRUE, 2);
    gtk_widget_show(vbox);

    // Create the label
    label = gtk_label_new("The options below allow you to customize\nthe ROM bin image that is created.");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
    gtk_widget_show(label);


    // Create a combo/list box for selecting the mode
    image_mode_combo = gtk_combo_box_text_new();

    // Add the mode select entries
    // TODO: convert strings to centralized definition
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "1bpp NES");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "2bpp NES");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "2bpp SNES/GB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "2bpp NGPC");

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "3bpp SNES");

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "4bpp GBA");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "4bpp SNES");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "4bpp GG/SMS/WSC");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "4bpp GEN");

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "8bpp GBA");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(image_mode_combo), "8bpp SNES");

    // Select default value
    // TODO: try to auto-detect image mode based on number of colors? (export only)
    gtk_combo_box_set_active(GTK_COMBO_BOX(image_mode_combo), 0);

    // Add it to the box for display and show it
    gtk_box_pack_start(GTK_BOX(vbox), image_mode_combo, FALSE, FALSE, 6);
    gtk_widget_show(image_mode_combo);


    // TODO: set Export as default focused button

    // Connect the controls to the response signal
    data.response      = &response;
    data.image_mode_combo = image_mode_combo;
    data.image_mode = image_mode;

    g_signal_connect(dialog, "response", G_CALLBACK(on_response),   &data);
    g_signal_connect(dialog, "destroy",  G_CALLBACK(gtk_main_quit), NULL);

    // TODO: Above is spawning these errors:
    //       IA__gtk_main_quit: assertion 'main_loops != NULL' failed
    //       https://www.gtk.org/tutorial1.2/gtk_tut-2.html
    //       https://ubuntuforums.org/showthread.php?t=394399

    // Show the dialog and run it
    gtk_widget_show(dialog);
    gimp_dialog_run(GIMP_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    return response;
}
