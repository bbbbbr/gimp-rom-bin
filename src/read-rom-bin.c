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

//
// Read a ROM / tile file and return a new INDEXED GimpImage.
//
// GIMP 3 changes that matter here:
//   * images, layers and drawables are GObjects instead of gint32 ids
//   * GimpPixelRgn is gone, pixel data is moved through a GeglBuffer
//   * an indexed image keeps its colormap in a GimpPalette object
//   * for an indexed drawable the GeglBuffer stores palette *indexes*
//     (2 bytes per pixel when the layer has an alpha channel), which is
//     exactly the layout the decoders produce
//
GimpImage * read_rom_bin (GFile * filename, int image_mode, GError ** error)
{
    GimpImage    * image;
    GimpLayer    * layer;
    GimpPalette  * palette;
    GeglBuffer   * buffer;
    GimpParasite * parasite;

    gchar        * file_contents = NULL;
    gsize          file_length   = 0;

    app_gfx_data   app_gfx;
    app_color_data colorpal;
    rom_gfx_data   rom_gfx;

    rom_bin_init_structs (&rom_gfx, &app_gfx, &colorpal);

    app_gfx.image_mode      = image_mode;
    app_gfx.bytes_per_pixel = BIN_BITDEPTH_INDEXED_ALPHA;


    // Try to read the whole file into memory
    if (! g_file_load_contents (filename, NULL, &file_contents,
                                &file_length, NULL, error))
        return NULL;

    rom_gfx.p_data = (unsigned char *) file_contents;
    rom_gfx.size   = (long int) file_length;

    // Make sure the file wasn't empty before decoding from it
    if (rom_gfx.size <= 0)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "The file '%s' is empty",
                     g_file_peek_path (filename) ? g_file_peek_path (filename) : "");

        g_free (file_contents);
        return NULL;
    }

    // Perform the decode procedure. The raw rom data is owned by this
    // function, the decoded pixel + colormap buffers by the decoders.
    if (0 != rom_bin_decode (&rom_gfx, &app_gfx, &colorpal))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Could not decode '%s' as a %s image",
                     g_file_peek_path (filename) ? g_file_peek_path (filename) : "",
                     rom_bin_mode_entry_by_mode (image_mode)
                         ? rom_bin_mode_entry_by_mode (image_mode)->label
                         : "ROM bin");

        g_free (file_contents);
        free (app_gfx.p_data);
        free (colorpal.p_data);

        return NULL;
    }

    g_free (file_contents);

    if ((app_gfx.width == 0) || (app_gfx.height == 0) ||
        (app_gfx.p_data == NULL))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "The decoded image has no pixels");

        free (app_gfx.p_data);
        free (colorpal.p_data);

        return NULL;
    }


    // Now create the new INDEXED image
    image = gimp_image_new ((gint) app_gfx.width, (gint) app_gfx.height,
                            GIMP_INDEXED);

    if (image == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "Could not create the image");

        free (app_gfx.p_data);
        free (colorpal.p_data);

        return NULL;
    }

    // Create the background layer, with an alpha channel so that the
    // "no rom data here" padding tiles stay identifiable on export
    layer = gimp_layer_new (image, "Background",
                            (gint) app_gfx.width, (gint) app_gfx.height,
                            GIMP_INDEXEDA_IMAGE,
                            100,
                            gimp_image_get_default_new_layer_mode (image));

    gimp_image_insert_layer (image, layer, NULL, 0);


    // Set up the indexed color map. GIMP 3 keeps it in the image's
    // palette object instead of a per-image colormap array.
    // (colorpal.size is the *number of colors*, 3 bytes each)
    if ((colorpal.p_data != NULL) && (colorpal.size > 0))
    {
        palette = gimp_image_get_palette (image);

        if (palette != NULL)
        {
            gimp_palette_set_colormap (palette,
                                       babl_format ("R'G'B' u8"),
                                       colorpal.p_data,
                                       (gsize) colorpal.size *
                                       (gsize) colorpal.bytes_per_pixel);

            // Note: gimp_image_get_palette() is transfer-none, the palette
            // proxy is owned by the image, do not unref it here
        }
    }

    // Now FINALLY set the pixel data, in the drawable's native format:
    // for an indexed layer that is one palette index per pixel (plus an
    // alpha byte when the layer has one)
    buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

    gegl_buffer_set (buffer,
                     GEGL_RECTANGLE (0, 0,
                                     (gint) app_gfx.width,
                                     (gint) app_gfx.height),
                     0,
                     gimp_drawable_get_format (GIMP_DRAWABLE (layer)),
                     app_gfx.p_data,
                     GEGL_AUTO_ROWSTRIDE);

    g_object_unref (buffer);


    if ((app_gfx.surplus_bytes_size > 0) &&
        (app_gfx.p_surplus_bytes != NULL))
    {
        // Store surplus (non-decodable) bytes from the rom into a gimp
        // metadata parasite, so that exporting can put them back
        parasite = gimp_parasite_new ("ROM-BIN-SURPLUS-BYTES",
                                      GIMP_PARASITE_PERSISTENT,
                                      (gint) app_gfx.surplus_bytes_size,
                                      app_gfx.p_surplus_bytes);

        gimp_image_attach_parasite (image, parasite);

        gimp_parasite_free (parasite);

        // Free the surplus bytes now that they are stored as a parasite
        free (app_gfx.p_surplus_bytes);
        app_gfx.p_surplus_bytes = NULL;
    }

    // Free the image data
    free (app_gfx.p_data);

    // Free the color map data
    free (colorpal.p_data);

    return image;
}
