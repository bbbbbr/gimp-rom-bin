/*=======================================================================
              ROM bin load / save plugin for the GIMP
                 Copyright 2018 - X

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


#include "rom_utils.h"


void romimg_calc_image_size(long int file_size,  app_gfx_data * p_app_gfx, rom_gfx_attrib rom_attrib)
{
    printf("romimg_calc_image_size\n");

    // First calculate the number of tiles in the image

    // Tiles are NxN pixels. Calculate size factoring in pixel bit-packing.
    int tiles = file_size / ((rom_attrib.TILE_PIXEL_WIDTH * rom_attrib.TILE_PIXEL_HEIGHT)
                             / (8 / rom_attrib.BITS_PER_PIXEL));


    // Now calculate Width & Height

    // * Width: if less than 128 pixels wide worth of
    //          tiles, then use cumulative tile width.
    //          Otherwise default to 128 (8 tiles)
    if ((tiles * rom_attrib.TILE_PIXEL_WIDTH) < rom_attrib.IMAGE_WIDTH_DEFAULT) {

        // Use number of tiles x size as the width
        p_app_gfx->width = (tiles * rom_attrib.TILE_PIXEL_WIDTH);
    }
    else
    {
        // Determine image width as a function of tile count

        // Starting width is 1 tile wide
        p_app_gfx->width = 1;

        // Keep increasing the width as long as it results in
        // an even multiple of the tiles
        // *and* it's <= 128 pixels wide (the optimal width)
        while ( ((tiles % (p_app_gfx->width * 2)) == 0) &&
                (p_app_gfx->width * 2 * rom_attrib.TILE_PIXEL_WIDTH <= rom_attrib.IMAGE_WIDTH_DEFAULT) ) {

            // Use the doubled width if it's still resulting in
            // an even multiple of the tiles
            p_app_gfx->width *= 2;
        }

        // Scale the width value up to tile-pixel-size
        p_app_gfx->width = (p_app_gfx->width * rom_attrib.TILE_PIXEL_WIDTH);
    }


    // * Height is a function of width, tile height and number of tiles
    //   Round up: Integer rounding up: (x + (n-1)) / n
    p_app_gfx->height = (((tiles * rom_attrib.TILE_PIXEL_WIDTH) + (rom_attrib.IMAGE_WIDTH_DEFAULT - 1))
                   / rom_attrib.IMAGE_WIDTH_DEFAULT);

    // Now scale up by the tile height
    p_app_gfx->height *= rom_attrib.TILE_PIXEL_HEIGHT;

}



int romimg_insert_color_to_map(unsigned char r, unsigned char g, unsigned char b, app_color_data * p_colorpal)
{
    printf("romimg_insert_color_to_map\n");

    // Make sure space is available in the buffer
    if (( (p_colorpal->index) + 2) > (p_colorpal->size * p_colorpal->bytes_per_pixel))
        return -1;

    p_colorpal->p_data[ (p_colorpal->index)++ ] = r;
    p_colorpal->p_data[ (p_colorpal->index)++ ] = g;
    p_colorpal->p_data[ (p_colorpal->index)++ ] = b;

    // Return success
    return 0;
}



// TODO: FEATURE: Consider trying to look for .pal file with name that matches .bin file and load it
int romimg_load_color_data(app_color_data * p_colorpal)
{
    int status = 0;

printf("romimg_load_color_data\n");

    // Check incoming buffer
    if (NULL == p_colorpal->p_data)
        return -1;


    // Initialize color counter to zero colors added
    p_colorpal->index = 0;

    // 1BPP Default
    if (2 == p_colorpal->size) {
        status += romimg_insert_color_to_map(0x00, 0x00, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xA0, 0xA0, 0xA0, p_colorpal);
    }


    // Check if an error occurred
    if (0 != status)
        return -1;
    else
        // Return success
        return 0;
}
