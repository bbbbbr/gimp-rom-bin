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

#include <string.h>


void romimg_log_transparent_tiles(unsigned int transparency_flag, unsigned int * p_empty_tile_count, app_gfx_data * p_app_gfx, rom_gfx_attrib rom_attrib)
{
    // Transparent pixels in a tile indicate that this is
    // past the end of valid ROM data. This will get removed
    // from total ROM size later.
    // This can happen if the number of tiles in a rom and
    // their size aren't an even multiple of the total image width
    if ((BIN_BITDEPTH_INDEXED_ALPHA == p_app_gfx->bytes_per_pixel)
       && (transparency_flag >= (rom_attrib.TILE_PIXEL_HEIGHT * rom_attrib.TILE_PIXEL_WIDTH))) {
        (*p_empty_tile_count)++;
    }
}


void romimg_log_transparent_pixel(unsigned char * p_image_pixel, unsigned int * p_transparency_flag,  app_gfx_data * p_app_gfx)
{
    // Found a transparent pixel in the non-encoded image, so this
    // tile may be past the end of valid ROM data. Flag for later
    if ((BIN_BITDEPTH_INDEXED_ALPHA == p_app_gfx->bytes_per_pixel)
        && (*(p_image_pixel + 1) == 0))
        (*p_transparency_flag)++;
}


void romimg_set_decoded_pixel_and_advance(unsigned char ** pp_image_pixel, unsigned char pixel_val, unsigned char is_transparent, app_gfx_data * p_app_gfx)
{
    // Set the image pixel
    **pp_image_pixel = pixel_val;

    // If the image has an transparency alpha mask
    // then set transparency as needed
    if (BIN_BITDEPTH_INDEXED_ALPHA == p_app_gfx->bytes_per_pixel) {
        if (is_transparent)
            *(*(pp_image_pixel) + 1) = 0;   // Set Alpha mask byte to TRANSPARENT (pixel does not contain valid rom data)
        else
            *(*(pp_image_pixel) + 1) = 255; // Set Alpha mask byte to VISIBLE (pixel stores valid rom data)
    }

    // Advance to next pixel in the buffer factoring in bytes depth
    *pp_image_pixel += p_app_gfx->bytes_per_pixel;
}


unsigned char * romimg_calc_appimg_offset(int x, int y, int tile_y, app_gfx_data * p_app_gfx, rom_gfx_attrib rom_attrib)
{
    // Calculate pointer location in image buffer based on x,y and tile y
    return(p_app_gfx->p_data
           + (p_app_gfx->bytes_per_pixel
              * ( (((y * rom_attrib.TILE_PIXEL_HEIGHT) + tile_y) * p_app_gfx->width)
                  + (x * rom_attrib.TILE_PIXEL_WIDTH)) ) );
}



long int romimg_calc_encoded_size(app_gfx_data * p_app_gfx, rom_gfx_attrib rom_attrib)
{
    long int size;

    size = (p_app_gfx->width * p_app_gfx->height) / (8 / rom_attrib.BITS_PER_PIXEL);

    return(size);
}



