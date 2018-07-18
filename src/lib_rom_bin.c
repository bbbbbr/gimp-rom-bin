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

#include "format_snes_3bpp.h"

#include "format_gba_4bpp.h"
#include "format_snespce_4bpp.h"
#include "format_ggsmswsc_4bpp.h"
#include "format_gens_4bpp.h"

#include "format_gba_8bpp.h"
#include "format_snes_8bpp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgimp/gimp.h>



static int (*function_map_decode[])(rom_gfx_data *,
                                    app_gfx_data *,
                                    app_color_data *) =  {
        [BIN_MODE_NES_1BPP]      = bin_decode_nes_1bpp,
        [BIN_MODE_NES_2BPP]      = bin_decode_nes_2bpp,
        [BIN_MODE_SNESGB_2BPP]   = bin_decode_snesgb_2bpp,
        [BIN_MODE_NGPC_2BPP]     = bin_decode_ngpc_2bpp,

        [BIN_MODE_SNES_3BPP]     = bin_decode_snes_3bpp,

        [BIN_MODE_GBA_4BPP]      = bin_decode_gba_4bpp,
        [BIN_MODE_SNES_4BPP]     = bin_decode_snes_4bpp,
        [BIN_MODE_GGSMSWSC_4BPP] = bin_decode_ggsmswsc_4bpp,
        [BIN_MODE_GENS_4BPP]     = bin_decode_gens_4bpp,

        [BIN_MODE_GBA_8BPP]      = bin_decode_gba_8bpp,
        [BIN_MODE_SNES_8BPP]     = bin_decode_snes_8bpp,
};


static int (*function_map_encode[])(rom_gfx_data *,
                                    app_gfx_data *) =  {
        [BIN_MODE_NES_1BPP]      = bin_encode_nes_1bpp,
        [BIN_MODE_NES_2BPP]      = bin_encode_nes_2bpp,
        [BIN_MODE_SNESGB_2BPP]   = bin_encode_snesgb_2bpp,
        [BIN_MODE_NGPC_2BPP]     = bin_encode_ngpc_2bpp,

        [BIN_MODE_SNES_3BPP]     = bin_encode_snes_3bpp,

        [BIN_MODE_GBA_4BPP]      = bin_encode_gba_4bpp,
        [BIN_MODE_SNES_4BPP]     = bin_encode_snes_4bpp,
        [BIN_MODE_GGSMSWSC_4BPP] = bin_encode_ggsmswsc_4bpp,
        [BIN_MODE_GENS_4BPP]     = bin_encode_gens_4bpp,

        [BIN_MODE_GBA_8BPP]      = bin_encode_gba_8bpp,
        [BIN_MODE_SNES_8BPP]     = bin_encode_snes_8bpp,
};



void rom_bin_init_structs(rom_gfx_data * p_rom_gfx,
                          app_gfx_data * p_app_gfx,
                          app_color_data * p_colorpal)
{
    p_app_gfx->image_mode = 0;
    p_app_gfx->width      = 0;
    p_app_gfx->height     = 0;
    p_app_gfx->p_data     = NULL;
    p_app_gfx->size       = 0;
    p_app_gfx->p_surplus_bytes    = NULL;
    p_app_gfx->surplus_bytes_size = 0;


    p_colorpal->index           = 0;
    p_colorpal->bytes_per_pixel = 0;
    p_colorpal->size            = 0;
    p_colorpal->p_data          = NULL;


    p_rom_gfx->size   = 0;
    p_rom_gfx->p_data = NULL;
}



int rom_bin_decode(rom_gfx_data * p_rom_gfx,
                   app_gfx_data * p_app_gfx,
                   app_color_data * p_colorpal)
{
    // Call the matching decode function
    if (p_app_gfx->image_mode < BIN_MODE_LAST) {

        if (0 != function_map_decode[ p_app_gfx->image_mode ](p_rom_gfx,
                                                             p_app_gfx,
                                                             p_colorpal))
            return -1;
    }
    else
        return -1;


    // Return success
    return 0;
}


int rom_bin_encode(rom_gfx_data * p_rom_gfx,
                   app_gfx_data * p_app_gfx)
{
    // Call the matching encode function
    if (p_app_gfx->image_mode < BIN_MODE_LAST) {

        if (0 != function_map_encode[ p_app_gfx->image_mode ](p_rom_gfx,
                                                             p_app_gfx))
            return -1;
    }
    else
        return -1;


    // Return success
    return 0;
}
