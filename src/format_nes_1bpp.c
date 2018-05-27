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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

#define NES_PIXELS_PER_BYTE_1BPP           8    // 1 pixel = 1 bits, 8 pixels are spread across 1 bytes

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode


// TODO: move into function?
static const rom_gfx_attrib rom_attrib = {
    128,  // .IMAGE_WIDTH_DEFAULT  // image defaults to 128 pixels wide
    8,    // .TILE_PIXEL_WIDTH     // tiles are 8 pixels wide
    8,    // .TILE_PIXEL_HEIGHT    // tiles 8 pixels tall
    1,    // .BITS_PER_PIXEL       // 1 bit per pixel mode

    2,    // .DECODED_NUM_COLORS         // 2 colors in pallete
    3     // .DECODED_BYTES_PER_PIXEL    // 3 bytes: R,G,B  // TODO: CENTRALIZE?
};


//
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
// 2.  1BPP NES/Monochrome
//
//   Colors Per Tile - 0-1
//   Space Used - 1 bit per pixel.  8 bytes per 8x8 tile.
//
//   Note: This is a tiled, linear bitmap format.
//   Each pair represents one byte
//   Format:
//
//   [r0, bp1], [r1, bp1], [r2, bp1], [r3, bp1], [r4, bp1], [r5, bp1], [r6, bp1], [r7, bp1]
//
//   Short Description:
//
//   The first bitplane is stored row by row, very simple.
//
//


// TODO: Pass in rom_attrib instead of glocal static?
static int bin_decode_image_data_nes_1bpp(void * file_data, long int * file_size, image_gfx_data * p_gfx)
{
    unsigned char pixdata[1];
    unsigned char * p_image_pixel;
    long int      file_offset = 0;

    // Check incoming buffers & vars
    if ((file_data      == NULL) ||
        (p_gfx->p_data  == NULL) ||
        (p_gfx->width   == 0) ||
        (p_gfx->height  == 0))
        return -1;


    // Make sure there is enough image data
    // then copy it into the image buffer
    // File size is a function of bits per pixel, width and height
    if (*file_size < ((p_gfx->width / (8 / rom_attrib.BITS_PER_PIXEL)) * p_gfx->height))
        return -1;

printf("Entering decode, passed file size check\n");

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    file_offset = 0;

    for (int y=0; y < (p_gfx->height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (p_gfx->width / rom_attrib.TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Read two bytes and unpack the 8 horizontal pixels
                pixdata[0] = *((unsigned char *)file_data + file_offset);

                // Set up the pointer to the pixel in the destination image buffer
                p_image_pixel = (p_gfx->p_data + (((y * rom_attrib.TILE_PIXEL_HEIGHT) + ty) * p_gfx->width)
                                               +   (x * rom_attrib.TILE_PIXEL_WIDTH));

                // Unpack the 8 horizontal pixels
                for (int b=0;b < NES_PIXELS_PER_BYTE_1BPP; b++) {
                    // pixel[0].n = b.0, pixel[1].n = b.1
                    *p_image_pixel = ((pixdata[0] >> 7) & 0x01) | ((pixdata[1] >> 6) & 0x02);

                    // Advance to the next pixel
                    p_image_pixel++;

                    // Upshift bits to prepare for the next pixel
                    pixdata[0] <<= 1;
                }

                // Increment the pointer to the next row in the tile
                file_offset++;
            } // End of per-tile decode
        }
    }

    // Return success
    return 0;
}



static int bin_encode_image_data_nes_1bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char * ptr_output_data)
{
    unsigned char pixdata[1];
    unsigned char * ptr_image_pixel;
    unsigned char * ptr_output_offset;

    // Check incoming buffers & vars
    if ((ptr_source_image_data == NULL) ||
        (ptr_output_size       == NULL) ||
        (ptr_output_data       == NULL) ||
        (source_width          == 0) ||
        (source_height         == 0))
        return -1;


    // Make sure there is enough size in the output buffer
    if (*ptr_output_size < (source_width * source_height) / (8 / rom_attrib.BITS_PER_PIXEL))
        return -1;

    // Un-bitpack the pixels
    // Encode the image top-to-bottom

    // Set the output buffer at the start
    ptr_output_offset = ptr_output_data;

    for (int y=0; y < (source_height / rom_attrib.TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (source_width / rom_attrib.TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < rom_attrib.TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the source image buffer
                ptr_image_pixel = (ptr_source_image_data + (((y * rom_attrib.TILE_PIXEL_HEIGHT) + ty) * source_width)
                                                         +   (x * rom_attrib.TILE_PIXEL_WIDTH));
                pixdata[0] = 0;
                pixdata[1] = 0;

                // Read in and pack 8 horizontal pixels into two bytes
                for (int b=0;b < NES_PIXELS_PER_BYTE_1BPP; b++) {

                    // b0.MSbit = pixel.1, b1.MSbit = pixel.0
                    pixdata[0] = (pixdata[0] << 1) |  ( (*ptr_image_pixel) & 0x01);

                    // Advance to next pixel
                    ptr_image_pixel++;
                }

                // Save the two packed bytes. LS Bits then MS Bits (MS Bits are 8 bytes later)
                *(ptr_output_offset) = pixdata[0];

                // Advance to next row in the tile
                ptr_output_offset++;
            }
        }
    }

    // Return success
    return 0;
}





int bin_decode_to_indexed_nes_1bpp(void * p_file_data, long int file_size, image_gfx_data * p_gfx, image_color_data * p_colorpal)
{
    printf("bin_decode_to_indexed_nes_1bpp\n");

    // Calculate width and height
    romimg_calc_image_size(file_size, p_gfx, rom_attrib);


    // Allocate the incoming image buffer, abort if it fails
    if (NULL == (p_gfx->p_data = malloc(p_gfx->width * p_gfx->height)) )
        return -1;


    // Read the image data
    if (0 != bin_decode_image_data_nes_1bpp(p_file_data,
                                            &file_size,
                                            p_gfx))
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


int bin_encode_to_indexed_nes_1bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char ** ptr_ptr_output_data, int image_mode)
{
    // TODO: Warn if number of colors > 2

    // Set output file size based on Width, Height and bit packing
    *ptr_output_size = (source_width * source_height) / (8 / rom_attrib.BITS_PER_PIXEL);

    *ptr_ptr_output_data = malloc(*ptr_output_size);

    // Did the alloc succeed?
    if(*ptr_ptr_output_data == NULL)
        return -1;


    // Encode the image data
    if (0 != bin_encode_image_data_nes_1bpp(ptr_source_image_data,
                                            source_width,
                                            source_height,
                                            ptr_output_size,
                                           *ptr_ptr_output_data));
        return -1;


    // Return success
    return 0;
}