// TODO: Better handling for files that aren't even multipels of tile size (ex: .nes files)
//       Could use transparent pixels to encoded/indicate non-file data (if entire tile == transparent: truncate)
//       Add image width option to open dialog (128 default)
void romimg_calc_decoded_size(long int file_size,  app_gfx_data * p_app_gfx, rom_gfx_attrib rom_attrib)
{
    // NOTE: If tile count /size is not an even multiple of IMAGE_WIDTH_DEFAULT
    //       then two conditions arise which need handling
    //
    //       * There isn't enough ROM image data to fill the full image width:
    //
    //         -> The remaining tiles in the image are set to transparent
    //            to indicate they don't contain data (and later shouldn't
    //            be used to encode data)
    //
    //       * There is extra ROM image data which is not an even multiple of tile size
    //
    //         -> Any extra bytes which don't get decoded are stored as
    //            a Gimp metadata parasite attached to the image. Those
    //            get retrieved during export/save and re-appended.

    // First calculate the number of tiles in the image

    // Tiles are NxN pixels. Calculate size factoring in pixel bit-packing.
    int tile_size_bytes;
    int tiles;
    long int surplus_bytes_count;

    tile_size_bytes = ((rom_attrib.TILE_PIXEL_WIDTH * rom_attrib.TILE_PIXEL_HEIGHT)
                             / (8 / rom_attrib.BITS_PER_PIXEL));

    // Calculate number of tiles, as well as number of bytes left over
    tiles = file_size / tile_size_bytes;

    surplus_bytes_count = (long int) (file_size % tile_size_bytes);



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
        // TODO: It would be nice if width was selectable (as a multiple of tile width)
        p_app_gfx->width = 128;
    }



    // * Height is a function of width, tile height and number of tiles
    //   Round up: Integer rounding up: (x + (n-1)) / n
    p_app_gfx->height = (((tiles * rom_attrib.TILE_PIXEL_WIDTH) + (rom_attrib.IMAGE_WIDTH_DEFAULT - 1))
                         / rom_attrib.IMAGE_WIDTH_DEFAULT);

    // Now scale up by the tile height
    p_app_gfx->height *= rom_attrib.TILE_PIXEL_HEIGHT;

    // If there are extra bytes left over then flag them
    // as needing to be stored in metadata as a gimp parasite
    p_app_gfx->surplus_bytes_size = surplus_bytes_count;

    printf("extra bytes %ld\n", p_app_gfx->surplus_bytes_size);
}



int romimg_stash_surplus_bytes(app_gfx_data * p_app_gfx, rom_gfx_data * p_rom_gfx)
{
    if (p_app_gfx->surplus_bytes_size > 0) {

        printf("Saving extra bytes %ld\n", p_app_gfx->surplus_bytes_size);

        // Set aside any surplus bytes at the end which weren't decoded as tiles
        // These will get attached to the gimp image as metadata parasite
        if (NULL == (p_app_gfx->p_surplus_bytes = malloc(p_app_gfx->surplus_bytes_size)) )
            return -1;

        memcpy(p_app_gfx->p_surplus_bytes,
               p_rom_gfx->p_data + (p_rom_gfx->size - p_app_gfx->surplus_bytes_size),
               p_app_gfx->surplus_bytes_size);
    }

    // Return success
    return 0;
}


int romimg_append_surplus_bytes(app_gfx_data * p_app_gfx, rom_gfx_data * p_rom_gfx)
{
    long int        new_size;
    unsigned char * p_new_rom_data = NULL;

    if (p_app_gfx->surplus_bytes_size > 0) {

        printf("Appending extra bytes %ld\n", p_app_gfx->surplus_bytes_size);

        // Allocate a new buffer with the size of the others combined
        new_size = p_rom_gfx->size + p_app_gfx->surplus_bytes_size;

        if (NULL == (p_new_rom_data = malloc(new_size)))
            return -1;

        printf("Size:  rom=%ld, surplus=%ld, newrom=%ld\n", p_rom_gfx->size,
                                                          p_app_gfx->surplus_bytes_size,
                                                          new_size);

        // Copy the contents of the main buffer into the new one
        memcpy(p_new_rom_data,
               p_rom_gfx->p_data,
               p_rom_gfx->size);

        // Append the contents of the surplus buffer into the END of the new one
        memcpy(p_new_rom_data + p_rom_gfx-> size,
               p_app_gfx->p_surplus_bytes,
               p_app_gfx->surplus_bytes_size);

        // Free the old rom buffer
        free(p_rom_gfx->p_data);

        // Swap the new buffer into the struct
        p_rom_gfx->p_data = p_new_rom_data;
        p_rom_gfx->size = new_size;
    }

    // Return success
    return 0;
}



