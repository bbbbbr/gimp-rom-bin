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

#include "write-webp.h"

#include <stdio.h>
#include <libgimp/gimp.h>
#include <webp/encode.h>

int write_webp(const gchar * filename, gint drawable_id, float quality)
{
    GimpDrawable * drawable;
    gint bpp;
    GimpPixelRgn rgn;
    long int data_size;
    void * image_data;
    size_t output_size;
    uint8_t * raw_data;
    FILE * file;

    // Get the drawable
    drawable = gimp_drawable_get(drawable_id);

    // Get the BPP
    bpp = gimp_drawable_bpp(drawable_id);

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        drawable->width,
                        drawable->height,
                        FALSE, FALSE);

    // Determine the size of the array of image data to get
    // and allocate it.
    data_size = drawable->width * drawable->height * bpp;
    image_data = malloc(data_size);

    // Get the image data
    gimp_pixel_rgn_get_rect(&rgn,
                            (guchar *)image_data,
                            0, 0,
                            drawable->width,
                            drawable->height);

    // We have the image data, now encode it.
    output_size = WebPEncodeRGB((const uint8_t *)image_data,
                                drawable->width,
                                drawable->height,
                                drawable->width * 3,
                                quality,
                                &raw_data);

    // Free the image data
    free(image_data);

    // Detach the drawable
    gimp_drawable_detach(drawable);

    // Make sure that the write was successful
    if(output_size == FALSE)
    {
        free(raw_data);
        return 0;
    }

    // Open the file
    file = fopen(filename, "wb");
    if(!file)
    {
        free(raw_data);
        return 0;
    }

    // Write the data and be done with it.
    fwrite(raw_data, output_size, 1, file);
    free(raw_data);
    fclose(file);

    return 1;
}
