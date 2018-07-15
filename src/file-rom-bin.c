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

const char LOAD_PROCEDURE[] = "file-rom-bin-load";
const char LOAD_PROCEDURE_NES2BPP_CHRNES[] = "file-bin-bin-load-nes2bpp-chrnes";
const char LOAD_PROCEDURE_GB2BPP_GB[] = "file-bin-bin-load-gb2bpp-gb";

const char SAVE_PROCEDURE[]        = "file-rom-bin-save";
const char SAVE_PROCEDURE_NES2BPP_CHRNES[] = "file-rom-bin-save-nes2bpp-chrnes";
const char SAVE_PROCEDURE_GB2BPP_GB[] = "file-rom-bin-save-gb2bpp-gb";

const char BINARY_NAME[]    = "file-rom-bin";

// Predeclare our entrypoints
static void query(void);
static void run(const gchar *, gint, const GimpParam *, gint *, GimpParam **);

// Declare our plugin entry points
GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run
};

MAIN()

// The query function
static void query(void)
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

    // Install the load procedure for ".bin" files (all formats)
    gimp_install_procedure(LOAD_PROCEDURE,
                           "Loads images in the ROM bin file format",
                           "Loads images in the ROM bin file format",
                           "--",
                           "Copyright --",
                           "2018",
                           "ROM GEN/SMS/SNES/NGPC/NES/GB(A) bin image",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the load procedure for ".chr" files (only NES chr 2bpp)
    gimp_install_procedure(LOAD_PROCEDURE_NES2BPP_CHRNES,
                           "Loads images in the NES .chr and .nes 2-bpp file format",
                           "Loads images in the NES .chr and .nes 2-bpp file format",
                           "--",
                           "Copyright --",
                           "2018",
                           "ROM NES .chr and .nes image 2-bpp",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

// Install the load procedure for ".chr" files (only NES chr 2bpp)
    gimp_install_procedure(LOAD_PROCEDURE_GB2BPP_GB,
                           "Loads images in the Gameboy .gb 2-bpp file format",
                           "Loads images in the Gameboy .gb 2-bpp file format",
                           "--",
                           "Copyright --",
                           "2018",
                           "ROM GB .gb image 2-bpp",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);


    // Install the save procedure for ".bin" files (all formats)
    gimp_install_procedure(SAVE_PROCEDURE,
                           "Saves files in the ROM bin image format",
                           "Saves files in the ROM bin image format",
                           "--",
                           "Copyright --",
                           "2018",
                           "ROM GEN/SMS/SNES/NGPC/NES/GB(A) bin image",
                           "INDEXED*",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Install the save procedure for ".chr" files (only NES chr 2bpp)
    gimp_install_procedure(SAVE_PROCEDURE_NES2BPP_CHRNES,
                           "Saves files in the NES .chr and .nes 2-bpp image format",
                           "Saves files in the NES .chr and .nes 2-bpp image format",
                           "--",
                           "Copyright --",
                           "2018",
                           "CHR NES 2bpp bin image",
                           "INDEXED*",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Install the save procedure for ".chr" files (only NES chr 2bpp)
    gimp_install_procedure(SAVE_PROCEDURE_GB2BPP_GB,
                           "Saves files in the Gameboy .gb 2-bpp image format",
                           "Saves files in the Gameboy .gb 2-bpp image format",
                           "--",
                           "Copyright --",
                           "2018",
                           "Gameboy .gb 2bpp bin image",
                           "INDEXED*",
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Register the load handlers
    gimp_register_load_handler(LOAD_PROCEDURE, "bin", "");

    // Additional NES handler for ".chr" format files and NES ROM files
    gimp_register_load_handler(LOAD_PROCEDURE_NES2BPP_CHRNES, "chr,nes", "");

    // Additional NES handler for ".chr" format files and NES ROM files
    gimp_register_load_handler(LOAD_PROCEDURE_GB2BPP_GB, "gb", "");


    // Now register the save handlers
    gimp_register_save_handler(SAVE_PROCEDURE, "bin", "");

    // Additional NES handler for ".chr" format files and NES ROM files
    gimp_register_save_handler(SAVE_PROCEDURE_NES2BPP_CHRNES, "chr,nes", "");

    // Additional NES handler for ".chr" format files and NES ROM files
    gimp_register_save_handler(SAVE_PROCEDURE_GB2BPP_GB, "gb", "");

    // MIME handler registration is disabled for now, due to non-interactive
    //gimp_register_file_handler_mime(LOAD_PROCEDURE, "image/bin");
    //gimp_register_file_handler_mime(LOAD_PROCEDURE_NES_2BPP, "image/chr");
    //gimp_register_file_handler_mime(SAVE_PROCEDURE, "image/bin");
    //gimp_register_file_handler_mime(SAVE_PROCEDURE_NES2BPP_CHRNES, "image/chr");
}

// The run function
static void run(const gchar * name,
         gint nparams,
         const GimpParam * param,
         gint * nreturn_vals,
         GimpParam ** return_vals)
{
    // Create the return value.
    static GimpParam return_values[2];
    *nreturn_vals = 1;
    *return_vals  = return_values;

    GimpRunMode   run_mode;
    run_mode      = param[0].data.d_int32;

    // Set the return value to success by default
    return_values[0].type          = GIMP_PDB_STATUS;
    return_values[0].data.d_status = GIMP_PDB_SUCCESS;


    // Check to see if this is the load procedure
    if( !strcmp(name, LOAD_PROCEDURE) ||
        !strcmp(name, LOAD_PROCEDURE_NES2BPP_CHRNES) ||
        !strcmp(name, LOAD_PROCEDURE_GB2BPP_GB))
    {
        int new_image_id;
        int image_mode = -1;

        // Check to make sure all parameters were supplied
        if(nparams != 3) {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }


        // Try to export the image
        gimp_ui_init(BINARY_NAME, FALSE);


        // Determine image file format, by load type or user dialog
        // * .chr and .nes files auto-default to NES 2bpp,
        // * .gb files auto-default to SNESGB 2bpp,
        //
        //   no need to show image format selection dialog
        if(!strcmp(name, LOAD_PROCEDURE_NES2BPP_CHRNES))
            // TODO: ?try to auto detect 1bpp vs 2bpp mode based on file size (4k = 1bpp, 4k+ = 2bpp)
            image_mode = BIN_MODE_NES_2BPP;
        else if(!strcmp(name, LOAD_PROCEDURE_GB2BPP_GB))
            image_mode = BIN_MODE_SNESGB_2BPP;
        else {
            // Only show settings dialog during interactive mode
            // - Thumbnail preview creation happens in GIMP_RUN_NONINTERACTIVE mode
            // - TODO: GIMP_RUN_WITH_LAST_VALS
            if (GIMP_RUN_INTERACTIVE == run_mode) {

                // Show the import/export dialog
                if(!export_dialog(&image_mode, name)) {
                    return_values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }
            }
            else if (GIMP_RUN_NONINTERACTIVE == run_mode) {

                // Default to SNES 4bpp for non-interactive mode
                image_mode = BIN_MODE_SNES_4BPP;
            }
        }


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
    else if(!strcmp(name, SAVE_PROCEDURE) ||
            !strcmp(name, SAVE_PROCEDURE_NES2BPP_CHRNES) ||
            !strcmp(name, SAVE_PROCEDURE_GB2BPP_GB))
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
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                       GIMP_EXPORT_CAN_HANDLE_ALPHA);

        switch(export_ret)
        {
            case GIMP_EXPORT_EXPORT:
            case GIMP_EXPORT_IGNORE:

                // Only call the export dialog if it is "bin" and not NES 2bpp ".chr" mode
              if(!strcmp(name, SAVE_PROCEDURE_NES2BPP_CHRNES)) {
                // Force NES 2bpp for .chr files
                image_mode = BIN_MODE_NES_2BPP;
              }
              else if(!strcmp(name, SAVE_PROCEDURE_GB2BPP_GB)) {
                // Force NES 2bpp for .chr files
                image_mode = BIN_MODE_SNESGB_2BPP;
              }
              else {
                // Now get the settings
                if(!export_dialog(&image_mode, name))
                {
                    return_values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }
              }

                status = write_rom_bin(param[3].data.d_string,
                                       image_id, drawable_id, image_mode);
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
