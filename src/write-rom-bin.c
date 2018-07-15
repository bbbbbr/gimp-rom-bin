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

#include "write-rom-bin.h"
#include "lib_rom_bin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

int write_rom_bin(const gchar * filename, gint image_id, gint drawable_id, int image_mode)
{
    int status;

    GimpDrawable * drawable;
    GimpPixelRgn rgn;
    GimpParasite * img_parasite;

    FILE * file;

    status = 0; // Default to success

    app_gfx_data   app_gfx;
    app_color_data colorpal; // TODO: rename to app_colorpal?
    rom_gfx_data   rom_gfx;

    rom_bin_init_structs(&rom_gfx, &app_gfx, &colorpal);

    app_gfx.image_mode = image_mode;


    // Get the drawable
    drawable = gimp_drawable_get(drawable_id);

    // Get the Bytes Per Pixel of the incoming app image
    app_gfx.bytes_per_pixel = (unsigned char)gimp_drawable_bpp(drawable_id);

    // Abort if it's not 1 or 2 bytes per pixel
    // TODO: handle both 1 (no alpha) and 2 (has alpha) byte-per-pixel mode
    if (app_gfx.bytes_per_pixel >= BIN_BITDEPTH_LAST) {
        return 0;
    }

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        drawable->width,
                        drawable->height,
                        FALSE, FALSE);


    // Determine the array size for the app's image then allocate it
    app_gfx.width   = drawable->width;
    app_gfx.height  = drawable->height;
    app_gfx.size    =  drawable->width * drawable->height * app_gfx.bytes_per_pixel;
    app_gfx.p_data  = malloc(app_gfx.size);

    // Get the image data
    gimp_pixel_rgn_get_rect(&rgn,
                            app_gfx.p_data,
                            0, 0,
                            drawable->width,
                            drawable->height);



    // TODO: move parasite metadata handling into a function?
    img_parasite = gimp_image_get_parasite(image_id,
                                           "ROM-BIN-SURPLUS-BYTES");

    if (img_parasite) {
        printf("Found parasite size %d\n", img_parasite->size);

        // Load surplus (non-encodable) bytes stashed in the gimp metadata parasite
        app_gfx.surplus_bytes_size = img_parasite->size;

        if (NULL == (app_gfx.p_surplus_bytes = malloc(app_gfx.surplus_bytes_size)) ) {
            // TODO: handle and centralize freeing these buffers better
            free(app_gfx.p_data);
            free(app_gfx.p_surplus_bytes);
            free(rom_gfx.p_data);
            return 0;
        }

        memcpy(app_gfx.p_surplus_bytes,
               (unsigned char *)img_parasite->data,
               img_parasite->size);
    }



    status = rom_bin_encode(&rom_gfx,
                            &app_gfx);
    // TODO: Check colormap size and throw a warning if it's too large (4bpp vs 2bpp, etc)
    if (status != 0) { };


    // Free the image data
    free(app_gfx.p_data);
    free(app_gfx.p_surplus_bytes);

    // Detach the drawable
    gimp_drawable_detach(drawable);

    // Make sure that the write was successful
    if(rom_gfx.size == FALSE)
    {
        free(rom_gfx.p_data);
        return 0;
    }

    // Open the file
    file = fopen(filename, "wb");
    if(!file)
    {
        free(rom_gfx.p_data);
        return 0;
    }

    // Write the data and close it
    fwrite(rom_gfx.p_data, rom_gfx.size, 1, file);
    free(rom_gfx.p_data);
    fclose(file);

    return 1;
}
