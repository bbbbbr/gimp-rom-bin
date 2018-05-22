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

#define BITS_PER_PIXEL                      2    // 2 bits per pixel mode
#define PIXELS_PER_WORD                     8    // 1 pixel = 2 bits, 2 bytes per row of 8 pixels
                                                 // In 2bpp mode, one byte stores bitplanes 1-2 for four adjacent pixels, grouped in pars of two bytes

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode
#define DECODED_COLOR_MAP_SIZE              4
#define DECODED_COLOR_MAP_BYTES_PER_PIXEL   3    // R,G,B
#define TILE_PIXEL_WIDTH                    8
#define TILE_PIXEL_HEIGHT                   8


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
// 9. 2BPP Neo Geo Pocket Color
//   Colors Per Tile - 0-3
//   Space Used - 2 bits per pixel.  16 bytes per 8x8 tile.
//
//   Note: This is a tiled, linear bitmap format.
//   Each group represents one byte
//   Format:
//
//   [p4-7 r0: bp*], [p0-3 r0: bp*], [p15-12 r1: bp*], [p11-8 r1: bp*]
//
//   Short Description:
//
//   To simplify, this is merely a mirror image of the 2BPP Virtual Boy format.  Another
//   explanation would be to say that it's in Little Endian instead of Big Endian.
//
//   This is a linear format, so each pixel has it's bitplanes stored consecutively and then
//   moves to the next pixel's bitplanes, stored consecutively.  Probably the easiest example
//   possible of a linear bitplane format.  This format is the same as the Virtual Boy 2BPP
//   format, except that they are congruent mirror images of each other.


static int bin_decode_image_data_ngp_2bpp(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data)
{
    unsigned short pixdata;
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
    if (*file_size < ((width / (8 / BITS_PER_PIXEL)) * height))
        return -1;

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    *file_offset = 0;

    for (int y=0; y < (height / TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (width / TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < TILE_PIXEL_HEIGHT; ty++) {

                // Set up the pointer to the pixel in the destination image buffer
                ptr_image_pixel = (image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * width)
                                              +   (x * TILE_PIXEL_WIDTH));

                // Read two bytes and unpack 8 horizontal pixels
                // First byte is LS Byte, second is MSByte
                pixdata =   (unsigned short) *((unsigned char *)file_data + (*file_offset)++ );
                pixdata |= ((unsigned short) *((unsigned char *)file_data + (*file_offset)++ )) << 8;

                // Read in and unpack 8 horizontal pixels from the two bytes
                for (int b=0;b < PIXELS_PER_WORD; b++) {

                    // Big Endian
                    // b1.0xC0 = pixel.0, b0.0x03 = pixel.7
                    *(ptr_image_pixel++) = (pixdata >> 14) & 0x03;

                    // Upshift source bits to prepare for next pixel bits
                    pixdata <<= 2;

                } // End of tile-row decode loop

            } // End of per-tile decode
        }
    }

    // Return success
    return 0;
}


