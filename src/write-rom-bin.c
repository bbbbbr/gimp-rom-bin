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

//
// Encode a layer of an INDEXED image into a ROM / tile file.
//
// GIMP 3 changes that matter here:
//   * the drawable is a GObject, pixel data comes from a GeglBuffer
//   * reading the buffer in the drawable's *native* format yields palette
//     indexes (1 byte, or 2 with an alpha channel), which is what the
//     encoders expect, no RGB conversion involved
//   * the file is written through GFile so that remote/URI targets work
//
gboolean write_rom_bin (GFile * filename, GimpImage * image,
                        GimpDrawable * drawable, int image_mode,
                        GError ** error)
{
    GeglBuffer   * buffer;
    const Babl   * format;
    GimpParasite * img_parasite;

    app_gfx_data   app_gfx;
    app_color_data colorpal;
    rom_gfx_data   rom_gfx;

    rom_bin_init_structs (&rom_gfx, &app_gfx, &colorpal);

    app_gfx.image_mode = image_mode;


    // Only indexed images make sense for these formats. The bytes per
    // pixel doubles when the layer has an alpha channel.
    if (! gimp_drawable_is_indexed (drawable))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Only indexed images can be written to ROM bin "
                     "formats. Convert the image (Image -> Mode -> "
                     "Indexed) first.");
        return FALSE;
    }

    app_gfx.bytes_per_pixel = (unsigned char) gimp_drawable_get_bpp (drawable);

    if ((app_gfx.bytes_per_pixel < BIN_BITDEPTH_INDEXED) ||
        (app_gfx.bytes_per_pixel >= BIN_BITDEPTH_LAST))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Unsupported image depth (%d bytes per pixel)",
                     app_gfx.bytes_per_pixel);
        return FALSE;
    }

    // Determine the array size for the image then allocate it
    app_gfx.width  = (unsigned int) gimp_drawable_get_width  (drawable);
    app_gfx.height = (unsigned int) gimp_drawable_get_height (drawable);
    app_gfx.size   = (int) (app_gfx.width * app_gfx.height *
                            app_gfx.bytes_per_pixel);

    if (NULL == (app_gfx.p_data = malloc ((size_t) app_gfx.size)))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOSPC,
                     "Out of memory while exporting the image");
        return FALSE;
    }

    // Get the image data, in the native (indexed) format of the layer
    buffer = gimp_drawable_get_buffer (drawable);
    format = gimp_drawable_get_format (drawable);

    gegl_buffer_get (buffer,
                     GEGL_RECTANGLE (0, 0,
                                     (gint) app_gfx.width,
                                     (gint) app_gfx.height),
                     1.0,
                     format,
                     app_gfx.p_data,
                     GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);

    g_object_unref (buffer);


    // TODO: move parasite metadata handling into a function?
    img_parasite = gimp_image_get_parasite (image, "ROM-BIN-SURPLUS-BYTES");

    if (img_parasite)
    {
        // Load surplus (non-encodable) bytes stashed in the gimp metadata
        // parasite at load time, so they get appended back to the file
        app_gfx.surplus_bytes_size = (long int) img_parasite->size;

        if (app_gfx.surplus_bytes_size > 0)
        {
            if (NULL == (app_gfx.p_surplus_bytes =
                             malloc ((size_t) app_gfx.surplus_bytes_size)))
            {
                gimp_parasite_free (img_parasite);
                free (app_gfx.p_data);

                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOSPC,
                             "Out of memory while exporting the image");
                return FALSE;
            }

            memcpy (app_gfx.p_surplus_bytes,
                    (unsigned char *) img_parasite->data,
                    (size_t) app_gfx.surplus_bytes_size);
        }

        gimp_parasite_free (img_parasite);
    }


    // Turn the pixel indexes back into rom / tile data
    rom_bin_encode (&rom_gfx, &app_gfx);

    // TODO: Check colormap size and throw a warning if it's too large (4bpp vs 2bpp, etc)

    // Free the image data
    free (app_gfx.p_data);
    free (app_gfx.p_surplus_bytes);

    // Make sure that the encode produced something to write
    if (rom_gfx.size <= 0 || rom_gfx.p_data == NULL)
    {
        free (rom_gfx.p_data);

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Nothing to write, the image did not encode to any "
                     "rom data");
        return FALSE;
    }

    // Write the data out
    if (! g_file_replace_contents (filename,
                                   (const gchar *) rom_gfx.p_data,
                                   (gsize) rom_gfx.size,
                                   NULL,     // etag
                                   FALSE,    // make backup
                                   G_FILE_CREATE_NONE,
                                   NULL,     // new etag
                                   NULL,     // cancellable
                                   error))
    {
        free (rom_gfx.p_data);
        return FALSE;
    }

    free (rom_gfx.p_data);

    return TRUE;
}
