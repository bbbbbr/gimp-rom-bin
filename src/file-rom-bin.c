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

#include <string.h>
#include <stdint.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "lib_rom_bin.h"
#include "read-rom-bin.h"
#include "write-rom-bin.h"
#include "export-dialog.h"

const char LOAD_PROCEDURE_SNESGB_2BPP[] = "file-snes-bin-load-2bpp";
const char LOAD_PROCEDURE_NES_2BPP[] = "file-nes-bin-load-2bpp";
const char LOAD_PROCEDURE_SNES_4BPP[] = "file-snes-bin-load-4bpp";
const char SAVE_PROCEDURE[] = "file-rom-bin-save";
const char BINARY_NAME[]    = "file-rom-bin";

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
        { GIMP_PDB_FLOAT,    "image_mode",  "ROM image format" }
    };

    // Install the load procedure
    gimp_install_procedure(LOAD_PROCEDURE_SNESGB_2BPP,
                           "Loads images in the SNES bin 2-bpp file format",
                           "Loads images in the SNES bin 2-bpp file format",
                           "Others & Nathan Osman (webp plugin base)",
                           "Copyright Others & Nathan Osman (webp plugin base)",
                           "2018",
                           "SNES bin image 2-bpp",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the load procedure
    gimp_install_procedure(LOAD_PROCEDURE_NES_2BPP,
                           "Loads images in the NES bin 2-bpp file format",
                           "Loads images in the NES bin 2-bpp file format",
                           "Others & Nathan Osman (webp plugin base)",
                           "Copyright Others & Nathan Osman (webp plugin base)",
                           "2018",
                           "NES bin image 2-bpp",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the load procedure
    gimp_install_procedure(LOAD_PROCEDURE_SNES_4BPP,
                           "Loads images in the SNES bin 4-bpp file format",
                           "Loads images in the SNES bin 4-bpp file format",
                           "Others & Nathan Osman (webp plugin base)",
                           "Copyright Others & Nathan Osman (webp plugin base)",
                           "2018",
                           "SNES bin image 4-bpp",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the save procedure
    gimp_install_procedure(SAVE_PROCEDURE,
                           "Saves files in the ROM bin image format",
                           "Saves files in the ROM bin image format",
                           "Others & Nathan Osman (webp plugin base)",
                           "Copyright Others & Nathan Osman (webp plugin base)",
                           "2018",
                           "ROM bin image",
                           "INDEXED*",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Register the load handlers

    // TODO: PROBLEM/WORKAROUND-
    //       * LOAD gets called to generate a thumbnail for an
    //         image when browsing in the open dialog. This spawns
    //         lots of unwanted 2/4bpp dialogs.
    //
    //       * For now: use separate 2/4bpp load handlers
    //
    //       * Other workaround: could register a dummy handler with no preview or
    //         non-dialog preview
    //       * Future fix? find a way to suppress the dialog or thumbnail previews
    //
    //       See: gimp_thumbnail_update_thumb, gimp/libgimpthumb/gimpthumbnail.c
    //            gimp/app/pdb/fileops-cmds.c, gimp/app/plug-in/gimpplugin.h

    gimp_register_file_handler_mime(LOAD_PROCEDURE_SNESGB_2BPP, "image/bin");
    gimp_register_load_handler(LOAD_PROCEDURE_SNESGB_2BPP, "bin", "");

    gimp_register_file_handler_mime(LOAD_PROCEDURE_NES_2BPP, "image/bin");
    gimp_register_load_handler(LOAD_PROCEDURE_NES_2BPP, "bin,chr", "");
    // Additional NES handler for ".chr" format files
    gimp_register_file_handler_mime(LOAD_PROCEDURE_NES_2BPP, "image/chr");
    gimp_register_load_handler(LOAD_PROCEDURE_NES_2BPP, "chr", "");


    gimp_register_file_handler_mime(LOAD_PROCEDURE_SNES_4BPP, "image/bin");
    gimp_register_load_handler(LOAD_PROCEDURE_SNES_4BPP, "bin", "");

    // Now register the save handlers
    gimp_register_file_handler_mime(SAVE_PROCEDURE, "image/bin");
    gimp_register_save_handler(SAVE_PROCEDURE, "bin", "");

    // Additional NES handler for ".chr" format files
    gimp_register_file_handler_mime(SAVE_PROCEDURE, "image/chr");
    gimp_register_save_handler(SAVE_PROCEDURE, "chr", "");
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
    if( !strcmp(name, LOAD_PROCEDURE_SNESGB_2BPP) ||
        !strcmp(name, LOAD_PROCEDURE_NES_2BPP) ||
        !strcmp(name, LOAD_PROCEDURE_SNES_4BPP) )
    {
        int new_image_id;
        int image_mode = -1;

        // Check to make sure all parameters were supplied
        if(nparams != 3)
        {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }


        // Try to export the image
        gimp_ui_init(BINARY_NAME, FALSE);

        // Get the settings
        // TODO: PROBLEM
        //       * LOAD gets called to generate a thumbnail for an
        //         image when browsing in the open dialog. This spawns
        //         lots of unwanted 2/4bpp dialogs.
        // if(!export_dialog(&image_mode, name))
        // {
        //     return_values[0].data.d_status = GIMP_PDB_CANCEL;
        //     return;
        // }

        // This is the workaround for the thumbnail/preview dialog spawn problem
        if(!strcmp(name, LOAD_PROCEDURE_SNESGB_2BPP))
          image_mode = BIN_MODE_SNESGB_2BPP;

        else if(!strcmp(name, LOAD_PROCEDURE_NES_2BPP))
          image_mode = BIN_MODE_NES_2BPP;

        else if (!strcmp(name, LOAD_PROCEDURE_SNES_4BPP))
          image_mode = BIN_MODE_SNES_4BPP;


        // Now read the image
        new_image_id = read_rom_bin(param[1].data.d_string, image_mode);

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
        // This is the export procedure

        gint32 image_id, drawable_id;
        int status = 1;
        int image_mode = -1;
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
                                       "BIN",
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED);

        switch(export_ret)
        {
            case GIMP_EXPORT_EXPORT:
            case GIMP_EXPORT_IGNORE:

                // Now get the settings
                if(!export_dialog(&image_mode, name))
                {
                    return_values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }

                status = write_rom_bin(param[3].data.d_string,
                                       drawable_id, image_mode);
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