static int bin_encode_image_data_ngp_2bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char * ptr_output_data)
{
    unsigned char * ptr_image_pixel;
    unsigned char * ptr_output_offset;
    unsigned short  output;

    // Check incoming buffers & vars
    if ((ptr_source_image_data == NULL) ||
        (ptr_output_size       == NULL) ||
        (ptr_output_data       == NULL) ||
        (source_width          == 0) ||
        (source_height         == 0))
        return -1;


    // Make sure there is enough size in the output buffer
    if (*ptr_output_size < (source_width * source_height) / (8 / BITS_PER_PIXEL))
        return -1;

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

                output = 0;

                // Big Endian
                // b1.0xC0 = pixel.0, b0.0x03 = pixel.7

                // Read in and pack 8 horizontal pixels into two bytes
                for (int b=0;b < PIXELS_PER_WORD; b++) {

                    // Upshift output to prepare for next pixel bits
                    output <<= 2;

                    // Store the source pixel bits into output
                    output |= *(ptr_image_pixel++) & 0x03;
                } // End of tile-row encode

                // split u16 output into two bytes and store
                *(ptr_output_offset++) = (unsigned char)(output & 0xFF);
                *(ptr_output_offset++) = (unsigned char)((output >> 8) & 0xFF);

            } // End of per-tile encode
        }
    }

    // Return success
    return 0;
}




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
static int bin_load_color_data_ngp_2bpp(unsigned char * ptr_color_map_data, int color_map_size)
{
    int status = 0;
    unsigned int color_index = 0;

    // Check incoming buffers & vars
    if (ptr_color_map_data == NULL)
        return -1;

    // 2BPP Default
    status += bin_insert_color_to_map(0x00, 0x00, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += bin_insert_color_to_map(0x8c, 0x63, 0x21, ptr_color_map_data, &color_index, color_map_size);
    status += bin_insert_color_to_map(0xAD, 0xB5, 0x31, ptr_color_map_data, &color_index, color_map_size);
    status += bin_insert_color_to_map(0xC6, 0xE7, 0x9C, ptr_color_map_data, &color_index, color_map_size);


    // Check if an error occurred
    if (0 != status)
        return -1;

    // Return success
    return 0;
}


int bin_decode_to_indexed_ngp_2bpp(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size,  int image_mode)
{
    long int file_offset = 0;


    // Set Width & Height
    // Tiles are 8x8 pixels. Calculate size factoring in bit-packing.
    int tiles = file_size / ((TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT)
                             / (8 / BITS_PER_PIXEL));

    // NOTE: If tile count is not an even multiple of IMAGE_WIDTH_DEFAULT
    //       then the width has to be adjusted to a multiple that works,
    //       otherwise file loading will fail
    //       (since the truncated width * heigh * tilesize != file size)
    //
    //        Useful reference:
    //
    //        * Cannot increase image size by rounding up, because the
    //          rounded-to-even-tile-rows image size would not match
    //          then the original image/file size, and it's important to
    //          be able to write back a file that is the same size as the
    //          original (if desired).
    //
    //        * Partial tile line decode would not work since tiles need to remain 8x8
    //          and this would require splitting all the partial tiles across a row instead
    //
    //        * INDEXEDA_IMAGE and using transparent pixels to indicate non-encoded regions
    //          might work. Would have to be careful on re-encode to preserve original file size

    // * Width: if less than 128 pixels wide worth of
    //          tiles, then use cumulative tile width.
    //          Otherwise try to increase it until it reaches
    //          the default of 128 (8 tiles)
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


    // Read the image data
    if (0 != bin_decode_image_data_ngp_2bpp(ptr_file_data,
                                            &file_size,
                                            &file_offset,
                                            *ptr_width,
                                            *ptr_height,
                                            *ptr_ptr_image_data))
        return -1;



    // Allocate the color map buffer
    *color_map_size = DECODED_COLOR_MAP_SIZE;

    // Allocate the color map buffer
    *ptr_ptr_color_map_data = malloc(*color_map_size * DECODED_COLOR_MAP_BYTES_PER_PIXEL);

    // Make sure the alloc succeeded
    if(*ptr_ptr_color_map_data == NULL)
        return -1;

    // Read the color map data
    if (0 != bin_load_color_data_ngp_2bpp(*ptr_ptr_color_map_data, *color_map_size))
        return -1;


    // Return success
    return 0;
}


int bin_encode_to_indexed_ngp_2bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char ** ptr_ptr_output_data, int image_mode)
{
    // Set output file size based on Width, Height and bit packing
    *ptr_output_size = (source_width * source_height) / (8 / BITS_PER_PIXEL);

    *ptr_ptr_output_data = malloc(*ptr_output_size);

    // Did the alloc succeed?
    if(*ptr_ptr_output_data == NULL)
        return -1;


    // Encode the image data
    if (0 != bin_encode_image_data_ngp_2bpp(ptr_source_image_data,
                                            source_width,
                                            source_height,
                                            ptr_output_size,
                                           *ptr_ptr_output_data));
        return -1;


    // Return success
    return 0;
}
