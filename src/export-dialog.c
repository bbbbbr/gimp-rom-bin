/*=======================================================================
              SNES bin load / save plugin for the GIMP
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

#include "lib_snesbin.h"

#include <stdio.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

extern const char SAVE_PROCEDURE[];
extern const char BINARY_NAME[];

// Response structure
struct snesbin_data {
    int       * response;
    GtkWidget * output_mode_combo;
    int      * output_mode;
};

void on_response(GtkDialog * dialog,
                 gint response_id,
                 gpointer user_data)
{
    // Read the value of the combo box and store it in user_data
    struct snesbin_data * data = user_data;

    // Connect to the combo box pointer
    // Then get currently selected string from combo box
    GtkWidget * output_mode_combo = data->output_mode_combo;
    gchar *string = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT(output_mode_combo) );

    // TODO: convert strings to centralized definition
    // Match the string up to an output mode
    if (!(g_strcmp0(string, "2bpp SNES/GB")))
        *(data->output_mode) = SNESBIN_MODE_2BPP;
    else if (!(g_strcmp0(string, "4bpp SNES")))
        *(data->output_mode) = SNESBIN_MODE_4BPP;
    else
        *(data->output_mode) = -1; //

    g_print( "Selected: >> %s <<\n", ( string ? string : "NULL" ) );

    // Free string
    g_free( string );


    // Quit the loop
    gtk_main_quit();

    if(response_id == GTK_RESPONSE_OK)
        *(data->response) = 1;
}

int export_dialog(int * output_mode)
{
    int response = 0;
    struct snesbin_data data;
    GtkWidget * dialog;
    GtkWidget * vbox;
    GtkWidget * label;
    GtkWidget * table;

    GtkWidget * output_mode_combo;


    // Create the dialog
    dialog = gimp_export_dialog_new("SNES bin",
                                    BINARY_NAME,
                                    SAVE_PROCEDURE);

    // Create the VBox
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       vbox, TRUE, TRUE, 2);
    gtk_widget_show(vbox);

    // Create the label
    label = gtk_label_new("The options below allow you to customize\nthe SNES bin image that is created.");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
    gtk_widget_show(label);


    // Create a combo/list box for selecting the mode
    output_mode_combo = gtk_combo_box_text_new();

    // Add the mode select entries
    // TODO: convert strings to centralized definition
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(output_mode_combo), "2bpp SNES/GB");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(output_mode_combo), "4bpp SNES");

    // Select default value
    // TODO: try to auto-detect based on number of colors? (export only)
    gtk_combo_box_set_active(GTK_COMBO_BOX(output_mode_combo), 0);

    // Add it to the box for display and show it
    gtk_box_pack_start(GTK_BOX(vbox), output_mode_combo, FALSE, FALSE, 6);
    gtk_widget_show(output_mode_combo);


    // Connect the controls to the response signal
    data.response      = &response;
    data.output_mode_combo = output_mode_combo;
    data.output_mode = output_mode;

    g_signal_connect(dialog, "response", G_CALLBACK(on_response),   &data);
    g_signal_connect(dialog, "destroy",  G_CALLBACK(gtk_main_quit), NULL);

    // Show the dialog and run it
    gtk_widget_show(dialog);
    gimp_dialog_run(GIMP_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    return response;
}
