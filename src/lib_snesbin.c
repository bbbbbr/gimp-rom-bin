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

#define SNESBIN_IMAGE_WIDTH_DEFAULT 128
#define SNESBIN_IMAGE_BITS_PER_PIXEL 2              // TODO, support 2 & 4 BPP modes
#define SNESBIN_DECODED_IMAGE_BYTES_PER_PIXEL 1    // 1 byte per pixel in indexed color mode
#define SNESBIN_DECODED_COLOR_MAP_SIZE 4            // TODO, support 2 & 4 BPP modes
#define SNESBIN_DECODED_COLOR_MAP_BYTES_PER_PIXEL 3 // R,G,B
#define SNESBIN_TILE_PIXEL_WIDTH                  8
#define SNESBIN_TILE_PIXEL_HEIGHT                 8


int snesbin_decode_image_data(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data);
int snesbin_decode_read_color_data(unsigned char * color_map_data);

int snesbin_decode_image_data(void * file_data, long int * file_size, long int *file_offset, int width, int height, unsigned char * image_data)
{
    unsigned char pixdata[2];
    unsigned char * image_pixel;

    printf("Entering Decode\n");


    // Check incoming buffers & vars
    if ((file_data      == NULL) ||
        (image_data     == NULL) ||
        (width          == 0) ||
        (height         == 0))
        return -1;


    // Make sure there is enough image data
    // then copy it into the image buffer
    // File size is a function of bits per pixel, width and height
    if (*file_size < ((width / (8 / SNESBIN_IMAGE_BITS_PER_PIXEL)) * height))
        return -1;

    printf("Starting Decode\n");

    // 2BPP SNES/GBA:
    //
    // * Packed 8x8 Tiles - serial encoded
    // * 1 Tile row = 2 bytes (byte_lo/0, byte_hi/1)
    // * Packed Bits: 876543210, 876543210
    // *              ---------  ---------
    // * Pixel #    : 012345678  012345678
    // * Pixel l/h  : LLLLLLLLL  HHHHHHHHH
    // * So, Pixel 0 = ((byte0 >> 8) & 0x01) | ((byte1 >> 7) & 0x20)


// for: 0 .. < height / 8_tile_px
//   for: 0 .. width / bpp / 8_tile_px

    // Un-bitpack the pixels
    // Decode the image top-to-bottom
    *file_offset = 0;

    for (int y=0; y < (height / SNESBIN_TILE_PIXEL_HEIGHT); y++) {
//        printf("y=%d\n", y);

        // Decode left-to-right
        // TODO: / SNESBIN_IMAGE_BITS_PER_PIXEL)
        for (int x=0; x < (width / SNESBIN_TILE_PIXEL_WIDTH); x++) {

//            printf(" x=%d\n", x);
            // Decode the 8x8 tile top to bottom
            for (int ty=0; ty < SNESBIN_TILE_PIXEL_HEIGHT; ty++) {

//                printf("  ty=%d, %ld\n", ty,*file_offset);

                // TODO : Any reason not to change void* to unsigned char* for ptr_file_data?
                // Read two bytes and unpack the 8 horizontal pixels
                pixdata[0] = *((unsigned char *)file_data + (*file_offset)++);
                pixdata[1] = *((unsigned char *)file_data + (*file_offset)++);

//                printf("  :%x, %x", pixdata[0], pixdata[1]);

                // Set up the pointer to the pixel in the destination image buffer
                // TODO: optimize
                image_pixel = (image_data + (((y * SNESBIN_TILE_PIXEL_HEIGHT) + ty) * width)
                                          +   (x * SNESBIN_TILE_PIXEL_WIDTH));

//                printf("   px=%d,py=%d\n", (x * SNESBIN_TILE_PIXEL_WIDTH), ((y * SNESBIN_TILE_PIXEL_HEIGHT) + ty));
                // Unpack the 8 horizontal pixels
                // for (int b=0;b < SNESBIN_IMAGE_BITS_PER_PIXEL; b++) {
                // TODO - tidy up constants
                for (int b=0;b < 8; b++) {

//                    printf("   pd=%d\n", ((pixdata[0] >> 7) & 0x01) | ((pixdata[1] >> 6) & 0x02));
                    // b0.MSbit = pixel.1, b1.MSbit = pixel.0
                    *image_pixel++ = ((pixdata[0] >> 7) & 0x01) | ((pixdata[1] >> 6) & 0x02);

                    // Upshift bits to prepare for the next pixel
                    pixdata[0] = pixdata[0] << 1;
                    pixdata[1] = pixdata[1] << 1;
                }
            }
        }
    }

    return 0;
}



