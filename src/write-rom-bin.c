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
#include <libgimp/gimp.h>

int write_rom_bin(const gchar * filename, gint drawable_id, int image_mode)
{
    int status = 1;

    GimpDrawable * drawable;
    gint bpp;
    GimpPixelRgn rgn;

    FILE * file;


    app_gfx_data   app_gfx;
    app_color_data colorpal; // TODO: rename to app_colorpal?
    rom_gfx_data   rom_gfx;

    rom_bin_init_structs(&rom_gfx, &app_gfx, &colorpal);

    app_gfx.image_mode = image_mode;


    // Get the drawable
    drawable = gimp_drawable_get(drawable_id);

    // Get the BPP
    // This should be 1 byte per pixel (INDEXED)
    bpp = gimp_drawable_bpp(drawable_id);

    // Abort if it's not 1 Byte Per Pixel
    if (1 != bpp) {
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
    app_gfx.size    =  drawable->width * drawable->height * bpp;
    app_gfx.p_data  = malloc(app_gfx.size);

    // Get the image data
    gimp_pixel_rgn_get_rect(&rgn,
                            app_gfx.p_data,
                            0, 0,
                            drawable->width,
                            drawable->height);

    // TODO: Check colormap size and throw a warning if it's too large (4bpp vs 2bpp, etc)
    status = rom_bin_encode_to_indexed(&rom_gfx,
                                       &app_gfx);


    // Free the image data
    free(app_gfx.p_data);

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
