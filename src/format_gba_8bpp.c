/*=======================================================================
              ROM bin load / save plugin for the GIMP
                 Copyright 2018 - X

                 Useful : https://www.rpi.edu/dept/acm/packages/gimp/gimp-1.2.3/plug-ins/common/pcx.c

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

#include "lib_rom_bin.h"
#include "rom_utils.h"
#include "format_gba_8bpp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

#define PIXELS_PER_TILE_ROW          8    // 1 pixel = 8 bits, 8 bytes per row of 8 pixels
                                          // In 8bpp mode, one byte stores bitplanes 1-8 in consecutive pixels


// TODO: move into function?
static const rom_gfx_attrib rom_attrib = {
    256,  // .IMAGE_WIDTH_DEFAULT  // image default pixel width
    8,    // .TILE_PIXEL_WIDTH     // tiles are 8 pixels wide
    8,    // .TILE_PIXEL_HEIGHT    // tiles 8 pixels tall
    8,    // .BITS_PER_PIXEL       // bits per pixel mode

    256,   // .DECODED_NUM_COLORS         // colors in palette
    3     // .DECODED_BYTES_PER_COLOR    // 3 bytes: R,G,B
};


// TODO
// * emu save state palette loading

//
// https://mrclick.zophar.net/TilEd/download/consolegfx.txt
//
//
// -----B. Terminology
//
//      BPP:  Bits per pixel.
//
//      Xth Bitplane: Tells how many bitplanes deep the pixel or row is; the top layer
//           is the first bitplane, the one below that is the second, and so on.
//
//      [rC: bpD]: row number C (0-7 in a 8x8 tile), bitplane number D (starting at 1).
//
//      [pA-B rC: bpD]: pixels A-B (leftmost pixel is 0), row number C (0-7 in a 8x8 tile),
//           bitplane number D (starting at 1. bp* means it's a linear format and stores the
//           bitplanes one after another bp 1 to bp Max.)
//
// -------------------------------------------------
//
//  8BPP GBA
//   Colors Per Tile - 0-255
//   Space Used - 8 bits per pixel.  64 bytes per 8x8 tile.
//
//   Each byte represents one pixel
//   Format:
//
//   Bits 7..0 = p0, 7..0 = p1
//
//   [p0 bp*], [p1 bp*], [p2 bp*], [p3 bp*],
//   [p4 bp*], [p5 bp*], [p6 bp*], [p7 bp*],
//
//   Short Description:
//
//   Bitplanes 1, 2, 3, and 4 are intertwined and stored pixel by pixel.


static int bin_decode_image(rom_gfx_data * p_rom_gfx,
                            app_gfx_data * p_app_gfx)
{
    unsigned char pixdata;
    unsigned char * p_image_pixel;
    long int      rom_offset;
    long int      tile_size_in_bytes;
    unsigned char rom_ended;

    int x,y,ty,b;

    // Check incoming buffers & vars
    if ((p_rom_gfx->p_data  == NULL) ||
        (p_app_gfx->p_data  == NULL) ||
        (p_app_gfx->width   == 0) ||
        (p_app_gfx->height  == 0))
        return -1;


    // Un-bitpack the pixels
    // Decode the image top-to-bottom

    // Set the output buffer at the start
    rom_offset = 0;
    rom_ended = FALSE;
    tile_size_in_bytes = ((rom_attrib.TILE_PIXEL_WIDTH * rom_attrib.TILE_PIXEL_HEIGHT) / (8 / rom_attrib.BITS_PER_PIXEL));

    for (y=0; y < (p_app_gfx->height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (x=0; x < (p_app_gfx->width / rom_attrib.TILE_PIXEL_WIDTH); x++) {

            // Set a flag if there isn't enough rom image data left
            // to read a complete tile. This can happen if the number
            // of tiles and their size isn't an even multiple of the
            // total image width
            //
            // Any extra bytes which don't get decoded are stored as
            // a Gimp metadata parasite attached to the image. Those
            // get retrieved during export/save and re-appended.
            //
            // The remaining tiles in the image are set to transparent
            // to indicate they don't contain data (and later shouldn't
            // be used to encode data)
            if ( (rom_offset + tile_size_in_bytes) > p_rom_gfx->size)
                rom_ended = TRUE;

            // Decode the 8x8 tile top to bottom
            for (ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the destination image buffer
                p_image_pixel = romimg_calc_appimg_offset(x, y, ty, p_app_gfx, rom_attrib);


                // Unpack the 8 horizontal pixels, two at a time
                for (b=0;b < PIXELS_PER_TILE_ROW; b++) {

                    if (!rom_ended) {
                        // Read a byte and unpack two horizontal pixels
                        pixdata = *(p_rom_gfx->p_data + rom_offset++);
                    }

                    // b0.0xFF = pixel.0, b1.0xFF = pixel.1
                    romimg_set_decoded_pixel_and_advance(&p_image_pixel,
                                                         (pixdata),
                                                         rom_ended,
                                                         p_app_gfx);

                } // End of tile-row decode loop
            } // End of per-tile decode
        }
    }

    // Return success
    return 0;
}



static int bin_encode_image(rom_gfx_data * p_rom_gfx,
                            app_gfx_data * p_app_gfx)
{
    unsigned char * p_image_pixel;
    long int      rom_offset;
    unsigned int  transparency_flag;
    unsigned int      empty_tile_count;
    long int tile_size_bytes;

    int x,y,ty,b;

    // Check incoming buffers & vars
    if ((p_app_gfx->p_data == NULL) ||
        (p_rom_gfx->p_data == NULL) ||
        (p_rom_gfx->size   == 0) ||
        (p_app_gfx->width  == 0) ||
        (p_app_gfx->height == 0))
        return -1;


    // Encode the image top-to-bottom

    // Set the output buffer at the start
    rom_offset = 0;
    empty_tile_count = 0;

    for (y=0; y < (p_app_gfx->height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (x=0; x < (p_app_gfx->width / rom_attrib.TILE_PIXEL_WIDTH); x++) {

            // Reset transparency_flag for the upcoming tile
            transparency_flag = 0;

            // Decode the 8x8 tile top to bottom
            for (ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the source image buffer
                p_image_pixel = romimg_calc_appimg_offset(x, y, ty, p_app_gfx, rom_attrib);

                // Read in and pack 8 horizontal pixels into four bytes
                for (b=0;b < PIXELS_PER_TILE_ROW; b++) {

                    // b0.0xFF = pixel.0, b1.0xFF = pixel.1

                    *(p_rom_gfx->p_data + rom_offset)  =  *(p_image_pixel);
                        // Log pixel transparency and advance to next pixel
                        romimg_log_transparent_pixel(p_image_pixel, &transparency_flag, p_app_gfx);
                        p_image_pixel += p_app_gfx->bytes_per_pixel;


                    // Advance to next byte in destination buffer
                    rom_offset++;

                } // End of tile-row encode
            } // End of per-tile encode

            romimg_log_transparent_tiles(transparency_flag, &empty_tile_count, p_app_gfx, rom_attrib);
        }
    }

    // Substract transparent/empty tiles from rom image file size (see above)
    tile_size_bytes = ((rom_attrib.TILE_PIXEL_WIDTH * rom_attrib.TILE_PIXEL_HEIGHT)
                       / (8 / rom_attrib.BITS_PER_PIXEL));


    p_rom_gfx->size -= (empty_tile_count * tile_size_bytes);

    // Return success
    return 0;
}



int bin_decode_gba_8bpp(rom_gfx_data * p_rom_gfx,
                        app_gfx_data * p_app_gfx,
                        app_color_data * p_colorpal)
{
    // Calculate width and height
    romimg_calc_decoded_size(p_rom_gfx->size, p_app_gfx, rom_attrib);


    // Set aside any surplus bytes if present
    if (0 != romimg_stash_surplus_bytes(p_app_gfx,
                                        p_rom_gfx))
        return -1;

    // Allocate the incoming image buffer, abort if it fails
    if (NULL == (p_app_gfx->p_data = malloc(p_app_gfx->width * p_app_gfx->height * p_app_gfx->bytes_per_pixel)) )
        return -1;


    // Read the image data
    if (0 != bin_decode_image(p_rom_gfx,
                              p_app_gfx))
        return -1;


    // Set up info about the color map
    p_colorpal->size            = rom_attrib.DECODED_NUM_COLORS;
    p_colorpal->bytes_per_pixel = rom_attrib.DECODED_BYTES_PER_COLOR;

    // Allocate the color map buffer, abort if it fails
    if (NULL == (p_colorpal->p_data = malloc(p_colorpal->size * p_colorpal->bytes_per_pixel)) )
        return -1;

    // Read the color map data
    if (0 != romimg_load_color_data(p_colorpal))
        return -1;


    // Return success
    return 0;
}


int bin_encode_gba_8bpp(rom_gfx_data * p_rom_gfx,
                        app_gfx_data * p_app_gfx)
{
    // TODO: Warn if number of colors > expected

    // Set output file size based on Width, Height and bit packing
    // Calculate width and height
    p_rom_gfx->size = romimg_calc_encoded_size(p_app_gfx, rom_attrib);

    // Allocate the color map buffer, abort if it fails
    if (NULL == (p_rom_gfx->p_data = malloc(p_rom_gfx->size)) )
        return -1;


    // Encode the image data
    if (0 != bin_encode_image(p_rom_gfx,
                              p_app_gfx))
        return -1;


    // Append any surplus bytes if present
    if (0 != romimg_append_surplus_bytes(p_app_gfx,
                                         p_rom_gfx))
        return -1;

    // Return success
    return 0;
}
