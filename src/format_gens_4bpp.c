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

// TODO: support color 0 = transparent for genesis tiles

#include "lib_rom_bin.h"
#include "rom_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

#define GENS_BYTE_ROW_INCREMENT_4BPP        2    // In 4bpp mode, one byte stores bitplanes 1-4 for two adjacent pixels
#define GENS_PIXEL_PAIRS_PER_DWORD_4BPP     4    // 1 pixel = 4 bits, 4 bytes per row of 8 pixels

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode

// TODO: move into function?
static const rom_gfx_attrib rom_attrib = {
    128,  // .IMAGE_WIDTH_DEFAULT  // image defaults to 128 pixels wide
    8,    // .TILE_PIXEL_WIDTH     // tiles are 8 pixels wide
    8,    // .TILE_PIXEL_HEIGHT    // tiles 8 pixels tall
    4,    // .BITS_PER_PIXEL       // bits per pixel mode

    16,   // .DECODED_NUM_COLORS         // colors in pallete
    3     // .DECODED_BYTES_PER_PIXEL    // 3 bytes: R,G,B
};


// TODO
// * emu save state palette loading
// * Code consolidation and modularization

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
// 12. 4BPP Genesis/x68k
//   Colors Per Tile - 0-15
//   Space Used - 4 bits per pixel.  32 bytes per 8x8 tile.
//
//   Note: This is a tiled, big Endian, linear format.
//   Each pair represents one byte
//   Format:
//
//   Bits 7..4 = p0, 3..0 = p1 / etc
//
//   [p0-1 r0: bp*], [p2-3 r0: bp*], [p4-5 r0: bp*], [p6-7 r0: bp*]
//   [p0-1 r1: bp*], [p2-3 r1: bp*], [p4-5 r1: bp*], [p6-7 r1: bp*]
//   [p0-1 r2: bp*], [p2-3 r2: bp*], [p4-5 r2: bp*], [p6-7 r2: bp*]
//   [p0-1 r3: bp*], [p2-3 r3: bp*], [p4-5 r3: bp*], [p6-7 r3: bp*]
//   [p0-1 r4: bp*], [p2-3 r4: bp*], [p4-5 r4: bp*], [p6-7 r4: bp*]
//   [p0-1 r5: bp*], [p2-3 r5: bp*], [p4-5 r5: bp*], [p6-7 r5: bp*]
//   [p0-1 r6: bp*], [p2-3 r6: bp*], [p4-5 r6: bp*], [p6-7 r6: bp*]
//   [p0-1 r7: bp*], [p2-3 r7: bp*], [p4-5 r7: bp*], [p6-7 r7: bp*]
//
//   Short Description:
//
//   Bitplanes 1, 2, 3, and 4 are intertwined and stored pixel by pixel.


static int bin_decode_image(rom_gfx_data * p_rom_gfx,
                            app_gfx_data * p_app_gfx)
{
    unsigned char pixdata;
    unsigned char * p_image_pixel;
    long int      offset;

    // Check incoming buffers & vars
    if ((p_rom_gfx->p_data  == NULL) ||
        (p_app_gfx->p_data  == NULL) ||
        (p_app_gfx->width   == 0) ||
        (p_app_gfx->height  == 0))
        return -1;


    // Make sure there is enough image data
    // then copy it into the image buffer
    // File size is a function of bits per pixel, width and height
    if (p_rom_gfx->size < ((p_app_gfx->width / (8 / rom_attrib.BITS_PER_PIXEL)) * p_app_gfx->height))
        return -1;

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    offset = 0;

    for (int y=0; y < (p_app_gfx->height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (p_app_gfx->width / rom_attrib.TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the destination image buffer
                p_image_pixel = (p_app_gfx->p_data + (((y * rom_attrib.TILE_PIXEL_HEIGHT) + ty) * p_app_gfx->width)
                                                   +   (x * rom_attrib.TILE_PIXEL_WIDTH));

                // Unpack the 8 horizontal pixels, two at a time
                for (int b=0;b < GENS_PIXEL_PAIRS_PER_DWORD_4BPP; b++) {

                    // Read a byte and unpack two horizontal pixels
                    pixdata = *(p_rom_gfx->p_data + offset++);

                    // Big Endian
                    // b0.0xF0 = pixel.0, b0.0x0F = pixel.1
                    *(p_image_pixel++) = (pixdata >> 4) & 0x0F;
                    *(p_image_pixel++) = (pixdata & 0x0F);

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
    long int      offset;

    // Check incoming buffers & vars
    if ((p_app_gfx->p_data == NULL) ||
        (p_rom_gfx->p_data == NULL) ||
        (p_rom_gfx->size   == 0) ||
        (p_app_gfx->width  == 0) ||
        (p_app_gfx->height == 0))
        return -1;


    // Make sure there is enough size in the output buffer
    if (p_rom_gfx->size < (p_app_gfx->width * p_app_gfx->height) / (8 / rom_attrib.BITS_PER_PIXEL))
        return -1;

    // Encode the image top-to-bottom

    // Set the output buffer at the start
    offset = 0;

    for (int y=0; y < (p_app_gfx->height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (p_app_gfx->width / rom_attrib.TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the source image buffer
                p_image_pixel = (p_app_gfx->p_data + (((y * rom_attrib.TILE_PIXEL_HEIGHT) + ty) * p_app_gfx->width)
                                                         +   (x * rom_attrib.TILE_PIXEL_WIDTH));
                // Read in and pack 8 horizontal pixels into four bytes
                // The bytes are in pairs, the second pair is 14 bytes later
                for (int b=0;b < GENS_PIXEL_PAIRS_PER_DWORD_4BPP; b++) {

                    // dest[0].0 = source.0 ... dest[3].0 = source.3
                    *(p_rom_gfx->p_data + offset)  = (*(p_image_pixel++) << 4) & 0xF0;
                    *(p_rom_gfx->p_data + offset) |=  *(p_image_pixel++) & 0x0F;

                    // Advance to next byte in destination buffer
                    offset++;

                } // End of tile-row encode
            } // End of per-tile encode
        }
    }

    // Return success
    return 0;
}



int bin_decode_gens_4bpp(rom_gfx_data * p_rom_gfx,
                         app_gfx_data * p_app_gfx,
                         app_color_data * p_colorpal)
{
    // Calculate width and height
    romimg_calc_image_size(p_rom_gfx->size, p_app_gfx, rom_attrib);

    // Allocate the incoming image buffer, abort if it fails
    if (NULL == (p_app_gfx->p_data = malloc(p_app_gfx->width * p_app_gfx->height)) )
        return -1;


    // Read the image data
    if (0 != bin_decode_image(p_rom_gfx,
                              p_app_gfx))
        return -1;


    // Set up info about the color map
    p_colorpal->size            = rom_attrib.DECODED_NUM_COLORS;
    p_colorpal->bytes_per_pixel = rom_attrib.DECODED_BYTES_PER_PIXEL;

    // Allocate the color map buffer, abort if it fails
    if (NULL == (p_colorpal->p_data = malloc(p_colorpal->size * p_colorpal->bytes_per_pixel)) )
        return -1;

    // Read the color map data
    if (0 != romimg_load_color_data(p_colorpal))
        return -1;


    // Return success
    return 0;
}


int bin_encode_gens_4bpp(rom_gfx_data * p_rom_gfx,
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
                              p_app_gfx));
        return -1;


    // Return success
    return 0;
}
