/*=======================================================================
              WebP load / save plugin for the GIMP
                 Copyright 2012 - Nathan Osman

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

#include <string.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "read-webp.h"
#include "write-webp.h"
#include "export-dialog.h"

const char LOAD_PROCEDURE[] = "file-webp-load";
const char SAVE_PROCEDURE[] = "file-webp-save";
const char BINARY_NAME[]    = "file-webp";

// Predeclare our entrypoints
void query();
void run(const gchar *, gint, const GimpParam *, gint *, GimpParam **);

// Declare our plugin entry points
GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run
};

MAIN()

// The query function
void query()
{
    // Load arguments
    static const GimpParamDef load_arguments[] =
    {
        { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
        { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
        { GIMP_PDB_STRING, "raw-filename", "The name entered" }
    };

    // Load return values
    static const GimpParamDef load_return_values[] =
    {
        { GIMP_PDB_IMAGE, "image", "Output image" }
    };

    // Save arguments
    static const GimpParamDef save_arguments[] =
    {
        { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
        { GIMP_PDB_IMAGE,    "image",        "Input image" },
        { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
        { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
        { GIMP_PDB_STRING,   "raw-filename", "The name entered" },
        { GIMP_PDB_FLOAT,    "quality",      "Quality of the image (0 <= quality <= 100)" }
    };

    // Install the load procedure
    gimp_install_procedure(LOAD_PROCEDURE,
                           "Loads images in the WebP file format",
                           "Loads images in the WebP file format",
                           "Nathan Osman",
                           "Copyright Nathan Osman",
                           "2012",
                           "WebP image",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the save procedure
    gimp_install_procedure(SAVE_PROCEDURE,
                           "Saves files in the WebP image format",
                           "Saves files in the WebP image format",
                           "Nathan Osman",
                           "Copyright Nathan Osman",
                           "2012",
                           "WebP image",
                           "RGB*",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Register the load handlers
    gimp_register_file_handler_mime(LOAD_PROCEDURE, "image/webp");
    gimp_register_load_handler(LOAD_PROCEDURE, "webp", "");

    // Now register the save handlers
    gimp_register_file_handler_mime(SAVE_PROCEDURE, "image/webp");
    gimp_register_save_handler(SAVE_PROCEDURE, "webp", "");
}

// The run function
void run(const gchar * name,
         gint nparams,
         const GimpParam * param,
         gint * nreturn_vals,
         GimpParam ** return_vals)
{
    // Create the return value.
    static GimpParam return_values[2];
    *nreturn_vals = 1;
    *return_vals  = return_values;

    // Set the return value to success by default
    return_values[0].type          = GIMP_PDB_STATUS;
    return_values[0].data.d_status = GIMP_PDB_SUCCESS;

    // Check to see if this is the load procedure
    if(!strcmp(name, LOAD_PROCEDURE))
    {
        int new_image_id;

        // Check to make sure all parameters were supplied
        if(nparams != 3)
        {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }

        // Now read the image
        new_image_id = read_webp(param[1].data.d_string);

        // Check for an error
        if(new_image_id == -1)
        {
            return_values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            return;
        }

        // Fill in the second return value
        *nreturn_vals = 2;

        return_values[1].type         = GIMP_PDB_IMAGE;
        return_values[1].data.d_image = new_image_id;
    }
    else if(!strcmp(name, SAVE_PROCEDURE))
    {
        gint32 image_id, drawable_id;
        int status = 1;
        float quality;
        GimpExportReturn export_ret;

        // Check to make sure all of the parameters were supplied
        if(nparams != 6)
        {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }

        image_id    = param[1].data.d_int32;
        drawable_id = param[2].data.d_int32;

        // Try to export the image
        gimp_ui_init(BINARY_NAME, FALSE);
        export_ret = gimp_export_image(&image_id,
                                       &drawable_id,
                                       "WEBP",
                                       GIMP_EXPORT_CAN_HANDLE_RGB);

        switch(export_ret)
        {
            case GIMP_EXPORT_EXPORT:
            case GIMP_EXPORT_IGNORE:

                // Now get the settings
                if(!export_dialog(&quality))
                {
                    return_values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }

                status = write_webp(param[3].data.d_string,
                                    drawable_id, quality);
                gimp_image_delete(image_id);

                break;
            case GIMP_EXPORT_CANCEL:
                return_values[0].data.d_status = GIMP_PDB_CANCEL;
                return;
        }

        if(!status)
            return_values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
    else
        return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
}
