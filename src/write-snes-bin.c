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

#include "write-snes-bin.h"
#include "lib_snesbin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgimp/gimp.h>

int write_snesbin(const gchar * filename, gint drawable_id, int image_mode)
{
    int status = 1;

    GimpDrawable * drawable;
    gint bpp;
    GimpPixelRgn rgn;

    long int source_data_size = 0;
    guchar * source_image_data = NULL;

    // TODO: was size_t output_size = 0;
    long int output_size = 0;
    unsigned char * output_data = NULL;
    FILE * file;

    // Get the drawable
    drawable = gimp_drawable_get(drawable_id);

    // Get the BPP
    // This should be 1 byte per pixel (INDEXED)
    bpp = gimp_drawable_bpp(drawable_id);

    // TODO: abort if it's not 1 Byte Per Pixel

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        drawable->width,
                        drawable->height,
                        FALSE, FALSE);


    // Determine the size of the array of image data to get
    // and allocate it.
    source_data_size = drawable->width * drawable->height * bpp;
    source_image_data = malloc(source_data_size);

    // Get the image data
    gimp_pixel_rgn_get_rect(&rgn,
                            source_image_data,
                            0, 0,
                            drawable->width,
                            drawable->height);

    // TODO: Check colormap size and throw a warning if it's too large (4bpp vs 2bpp, etc)

    // TODO: Encode image data to bin file + output format option/dialog
    status = snesbin_encode_to_indexed(source_image_data,
                                       drawable->width,
                                       drawable->height,
                                       &output_size,
                                       &output_data,
                                       image_mode);


    // Free the image data
    free(source_image_data);

    // Detach the drawable
    gimp_drawable_detach(drawable);

    // Make sure that the write was successful
    if(output_size == FALSE)
    {
        free(output_data);
        return 0;
    }

    // Open the file
    file = fopen(filename, "wb");
    if(!file)
    {
        free(output_data);
        return 0;
    }

    // Write the data and close it
    fwrite(output_data, output_size, 1, file);
    free(output_data);
    fclose(file);

    return 1;
}
