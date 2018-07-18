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
        BIN_MODE_NGPC_2BPP,

        BIN_MODE_SNES_3BPP,

        BIN_MODE_GBA_4BPP,
        BIN_MODE_SNES_4BPP,
        BIN_MODE_GGSMSWSC_4BPP,
        BIN_MODE_GENS_4BPP,

        BIN_MODE_GBA_8BPP,
        BIN_MODE_SNES_8BPP,

        BIN_MODE_LAST
    };

    enum rom_bin_pixel_modes {
        BIN_BITDEPTH_INDEXED = 1,
        BIN_BITDEPTH_INDEXED_ALPHA = 2,
        BIN_BITDEPTH_LAST
    };

    // TODO: move these to lib_rom_bin.h

        typedef struct rom_gfx_attrib {
            unsigned int  IMAGE_WIDTH_DEFAULT;
            unsigned int  TILE_PIXEL_WIDTH;
            unsigned int  TILE_PIXEL_HEIGHT;
            unsigned char BITS_PER_PIXEL;

            unsigned int  DECODED_NUM_COLORS;
            unsigned char DECODED_BYTES_PER_COLOR;
        } rom_gfx_attrib;


        typedef struct  app_gfx_data {
            int              image_mode;
            unsigned int     width;
            unsigned int     height;
            unsigned char  * p_data;
            unsigned char    bytes_per_pixel;
            int              size;

            long int         surplus_bytes_size;
            unsigned char  * p_surplus_bytes;
        }  app_gfx_data;

        typedef struct rom_gfx_data {
            long int        size;
            unsigned char * p_data;
        } rom_gfx_data;

        typedef struct app_color_data {
            unsigned int     index;
            unsigned char    bytes_per_pixel;
            int              size;
            unsigned char * p_data;
        } app_color_data;

    void rom_bin_init_structs(rom_gfx_data *, app_gfx_data *, app_color_data *);

    int rom_bin_decode(rom_gfx_data *, app_gfx_data *, app_color_data *);
    int rom_bin_encode(rom_gfx_data *, app_gfx_data *);


#endif // ROM_BIN_FILE_HEADER