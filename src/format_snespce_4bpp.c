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

#define SNES_BITS_PER_PIXEL_4BPP            4    // 4 bits per pixel mode
#define SNES_BYTE_GAP_LOHI_PLANES_4BPP      16   // In 4bpp mode there is a 16 byte offset between the pairs of Low and High bytes
#define SNES_BYTE_ROW_INCREMENT_4BPP        2    // In 4bpp mode, two adjacent bytes form bitplanes 1 & 2 (out of 1,2,3,4) of a row, this is the increment amount for decoding
#define SNES_PIXELS_PER_DWORD_4BPP          8    // 1 pixel = 4 bits, 8 pixels are in 2 + 2 consecutive bytes among 18 bytes ([0],[1] ~14 bytes~ [3],[4])

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode
#define DECODED_COLOR_MAP_SIZE_4BPP         16
#define DECODED_COLOR_MAP_BYTES_PER_PIXEL   3    // R,G,B
#define TILE_PIXEL_WIDTH                    8
#define TILE_PIXEL_HEIGHT                   8


// TODO
// * ZSNES save state palette loading
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
// 6. 4BPP SNES/PC Engine
//  Colors Per Tile - 0-15
//  Space Used - 4 bits per pixel.  32 bytes for a 8x8 tile.
//
//  Note: This is a tiled, planar bitmap format.
//  Each pair represents one byte
//  Format:
//
//  **** Starred bytes form the first tile row, note how MS bits are in second 16 bytes
//
//  *[r0, bp1]*, *[r0, bp2]*, [r1, bp1], [r1, bp2], [r2, bp1], [r2, bp2], [r3, bp1], [r3, bp2]
//   [r4, bp1],   [r4, bp2],  [r5, bp1], [r5, bp2], [r6, bp1], [r6, bp2], [r7, bp1], [r7, bp2]
//  *[r0, bp3]*, *[r0, bp4]*, [r1, bp3], [r1, bp4], [r2, bp3], [r2, bp4], [r3, bp3], [r3, bp4]
//   [r4, bp3],   [r4, bp4],  [r5, bp3], [r5, bp4], [r6, bp3], [r6, bp4], [r7, bp3], [r7, bp4]
//
//  Short Description:
//
//  Bitplanes 1 and 2 are stored first, intertwined row by row.  Then bitplanes 3 and 4
//  are stored, intertwined row by row.


static int snesbin_decode_image_data_4bpp(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data)
{
    unsigned char pixdata[4];
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
    if (*file_size < ((width / (8 / SNES_BITS_PER_PIXEL_4BPP)) * height))
        return -1;

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    *file_offset = 0;

    for (int y=0; y < (height / TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (width / TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < TILE_PIXEL_HEIGHT; ty++) {
                // Read four bytes and unpack the 8 horizontal pixels

                // First the LS bits
                pixdata[0] = *((unsigned char *)file_data + (*file_offset));
                pixdata[1] = *((unsigned char *)file_data + (*file_offset) + 1);

                // Then the MS bits (they are in the next 16 bytes of data
                pixdata[2] = *((unsigned char *)file_data + (*file_offset) + SNES_BYTE_GAP_LOHI_PLANES_4BPP);
                pixdata[3] = *((unsigned char *)file_data + (*file_offset) + SNES_BYTE_GAP_LOHI_PLANES_4BPP + 1);

                // Set up the pointer to the pixel in the destination image buffer
                ptr_image_pixel = (image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * width)
                                              +   (x * TILE_PIXEL_WIDTH));

                // Unpack the 8 horizontal pixels
                for (int b=0;b < SNES_PIXELS_PER_DWORD_4BPP; b++) {

                    // b0.MSbit = pixel.1, b1.MSbit = pixel.0
                    *ptr_image_pixel = ((pixdata[0] >> 7) & 0x01) |
                                       ((pixdata[1] >> 6) & 0x02) |
                                       ((pixdata[2] >> 5) & 0x04) |
                                       ((pixdata[3] >> 4) & 0x08);

                    // Advance to the next pixel
                    ptr_image_pixel++;

                    // Upshift bits to prepare for the next pixel
                    pixdata[0] <<= 1;
                    pixdata[1] <<= 1;
                    pixdata[2] <<= 1;
                    pixdata[3] <<= 1;
                } // End of tile-row decode loop

                // Increment the pointer to the next row in the tile
                (*file_offset) += SNES_BYTE_ROW_INCREMENT_4BPP;
            } // End of per-tile decode

            // Now advance to the start of the next tile, which is 16 bytes further
            (*file_offset) += SNES_BYTE_GAP_LOHI_PLANES_4BPP;
        }
    }

    // Return success
    return 0;
}