// TODO: Rename to Set Color Data (since no palette is built into SNES bin files)
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

    color_map_data[colorindex++] = 80;
    color_map_data[colorindex++] = 80;
    color_map_data[colorindex++] = 80;

    color_map_data[colorindex++] = 160;
    color_map_data[colorindex++] = 160;
    color_map_data[colorindex++] = 160;

    color_map_data[colorindex++] = 240;
    color_map_data[colorindex++] = 240;
    color_map_data[colorindex++] = 240;

    return 0;
}


int snesbin_decode_to_indexed(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size)
{
    long int file_offset = 0;

    // Set Width & Height
    // Width is always 128
    // Height is a function of bits per pixel and file size

    // TODO: FIX ME!
    //  (8 / SNESBIN_IMAGE_BITS_PER_PIXEL)
//    *ptr_width = SNESBIN_IMAGE_WIDTH_DEFAULT;
//    *ptr_height = file_size / SNESBIN_IMAGE_WIDTH_DEFAULT;
    int tiles = file_size / ((SNESBIN_TILE_PIXEL_WIDTH * SNESBIN_TILE_PIXEL_HEIGHT)
                             / (8 / SNESBIN_IMAGE_BITS_PER_PIXEL));
    if ((tiles * SNESBIN_TILE_PIXEL_WIDTH) >= SNESBIN_IMAGE_WIDTH_DEFAULT) {
        *ptr_width = SNESBIN_IMAGE_WIDTH_DEFAULT;
    } else {
        *ptr_width = (tiles * SNESBIN_TILE_PIXEL_WIDTH);
    }

    // Integer rounding up: (x + (n-1)) / n
    *ptr_height = (((tiles * SNESBIN_TILE_PIXEL_WIDTH)
                    + (SNESBIN_IMAGE_WIDTH_DEFAULT - 1))
                   / SNESBIN_IMAGE_WIDTH_DEFAULT)
                  * SNESBIN_TILE_PIXEL_HEIGHT;

//    *ptr_width = SNESBIN_IMAGE_WIDTH_DEFAULT;
//    *ptr_height = file_size / SNESBIN_IMAGE_WIDTH_DEFAULT;


    printf("File size %ld bytes\n", file_size);
    printf("Image size width = %d x height = %d\n", *ptr_width, *ptr_height);
    printf("Allocating image buffer of size %d bytes\n", *ptr_width * *ptr_height);
    *ptr_ptr_image_data = malloc(*ptr_width * *ptr_height);

    // Make sure the alloc succeeded
    if(*ptr_ptr_image_data == NULL)
        return -1;


    // Read the image data
    if (0 != snesbin_decode_image_data(ptr_file_data,
                                       &file_size,
                                       &file_offset,
                                       *ptr_width,
                                       *ptr_height,
                                       *ptr_ptr_image_data))
        return -1;


    *color_map_size = SNESBIN_DECODED_COLOR_MAP_SIZE;
    printf("Allocating color map buffer of size %d bytes\n", *color_map_size * SNESBIN_DECODED_COLOR_MAP_BYTES_PER_PIXEL);
    *ptr_ptr_color_map_data = malloc(*color_map_size * SNESBIN_DECODED_COLOR_MAP_BYTES_PER_PIXEL);

    // Make sure the alloc succeeded
    if(*ptr_ptr_color_map_data == NULL)
        return -1;

    printf("Reading color map\n");

    // Read the color map data
    if (0 != snesbin_decode_read_color_data(*ptr_ptr_color_map_data))
        return -1;

    printf("Color map done\n");

    return 0;

}
