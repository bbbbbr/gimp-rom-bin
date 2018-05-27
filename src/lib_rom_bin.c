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

#include "lib_rom_bin.h"

#include "format_nes_1bpp.h"
#include "format_nes_2bpp.h"
#include "format_snesgb_2bpp.h"
#include "format_ngp_2bpp.h"
#include "format_gba_4bpp.h"
#include "format_snespce_4bpp.h"
#include "format_ggsmswsc_4bpp.h"
#include "format_gens_4bpp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>


void rom_bin_init_structs(rom_gfx_data * p_rom_gfx,
                          app_gfx_data * p_app_gfx,
                          app_color_data * p_colorpal)
{
    p_app_gfx->image_mode = 0;
    p_app_gfx->width      = 0;
    p_app_gfx->height     = 0;
    p_app_gfx->p_data     = NULL;
    p_app_gfx->size   = 0;

    p_colorpal->index           = 0;
    p_colorpal->bytes_per_pixel = 0;
    p_colorpal->size            = 0;
    p_colorpal->p_data          = NULL;

    p_rom_gfx->size   = 0;
    p_rom_gfx->p_data = NULL;
}



// TODO: use a structure to pass all these vars
// TODO: tidy this up
// TODO: remove _indexed from function name
int rom_bin_decode_to_indexed(rom_gfx_data * p_rom_gfx,
                              app_gfx_data * p_app_gfx,
                              app_color_data * p_colorpal)
{

    // TODO : remove Shims for compiler
    unsigned int   * p_width;
    unsigned int   * p_height;
    unsigned char ** p_p_image_data;

    unsigned char ** p_p_color_map_data;
    int            * p_color_map_size;
    void * p_file_data;
    long int file_size;

    printf("rom_bin_decode_to_indexed\n");


    if (BIN_MODE_NES_1BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_nes_1bpp(p_rom_gfx,
                                                p_app_gfx,
                                                p_colorpal))
            return -1;
    }
    else if (BIN_MODE_NES_2BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_nes_2bpp(p_file_data,
                                                file_size,
                                                p_width,
                                                p_height,
                                                p_p_image_data,
                                                p_p_color_map_data,
                                                p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else if (BIN_MODE_SNESGB_2BPP == p_app_gfx->image_mode) {

        if (0 != snesbin_decode_to_indexed_snesgb_2bpp(p_file_data,
                                                       file_size,
                                                       p_width,
                                                       p_height,
                                                       p_p_image_data,
                                                       p_p_color_map_data,
                                                       p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else if (BIN_MODE_NGP_2BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_ngp_2bpp(p_file_data,
                                                file_size,
                                                p_width,
                                                p_height,
                                                p_p_image_data,
                                                p_p_color_map_data,
                                                p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else if (BIN_MODE_GBA_4BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_gba_4bpp(p_file_data,
                                                file_size,
                                                p_width,
                                                p_height,
                                                p_p_image_data,
                                                p_p_color_map_data,
                                                p_color_map_size,
                                                p_app_gfx->image_mode))
        return -1;
    }
    else if (BIN_MODE_SNES_4BPP == p_app_gfx->image_mode) {

        if (0 != snesbin_decode_to_indexed_snes_4bpp(p_file_data,
                                                     file_size,
                                                     p_width,
                                                     p_height,
                                                     p_p_image_data,
                                                     p_p_color_map_data,
                                                     p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else if (BIN_MODE_GGSMSWSC_4BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_ggsmswsc_4bpp(p_file_data,
                                                     file_size,
                                                     p_width,
                                                     p_height,
                                                     p_p_image_data,
                                                     p_p_color_map_data,
                                                     p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else if (BIN_MODE_GENS_4BPP == p_app_gfx->image_mode) {

        if (0 != bin_decode_to_indexed_gens_4bpp(p_file_data,
                                                 file_size,
                                                 p_width,
                                                 p_height,
                                                 p_p_image_data,
                                                 p_p_color_map_data,
                                                 p_color_map_size,
                                                p_app_gfx->image_mode))
            return -1;
    }
    else {
        return -1;
    }


    // Return success
    return 0;
}


// TODO: tidy this up
// TODO: remove _indexed from function name
int rom_bin_encode_to_indexed(rom_gfx_data * p_rom_gfx,
                              app_gfx_data * p_app_gfx)
{
    // compiler shims
    unsigned char * ptr_source_image_data;
    int source_width;
    int source_height;
    long int * ptr_output_size;
    unsigned char ** ptr_ptr_output_data;
    int image_mode;


    if (BIN_MODE_NES_1BPP == p_app_gfx->image_mode) {

        if (0 != bin_encode_to_indexed_nes_1bpp(p_rom_gfx,
                                                p_app_gfx))
            return -1;
    }
    else if (BIN_MODE_NES_2BPP == image_mode) {

        if (0 != bin_encode_to_indexed_nes_2bpp(ptr_source_image_data,
                                                source_width,
                                                source_height,
                                                ptr_output_size,
                                                ptr_ptr_output_data,
                                                image_mode))
            return -1;
    }
    else if (BIN_MODE_SNESGB_2BPP == image_mode) {

        if (0 != snesbin_encode_to_indexed_snesgb_2bpp(ptr_source_image_data,
                                                       source_width,
                                                       source_height,
                                                       ptr_output_size,
                                                       ptr_ptr_output_data,
                                                       image_mode))
            return -1;
    }
    else if (BIN_MODE_NGP_2BPP == image_mode) {

        if (0 != bin_encode_to_indexed_ngp_2bpp(ptr_source_image_data,
                                                source_width,
                                                source_height,
                                                ptr_output_size,
                                                ptr_ptr_output_data,
                                                image_mode))
            return -1;
    }
    else if (BIN_MODE_GBA_4BPP == image_mode) {

        if (0 != bin_encode_to_indexed_gba_4bpp(ptr_source_image_data,
                                                source_width,
                                                source_height,
                                                ptr_output_size,
                                                ptr_ptr_output_data,
                                                image_mode))
            return -1;
    }
    else if (BIN_MODE_SNES_4BPP == image_mode) {

        if (0 != snesbin_encode_to_indexed_snes_4bpp(ptr_source_image_data,
                                                     source_width,
                                                     source_height,
                                                     ptr_output_size,
                                                     ptr_ptr_output_data,
                                                     image_mode))
            return -1;
    }
    else if (BIN_MODE_GGSMSWSC_4BPP == image_mode) {

        if (0 != bin_encode_to_indexed_ggsmswsc_4bpp(ptr_source_image_data,
                                                     source_width,
                                                     source_height,
                                                     ptr_output_size,
                                                     ptr_ptr_output_data,
                                                     image_mode))
            return -1;
    }
    else if (BIN_MODE_GENS_4BPP == image_mode) {

        if (0 != bin_encode_to_indexed_gens_4bpp(ptr_source_image_data,
                                                 source_width,
                                                 source_height,
                                                 ptr_output_size,
                                                 ptr_ptr_output_data,
                                                 image_mode))
            return -1;
    }
    else {
            return -1;
    }


    // Return success
    return 0;
}