static int snesbin_encode_image_data_4bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char * ptr_output_data)
{
    unsigned char pixdata[4];
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
    if (*ptr_output_size < (source_width * source_height) / (8 / SNES_BITS_PER_PIXEL_4BPP))
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
                pixdata[0] = 0;
                pixdata[1] = 0;
                pixdata[2] = 0;
                pixdata[3] = 0;

                // Read in and pack 8 horizontal pixels into four bytes
                // The bytes are in pairs, the second pair is 14 bytes later
                for (int b=0;b < SNES_PIXELS_PER_DWORD_4BPP; b++) {

                    // dest[0].0 = source.0 ... dest[3].0 = source.3
                    pixdata[0] = (pixdata[0] << 1) |  ( (*ptr_image_pixel) & 0x01);
                    pixdata[1] = (pixdata[1] << 1) | (( (*ptr_image_pixel) & 0x02) >> 1);
                    pixdata[2] = (pixdata[2] << 1) | (( (*ptr_image_pixel) & 0x04) >> 2);
                    pixdata[3] = (pixdata[3] << 1) | (( (*ptr_image_pixel) & 0x08) >> 3);

                    // Advance to next pixel
                    ptr_image_pixel++;
                } // End of tile-row encode


                // Save the LSBits two packed bytes
                *(ptr_output_offset)     = pixdata[0];
                *(ptr_output_offset + 1) = pixdata[1];

                // Then the MS bits (they are in the next 16 bytes of data)
                *(ptr_output_offset + SNES_BYTE_GAP_LOHI_PLANES_4BPP) = pixdata[2];
                *(ptr_output_offset + SNES_BYTE_GAP_LOHI_PLANES_4BPP + 1) = pixdata[3];

                // Advance to next row in tile
                ptr_output_offset += SNES_BYTE_ROW_INCREMENT_4BPP;

            } // End of per-tile encode

            // Now advance to the start of the next tile
            // The pointer is in the middle of the current tile (16 of 32 bytes),
            // so increment by another 16 bytes to get to the start of the next tile
            ptr_output_offset += SNES_BYTE_GAP_LOHI_PLANES_4BPP;
        }
    }

    // Return success
    return 0;
}




static int snesbin_insert_color_to_map(unsigned char r, unsigned char g, unsigned char b, unsigned char * ptr_color_map_data, unsigned int * ptr_color_index, int color_map_size)
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
static int snesbin_load_color_data_4bpp(unsigned char * ptr_color_map_data, int color_map_size)
{
    int status = 0;
    unsigned int color_index = 0;

    // Check incoming buffers & vars
    if (ptr_color_map_data == NULL)
        return -1;

    status += snesbin_insert_color_to_map(0x00, 0x00, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x8c, 0x63, 0x21, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xAD, 0xB5, 0x31, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xC6, 0xE7, 0x9C, ptr_color_map_data, &color_index, color_map_size);

    status += snesbin_insert_color_to_map(0xF8, 0xF8, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xF8, 0xC0, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xF8, 0x78, 0x00, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xF8, 0x00, 0x00, ptr_color_map_data, &color_index, color_map_size);

    status += snesbin_insert_color_to_map(0xFA, 0xD3, 0x5A, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x29, 0xA2, 0x29, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x00, 0x78, 0x48, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x00, 0x38, 0x39 ,ptr_color_map_data, &color_index, color_map_size);

    status += snesbin_insert_color_to_map(0xD8, 0xF0, 0xF8, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0xA8, 0xC0, 0xC8, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x90, 0xA8, 0xB0, ptr_color_map_data, &color_index, color_map_size);
    status += snesbin_insert_color_to_map(0x60, 0x78, 0x90, ptr_color_map_data, &color_index, color_map_size);


    // Check if an error occurred
    if (0 != status)
        return -1;

    // Return success
    return 0;
}


int snesbin_decode_to_indexed_snes_4bpp(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size,  int image_mode)
{
    long int file_offset = 0;


    // Set Width & Height
    // Tiles are 8x8 pixels. Calculate size factoring in bit-packing.
    int tiles = file_size / ((TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT)
                             / (8 / SNES_BITS_PER_PIXEL_4BPP));

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
    if (0 != snesbin_decode_image_data_4bpp(ptr_file_data,
                                            &file_size,
                                            &file_offset,
                                            *ptr_width,
                                            *ptr_height,
                                            *ptr_ptr_image_data))
        return -1;



    // Allocate the color map buffer
    *color_map_size = DECODED_COLOR_MAP_SIZE_4BPP;

    // Allocate the color map buffer
    *ptr_ptr_color_map_data = malloc(*color_map_size * DECODED_COLOR_MAP_BYTES_PER_PIXEL);

    // Make sure the alloc succeeded
    if(*ptr_ptr_color_map_data == NULL)
        return -1;

    // Read the color map data
    if (0 != snesbin_load_color_data_4bpp(*ptr_ptr_color_map_data, *color_map_size))
        return -1;


    // Return success
    return 0;
}


int snesbin_encode_to_indexed_snes_4bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char ** ptr_ptr_output_data, int image_mode)
{
    // Set output file size based on Width, Height and bit packing
    *ptr_output_size = (source_width * source_height) / (8 / SNES_BITS_PER_PIXEL_4BPP);

    *ptr_ptr_output_data = malloc(*ptr_output_size);

    // Did the alloc succeed?
    if(*ptr_ptr_output_data == NULL)
        return -1;


    // Encode the image data
    if (0 != snesbin_encode_image_data_4bpp(ptr_source_image_data,
                                            source_width,
                                            source_height,
                                            ptr_output_size,
                                           *ptr_ptr_output_data));
        return -1;


    // Return success
    return 0;
}