int romimg_insert_color_to_map(unsigned char r, unsigned char g, unsigned char b, app_color_data * p_colorpal)
{
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
    else if (4 == p_colorpal->size) {
        status += romimg_insert_color_to_map(0x00, 0x00, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0x8c, 0x63, 0x21, p_colorpal);
        status += romimg_insert_color_to_map(0xAD, 0xB5, 0x31, p_colorpal);
        status += romimg_insert_color_to_map(0xC6, 0xE7, 0x9C, p_colorpal);
    }
    else if (8 == p_colorpal->size) {
        status += romimg_insert_color_to_map(0x00, 0x00, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0x8c, 0x63, 0x21, p_colorpal);
        status += romimg_insert_color_to_map(0xAD, 0xB5, 0x31, p_colorpal);
        status += romimg_insert_color_to_map(0xC6, 0xE7, 0x9C, p_colorpal);

        status += romimg_insert_color_to_map(0xF8, 0xF8, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0xC0, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0x78, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0x00, 0x00, p_colorpal);
    }
    else if (16 == p_colorpal->size) {
        status += romimg_insert_color_to_map(0x00, 0x00, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0x8c, 0x63, 0x21, p_colorpal);
        status += romimg_insert_color_to_map(0xAD, 0xB5, 0x31, p_colorpal);
        status += romimg_insert_color_to_map(0xC6, 0xE7, 0x9C, p_colorpal);

        status += romimg_insert_color_to_map(0xF8, 0xF8, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0xC0, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0x78, 0x00, p_colorpal);
        status += romimg_insert_color_to_map(0xF8, 0x00, 0x00, p_colorpal);

        status += romimg_insert_color_to_map(0xFA, 0xD3, 0x5A, p_colorpal);
        status += romimg_insert_color_to_map(0x29, 0xA2, 0x29, p_colorpal);
        status += romimg_insert_color_to_map(0x00, 0x78, 0x48, p_colorpal);
        status += romimg_insert_color_to_map(0x00, 0x38, 0x39, p_colorpal);

        status += romimg_insert_color_to_map(0xD8, 0xF0, 0xF8, p_colorpal);
        status += romimg_insert_color_to_map(0xA8, 0xC0, 0xC8, p_colorpal);
        status += romimg_insert_color_to_map(0x90, 0xA8, 0xB0, p_colorpal);
        status += romimg_insert_color_to_map(0x60, 0x78, 0x90, p_colorpal);
    }
    else if (256 == p_colorpal->size) {


    // TODO: 256 colors - use #define
        int c;
        for (c=0; c < 256/16; c++) {
            status += romimg_insert_color_to_map(0x00, 0x00, 0x00, p_colorpal);
            status += romimg_insert_color_to_map(0x8c, 0x63, 0x21, p_colorpal);
            status += romimg_insert_color_to_map(0xAD, 0xB5, 0x31, p_colorpal);
            status += romimg_insert_color_to_map(0xC6, 0xE7, 0x9C, p_colorpal);

            status += romimg_insert_color_to_map(0xF8, 0xF8, 0x00, p_colorpal);
            status += romimg_insert_color_to_map(0xF8, 0xC0, 0x00, p_colorpal);
            status += romimg_insert_color_to_map(0xF8, 0x78, 0x00, p_colorpal);
            status += romimg_insert_color_to_map(0xF8, 0x00, 0x00, p_colorpal);

            status += romimg_insert_color_to_map(0xFA, 0xD3, 0x5A, p_colorpal);
            status += romimg_insert_color_to_map(0x29, 0xA2, 0x29, p_colorpal);
            status += romimg_insert_color_to_map(0x00, 0x78, 0x48, p_colorpal);
            status += romimg_insert_color_to_map(0x00, 0x38, 0x39, p_colorpal);

            status += romimg_insert_color_to_map(0xD8, 0xF0, 0xF8, p_colorpal);
            status += romimg_insert_color_to_map(0xA8, 0xC0, 0xC8, p_colorpal);
            status += romimg_insert_color_to_map(0x90, 0xA8, 0xB0, p_colorpal);
            status += romimg_insert_color_to_map(0x60, 0x78, 0x90, p_colorpal);
        }
    }


    // Check if an error occurred
    if (0 != status)
        return -1;
    else
        // Return success
        return 0;
}
