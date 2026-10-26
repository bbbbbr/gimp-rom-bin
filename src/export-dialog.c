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

//
// Generate the widgets straight from the procedure's arguments,
// so the format picker is a "choice" argument on the procedure itself.
// That means the last used format is remembered by GIMP between invocations,
// and that the dialog no longer needs its own main loop.
//
gboolean rom_bin_settings_dialog (GimpProcedure       * procedure,
                                  GimpProcedureConfig * config,
                                  const gchar         * title,
                                  const gchar         * ok_label)
{
    GtkWidget * dialog;
    GtkWidget * widget;
    GtkWidget * vbox;
    gboolean    run;

    dialog = gimp_procedure_dialog_new (procedure, config, title);

    if (ok_label != NULL)
        gimp_procedure_dialog_set_ok_label (GIMP_PROCEDURE_DIALOG (dialog),
                                            ok_label);

    // Explanatory label
    gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                     "rom-bin-info",
                                     "The options below allow you to customize\n"
                                     "the ROM bin image that is created.",
                                     FALSE, FALSE);

    // The format picker, generated from the "mode" choice argument
    widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                               MODE_ARGUMENT,
                                               G_TYPE_NONE);

    gimp_help_set_help_data (widget,
                             "The bit depth and tile layout of the ROM or "
                             "tile file. Formats are grouped by the system "
                             "they come from.",
                             NULL);

    // Stack the label and the picker vertically
    vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                           "rom-bin-vbox",
                                           "rom-bin-info",
                                           MODE_ARGUMENT,
                                           NULL);

    gtk_box_set_spacing (GTK_BOX (vbox), 12);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                "rom-bin-vbox",
                                NULL);

    gtk_widget_show_all (dialog);

    run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

    gtk_widget_destroy (dialog);

    return run;
}
