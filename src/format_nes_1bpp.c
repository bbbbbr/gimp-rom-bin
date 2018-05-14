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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

#define IMAGE_WIDTH_DEFAULT                 128

#define NES_BITS_PER_PIXEL_1BPP            1    // 1 bits per pixel mode
#define NES_PIXELS_PER_BYTE_1BPP           8    // 1 pixel = 1 bits, 8 pixels are spread across 1 bytes

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode
#define DECODED_COLOR_MAP_SIZE_1BPP         2
#define DECODED_COLOR_MAP_BYTES_PER_PIXEL   3    // R,G,B //TODO centralize
#define TILE_PIXEL_WIDTH                    8
#define TILE_PIXEL_HEIGHT                   8


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


// TODO: why is file_offset being passed in?
static int bin_decode_image_data_nes_1bpp(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data)
{
    unsigned char pixdata[1];
    unsigned char * ptr_image_pixel;

    // Check incoming buffers & vars
    if ((file_data      == NULL) ||
        (image_data     == NULL) ||
        (width          == 0) ||
        (height         == 0))
        return -1;


    // Make sure there is enough image data
    // then copy it into the image buffer
    // File size is a function of bits per pixel, width and height
    if (*file_size < ((width / (8 / NES_BITS_PER_PIXEL_1BPP)) * height))
        return -1;

printf("Entering decode, passed file size check\n");

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    *file_offset = 0;

    for (int y=0; y < (height / TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (width / TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < TILE_PIXEL_HEIGHT; ty++) {

                // Read two bytes and unpack the 8 horizontal pixels
                pixdata[0] = *((unsigned char *)file_data + (*file_offset));

                // Set up the pointer to the pixel in the destination image buffer
                ptr_image_pixel = (image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * width)
                                              +   (x * TILE_PIXEL_WIDTH));

                // Unpack the 8 horizontal pixels
                for (int b=0;b < NES_PIXELS_PER_BYTE_1BPP; b++) {
                    // pixel[0].n = b.0, pixel[1].n = b.1
                    *ptr_image_pixel = ((pixdata[0] >> 7) & 0x01) | ((pixdata[1] >> 6) & 0x02);

                    // Advance to the next pixel
                    ptr_image_pixel++;

                    // Upshift bits to prepare for the next pixel
                    pixdata[0] <<= 1;
                }

                // Increment the pointer to the next row in the tile
                (*file_offset)++;
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
    if (*ptr_output_size < (source_width * source_height) / (8 / NES_BITS_PER_PIXEL_1BPP))
        return -1;

    // Un-bitpack the pixels
    // Encode the image top-to-bottom

    // Set the output buffer at the start
    ptr_output_offset = ptr_output_data;

    for (int y=0; y < (source_height / TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (source_width / TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the source image buffer
                ptr_image_pixel = (ptr_source_image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * source_width)
                                                         +   (x * TILE_PIXEL_WIDTH));
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



// TODO: Centralize
static int bin_insert_color_to_map(unsigned char r, unsigned char g, unsigned char b, unsigned char * ptr_color_map_data, unsigned int * ptr_color_index, int color_map_size)
{
    // Make sure space is available in the buffer
    if (( (*ptr_color_index) + 2) > (color_map_size * DECODED_COLOR_MAP_BYTES_PER_PIXEL))
        return -1;

    ptr_color_map_data[ (*ptr_color_index)++ ] = r;
    ptr_color_map_data[ (*ptr_color_index)++ ] = g;
    ptr_color_map_data[ (*ptr_color_index)++ ] = b;

    // Return success
    return 0;
}


// TODO: FEATURE: Consider trying to look for .pal file with name that matches .bin file and load it
static int bin_load_color_data_nes_1bpp(unsigned char * ptr_color_map_data, int color_map_size)
{
    int status = 0;
    unsigned int color_index = 0;

    // Check incoming buffers & vars
    if (ptr_color_map_data == NULL)
        return -1;

    // 1BPP Default
    status += bin_insert_color_to_map(0x00, 0x00, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += bin_insert_color_to_map(0xA0, 0xA0, 0xA0, ptr_color_map_data, &color_index, color_map_size);

    // Check if an error occurred
    if (0 != status)
        return -1;

    // Return success
    return 0;
}


int bin_decode_to_indexed_nes_1bpp(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size,  int image_mode)
{
    long int file_offset = 0;

    printf("Entering NES 1bpp decode\n");

    // TODO: move tile, width, height into a function

    // Set Width & Height
    // Tiles are 8x8 pixels. Calculate size factoring in bit-packing.
    int tiles = file_size / ((TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT)
                             / (8 / NES_BITS_PER_PIXEL_1BPP));

    // * Width: if less than 128 pixels wide worth of
    //          tiles, then use cumulative tile width.
    //          Otherwise default to 128 (8 tiles)
    if ((tiles * TILE_PIXEL_WIDTH) >= IMAGE_WIDTH_DEFAULT) {

        // Start at 2 tiles wide
        *ptr_width = 1;

        // Keep increasing the width as long as it results in
        // an even multiple of the tiles *and* it's <= 128 pixels wide (the optimal width)
        while ( ((tiles % ((*ptr_width) * 2)) == 0) &&
             ((*ptr_width) * 2 * TILE_PIXEL_WIDTH <= IMAGE_WIDTH_DEFAULT) ) {

            // Use the doubled width if it's still resulting in
            // an even multiple of the tiles
            (*ptr_width) *= 2;
        }

        // Scale the width value up to tile-pixel-size
        *ptr_width = (*ptr_width * TILE_PIXEL_WIDTH);
    }

    else {
        *ptr_width = (tiles * TILE_PIXEL_WIDTH);
    }

    // * Height is a function of width, tile height and number of tiles
    //   Round up: Integer rounding up: (x + (n-1)) / n
    *ptr_height = (((tiles * TILE_PIXEL_WIDTH) + (IMAGE_WIDTH_DEFAULT - 1))
                   / IMAGE_WIDTH_DEFAULT);
    // Now scale up by the tile height
    *ptr_height *= TILE_PIXEL_HEIGHT;

    // Allocate the incoming image buffer
    *ptr_ptr_image_data = malloc(*ptr_width * *ptr_height);

    // Make sure the alloc succeeded
    if(*ptr_ptr_image_data == NULL)
        return -1;


printf("w=%d, h=%d, t=%d, fs=%ld, ibs=%d \n",*ptr_width, *ptr_height, tiles, file_size, *ptr_width * *ptr_height);

    // Read the image data
    if (0 != bin_decode_image_data_nes_1bpp(ptr_file_data,
                                            &file_size,
                                            &file_offset,
                                            *ptr_width,
                                            *ptr_height,
                                            *ptr_ptr_image_data))
        return -1;


    // Allocate the color map buffer
    *color_map_size = DECODED_COLOR_MAP_SIZE_1BPP;

    // Allocate the color map buffer
    *ptr_ptr_color_map_data = malloc(*color_map_size * DECODED_COLOR_MAP_BYTES_PER_PIXEL);

    // Make sure the alloc succeeded
    if(*ptr_ptr_color_map_data == NULL)
        return -1;

printf("colors=%d\n", *color_map_size);

    // Read the color map data
    if (0 != bin_load_color_data_nes_1bpp(*ptr_ptr_color_map_data, *color_map_size))
        return -1;


    // Return success
    return 0;
}


int bin_encode_to_indexed_nes_1bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char ** ptr_ptr_output_data, int image_mode)
{
    // TODO: Warn if number of colors > 2

    // Set output file size based on Width, Height and bit packing
    *ptr_output_size = (source_width * source_height) / (8 / NES_BITS_PER_PIXEL_1BPP);

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
