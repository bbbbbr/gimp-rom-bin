/*=======================================================================
              SNES bin load / save plugin for the GIMP
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

#include "lib_snesbin.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>

#define IMAGE_WIDTH_DEFAULT                 128

#define SNES_BITS_PER_PIXEL_2BPP            2    // TODO, support 2 & 4 BPP modes
#define SNES_BITS_PER_PIXEL_4BPP            4    // TODO, support 2 & 4 BPP modes
#define SNES_PIXELS_PER_BYTE_2BPP  8    // 1 pixel = 2 bits, 8 pixels are spread across 2 consecutive bytes (lo...hi byte)
#define SNES_PIXELS_PER_BYTE_4BPP  4    // 1 pixel = 4 bits, 8 pixels are spread across 4 consecutive bytes (lo...hi byte)

#define DECODED_IMAGE_BYTES_PER_PIXEL       1    // 1 byte per pixel in indexed color mode
#define DECODED_COLOR_MAP_SIZE              4    // TODO, support 2 & 4 BPP modes
#define DECODED_COLOR_MAP_BYTES_PER_PIXEL   3    // R,G,B
#define TILE_PIXEL_WIDTH                    8
#define TILE_PIXEL_HEIGHT                   8


int snesbin_decode_image_data_2bpp(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data);
int snesbin_encode_image_data_2bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char * ptr_output_data);
int snesbin_decode_read_color_data(unsigned char * color_map_data);

int snesbin_decode_image_data_2bpp(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data)
{
    unsigned char pixdata[2];
    unsigned char * image_pixel;

    // Check incoming buffers & vars
    if ((file_data      == NULL) ||
        (image_data     == NULL) ||
        (width          == 0) ||
        (height         == 0))
        return -1;


    // Make sure there is enough image data
    // then copy it into the image buffer
    // File size is a function of bits per pixel, width and height
    if (*file_size < ((width / (8 / SNES_BITS_PER_PIXEL_2BPP)) * height))
        return -1;

    // 2BPP SNES/GBA:
    //
    // * Packed 8x8 Tiles - serial encoded
    // * 1 Tile row = 2 bytes (byte_lo/0, byte_hi/1)
    // * Packed Bits: 876543210, 876543210
    // *              ---------  ---------
    // * Pixel #    : 012345678  012345678
    // * Pixel l/h  : LLLLLLLLL  HHHHHHHHH
    // * So, Pixel 0 = ((byte0 >> 8) & 0x01) | ((byte1 >> 7) & 0x20)
    // TODO : verify lo/hi byte order

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    *file_offset = 0;

    for (int y=0; y < (height / TILE_PIXEL_HEIGHT); y++) {
        // Decode left-to-right
        for (int x=0; x < (width / TILE_PIXEL_WIDTH); x++) {
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < TILE_PIXEL_HEIGHT; ty++) {
                // Read two bytes and unpack the 8 horizontal pixels
                pixdata[0] = *((unsigned char *)file_data + (*file_offset)++);
                pixdata[1] = *((unsigned char *)file_data + (*file_offset)++);

                // Set up the pointer to the pixel in the destination image buffer
                // TODO: optimize
                image_pixel = (image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * width)
                                          +   (x * TILE_PIXEL_WIDTH));

                // Unpack the 8 horizontal pixels
                for (int b=0;b < SNES_PIXELS_PER_BYTE_2BPP; b++) {

                    // b0.MSbit = pixel.1, b1.MSbit = pixel.0
                    // TODO: Is the bit order swapped here? should 0 be LSBit?
                    *image_pixel++ = ((pixdata[0] >> 7) & 0x01) | ((pixdata[1] >> 6) & 0x02);

                    // Upshift bits to prepare for the next pixel
                    pixdata[0] = pixdata[0] << 1;
                    pixdata[1] = pixdata[1] << 1;
                }
            }
        }
    }

    // Return success
    return 0;
}



// TODO: Rename to Set Color Data (since no palette is built into SNES bin files)
// TODO: FEATURE: Consider trying to look for .pal file with name that matches .bin file and load it
int snesbin_decode_read_color_data(unsigned char * color_map_data)
{
    int colorindex = 0;

    // Check incoming buffers & vars
    if (color_map_data == NULL)
        return -1;

    // TODO : check buffer size!

	//TODO: Improve palette
    color_map_data[colorindex++] = 0;
    color_map_data[colorindex++] = 0;
    color_map_data[colorindex++] = 0;

    color_map_data[colorindex++] = 0x8c;
    color_map_data[colorindex++] = 0x63;
    color_map_data[colorindex++] = 0x21;

    color_map_data[colorindex++] = 0xAD;
    color_map_data[colorindex++] = 0xB5;
    color_map_data[colorindex++] = 0x31;

    color_map_data[colorindex++] = 0xC6;
    color_map_data[colorindex++] = 0xE7;
    color_map_data[colorindex++] = 0x9C;


    // Return success
    return 0;
}


int snesbin_decode_to_indexed(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size)
{
    long int file_offset = 0;

    // Set Width & Height
    // Tiles are 8x8 pixels. Calculate size factoring in bit-packing.
    int tiles = file_size / ((TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT)
                             / (8 / SNES_BITS_PER_PIXEL_2BPP));

    // * Width: if less than 128 pixels wide worth of
    //          tiles, then use cumulative tile width.
    //          Otherwise default to 128 (8 tiles)
    if ((tiles * TILE_PIXEL_WIDTH) >= IMAGE_WIDTH_DEFAULT) {
        *ptr_width = IMAGE_WIDTH_DEFAULT;
    } else {
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
    if (0 != snesbin_decode_image_data_2bpp(ptr_file_data,
                                            &file_size,
                                            &file_offset,
                                            *ptr_width,
                                            *ptr_height,
                                            *ptr_ptr_image_data))
        return -1;


    // Allocate the color map buffer
    *color_map_size = DECODED_COLOR_MAP_SIZE;
    *ptr_ptr_color_map_data = malloc(*color_map_size * DECODED_COLOR_MAP_BYTES_PER_PIXEL);

    // Make sure the alloc succeeded
    if(*ptr_ptr_color_map_data == NULL)
        return -1;

    // Read the color map data
    if (0 != snesbin_decode_read_color_data(*ptr_ptr_color_map_data))
        return -1;


    // Return success
    return 0;
}





int snesbin_encode_image_data_2bpp(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char * ptr_output_data)
{
    unsigned char pixdata[2];
    unsigned char * image_pixel;
    unsigned char * ptr_output_offset;

    // Check incoming buffers & vars
    if ((ptr_source_image_data == NULL) ||
        (ptr_output_size       == NULL) ||
        (ptr_output_data       == NULL) ||
        (source_width          == 0) ||
        (source_height         == 0))
        return -1;


    // Make sure there is enough size in the output buffer
    if (*ptr_output_size < (source_width * source_height) / (8 / SNES_BITS_PER_PIXEL_2BPP))
        return -1;

    // 2BPP SNES/GBA:
    //
    // * Packed 8x8 Tiles - serial encoded
    // * 1 Tile row = 2 bytes (byte_lo/0, byte_hi/1)
    // * Packed Bits: 876543210, 876543210
    // *              ---------  ---------
    // * Pixel #    : 012345678  012345678
    // * Pixel l/h  : LLLLLLLLL  HHHHHHHHH
    // * So, Pixel 0 = ((byte0 >> 8) & 0x01) | ((byte1 >> 7) & 0x20)
    // TODO : verify lo/hi byte order

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
                image_pixel = (ptr_source_image_data + (((y * TILE_PIXEL_HEIGHT) + ty) * source_width)
                                                     +   (x * TILE_PIXEL_WIDTH));
                pixdata[0] = 0;
                pixdata[1] = 0;

                // Read in and pack 8 horizontal pixels into two bytes
                for (int b=0;b < SNES_PIXELS_PER_BYTE_2BPP; b++) {

                    // b0.MSbit = pixel.1, b1.MSbit = pixel.0
                    // TODO: Is the bit order swapped here? should 0 be LSBit?

                    pixdata[0] = (pixdata[0] << 1) |  (*image_pixel & 0x01);
                    pixdata[1] = (pixdata[1] << 1) | ((*image_pixel & 0x02) >> 1);

                    // Advance to next pixel
                    *image_pixel++;
                }

                // Save the two packed bytes
                *ptr_output_offset++ = pixdata[0];
                *ptr_output_offset++ = pixdata[1];
            }
        }
    }

    // Return success
    return 0;
}



int snesbin_encode_to_indexed(unsigned char * ptr_source_image_data, int source_width, int source_height, long int * ptr_output_size, unsigned char ** ptr_ptr_output_data)
{
    // Set output file size based on Width, Height and bit packing
    *ptr_output_size = (source_width * source_height) / (8 / SNES_BITS_PER_PIXEL_2BPP);

    *ptr_ptr_output_data = malloc(*ptr_output_size);

    // Did the alloc succeed?
    if(*ptr_ptr_output_data == NULL)
        return -1;


    // Encode the image data
    if (0 != snesbin_encode_image_data_2bpp(ptr_source_image_data,
                                            source_width,
                                            source_height,
                                            ptr_output_size,
                                           *ptr_ptr_output_data));
        return -1;


    // Return success
    return 0;
}
