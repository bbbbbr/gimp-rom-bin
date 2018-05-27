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

    FILE * file;
    long int filesize;

    // TODO : Look into void* vs unsigned char* for ptr_file_data
    void * filedata = NULL;

    image_gfx_data gfx;

        gfx.image_mode = image_mode;
        gfx.width      = 0;
        gfx.height     = 0;
        gfx.p_data     = NULL;


    image_color_data colorpal;

        colorpal.index           = 0;
        colorpal.bytes_per_pixel = 0;
        colorpal.size            = 0;
        colorpal.p_data          = NULL;


    gint32 new_image_id,
           new_layer_id;
    GimpDrawable * drawable;
    GimpPixelRgn rgn;


    // Try to open the file
    file = fopen(filename, "rb");
    if(!file)
        return -1;

    // Get the file size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Now prepare a buffer of that size
    // and read the data.
    filedata = malloc(filesize);
    fread(filedata, filesize, 1, file);

    // Close the file
    fclose(file);

    // Make sure the alloc succeeded
    if(filedata == NULL)
        return -1;


    // Perform the load procedure and free the raw data.
    status = rom_bin_decode_to_indexed(filedata,
                                       filesize,
                                       &gfx,
                                       &colorpal,
                                       image_mode);

    free(filedata);

    // Check to make sure that the load was successful
    if (0 != status)
    {
        printf("Image load failed \n");

        if (gfx.p_data)
            free(gfx.p_data);

        if (colorpal.p_data)
            free(colorpal.p_data);

        printf("Image load failed: free complete \n");

        return -1;
    }



    // Now create the new INDEXED image.
    new_image_id = gimp_image_new(gfx.width, gfx.height, GIMP_INDEXED);

    // Create the new layer
    new_layer_id = gimp_layer_new(new_image_id,
                                  "Background",
                                  gfx.width, gfx.height,
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
                        gfx.width, gfx.height,
                        TRUE, FALSE);

    // Now FINALLY set the pixel data
    gimp_pixel_rgn_set_rect(&rgn,
                            gfx.p_data,
                            0, 0,
                            gfx.width, gfx.height);

    // We're done with the drawable
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    // Free the image data
    free(gfx.p_data);

    // Free the color map data
    free(colorpal.p_data);

    // Add the layer to the image
    gimp_image_add_layer(new_image_id, new_layer_id, 0);

    // Set the filename
    gimp_image_set_filename(new_image_id, filename);

    return new_image_id;
}
