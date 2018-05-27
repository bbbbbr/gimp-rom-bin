/*=======================================================================
              YY-CHR load / save plugin for the GIMP
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

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <libgimp/gimp.h>


#ifndef ROM_BIN_FILE_HEADER
#define ROM_BIN_FILE_HEADER

    // TODO: update naming convention
    enum rom_bin_modes {
        BIN_MODE_NES_1BPP,
        BIN_MODE_NES_2BPP,
        BIN_MODE_SNESGB_2BPP,
        BIN_MODE_NGP_2BPP,
        BIN_MODE_GBA_4BPP,
        BIN_MODE_SNES_4BPP,
        BIN_MODE_GGSMSWSC_4BPP,
        BIN_MODE_GENS_4BPP,
    };

    // TODO: move these to lib_rom_bin.h

        typedef struct rom_gfx_attrib {
            unsigned int  IMAGE_WIDTH_DEFAULT;
            unsigned int  TILE_PIXEL_WIDTH;
            unsigned int  TILE_PIXEL_HEIGHT;
            unsigned char BITS_PER_PIXEL;

            unsigned char DECODED_NUM_COLORS;
            unsigned char DECODED_BYTES_PER_PIXEL;
        } rom_gfx_attrib;


        typedef struct image_gfx_data {
            unsigned char    image_mode; // TODO update functions and calls to use this
            unsigned int     width;
            unsigned int     height;
            unsigned char  * p_data;
        } image_gfx_data;

        typedef struct image_color_data {
            unsigned int     index;
            unsigned char    bytes_per_pixel;
            int              size;
            unsigned char * p_data;
        } image_color_data;




    int rom_bin_decode_to_indexed(void *, long int, image_gfx_data *, image_color_data *, int);
    int rom_bin_encode_to_indexed(unsigned char *, int, int, long int *, unsigned char **, int);


#endif // ROM_BIN_FILE_HEADER