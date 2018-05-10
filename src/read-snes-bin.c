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

#include "read-snes-bin.h"
#include "lib_snesbin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgimp/gimp.h>

int read_snesbin(const gchar * filename, int image_mode)
{
    int status = 1;

    FILE * file;
    long int filesize;

    // TODO : Look into void* vs unsigned char* for ptr_file_data
    void * filedata = NULL;

    guchar * image_data = NULL,
           * color_map_data = NULL;

    int width = 0,
        height = 0;
    int color_map_size = 0;

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
    status = snesbin_decode_to_indexed(filedata,
                                       filesize,
                                       &width,
                                       &height,
                                       &image_data,
                                       &color_map_data,
                                       &color_map_size,
                                       image_mode);

    free(filedata);

    // Check to make sure that the load was successful
    if (0 != status)
    {
        printf("Image load failed %lx, %lx\n", (unsigned long)image_data, (unsigned long)color_map_data);

        if (image_data)
            free(image_data);

        if (color_map_data)
            free(color_map_data);

        return -1;
    }



    // Now create the new INDEXED image.
    new_image_id = gimp_image_new(width, height, GIMP_INDEXED);

    // Create the new layer
    new_layer_id = gimp_layer_new(new_image_id,
                                  "Background",
                                  width, height,
                                  GIMP_INDEXED_IMAGE,
                                  100,
                                  GIMP_NORMAL_MODE);

    // Get the drawable for the layer
    drawable = gimp_drawable_get(new_layer_id);


    // Set up the indexed color map
    gimp_image_set_colormap(new_image_id, color_map_data, color_map_size);

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        width, height,
                        TRUE, FALSE);

    // Now FINALLY set the pixel data
    gimp_pixel_rgn_set_rect(&rgn,
                            image_data,
                            0, 0,
                            width, height);

    // We're done with the drawable
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    // Free the image data
    free(image_data);

    // Free the color map data
    free(color_map_data);

    // Add the layer to the image
    gimp_image_add_layer(new_image_id, new_layer_id, 0);

    // Set the filename
    gimp_image_set_filename(new_image_id, filename);

    return new_image_id;
}
