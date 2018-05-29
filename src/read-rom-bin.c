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

#include "read-rom-bin.h"
#include "lib_rom_bin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgimp/gimp.h>

int read_rom_bin(const gchar * filename, int image_mode)
{
    int status = 1;

    gint32 new_image_id,
           new_layer_id;
    GimpDrawable * drawable;
    GimpPixelRgn rgn;

    FILE * file;


    app_gfx_data   app_gfx;
    app_color_data colorpal; // TODO: rename to app_colorpal?
    rom_gfx_data   rom_gfx;

    rom_bin_init_structs(&rom_gfx, &app_gfx, &colorpal);

    app_gfx.image_mode = image_mode;


    // Try to open the file
    file = fopen(filename, "rb");
    if(!file)
        return -1;

    // Get the file size
    fseek(file, 0, SEEK_END);
    rom_gfx.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Now prepare a buffer of that size
    // and read the data.
    rom_gfx.p_data = malloc(rom_gfx.size);
    fread(rom_gfx.p_data, rom_gfx.size, 1, file);

    // Close the file
    fclose(file);

    // Make sure the alloc succeeded
    if(rom_gfx.p_data == NULL)
        return -1;


    // Perform the load procedure and free the raw data.
    status = rom_bin_decode(&rom_gfx,
                            &app_gfx,
                            &colorpal);

    free(rom_gfx.p_data);

    // Check to make sure that the load was successful
    if (0 != status)
    {
        printf("Image load failed \n");

        if (app_gfx.p_data)
            free(app_gfx.p_data);

        if (colorpal.p_data)
            free(colorpal.p_data);

        printf("Image load failed: free complete \n");

        return -1;
    }



    // Now create the new INDEXED image.
    new_image_id = gimp_image_new(app_gfx.width, app_gfx.height, GIMP_INDEXED);

    // Create the new layer
    new_layer_id = gimp_layer_new(new_image_id,
                                  "Background",
                                  app_gfx.width, app_gfx.height,
                                  GIMP_INDEXED_IMAGE,
                                  100,
                                  GIMP_NORMAL_MODE);

    // Get the drawable for the layer
    drawable = gimp_drawable_get(new_layer_id);


    // Set up the indexed color map
    gimp_image_set_colormap(new_image_id, colorpal.p_data, colorpal.size);

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        app_gfx.width, app_gfx.height,
                        TRUE, FALSE);

    // Now FINALLY set the pixel data
    gimp_pixel_rgn_set_rect(&rgn,
                            app_gfx.p_data,
                            0, 0,
                            app_gfx.width, app_gfx.height);

    // We're done with the drawable
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    // Free the image data
    free(app_gfx.p_data);

    // Free the color map data
    free(colorpal.p_data);

    // Add the layer to the image
    gimp_image_add_layer(new_image_id, new_layer_id, 0);

    // Set the filename
    gimp_image_set_filename(new_image_id, filename);

    return new_image_id;
}
