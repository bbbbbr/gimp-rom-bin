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
#include "format_casioloopy_4bpp.h"

#include "format_gba_8bpp.h" // Also covers Casio Loopy
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
        [BIN_MODE_CASIOLOOPY_4BPP] = bin_decode_casioloopy_4bpp,

        [BIN_MODE_GBA_8BPP]      = bin_decode_gba_8bpp,
        [BIN_MODE_SNES_8BPP]     = bin_decode_snes_8bpp,
        [BIN_MODE_CASIOLOOPY_8BPP] = bin_decode_gba_8bpp, // Casio Loopy uses same 8bpp format as GBA
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
        [BIN_MODE_CASIOLOOPY_4BPP] = bin_encode_casioloopy_4bpp,

        [BIN_MODE_GBA_8BPP]      = bin_encode_gba_8bpp,
        [BIN_MODE_SNES_8BPP]     = bin_encode_snes_8bpp,
        [BIN_MODE_CASIOLOOPY_8BPP]  = bin_encode_gba_8bpp, // Casio Loopy uses same 8bpp format as GBA
};



//
// The list of decodable/encodable formats, shared by the import/export
// dialog and by the "mode" argument of every registered procedure.
//
// Order matters: it's the order the formats show up in the dialog.
//
static const rom_bin_mode_entry rom_bin_mode_list_data[] = {

    { "nes-1bpp",        BIN_MODE_NES_1BPP,      "1bpp NES",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_NES },

    { "nes-2bpp",        BIN_MODE_NES_2BPP,      "2bpp NES",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_NES },

    { "snesgb-2bpp",     BIN_MODE_SNESGB_2BPP,   "2bpp SNES/GB",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_GB | ROM_BIN_EXT_MASK_SNES },

    { "ngpc-2bpp",       BIN_MODE_NGPC_2BPP,     "2bpp NGPC",
      ROM_BIN_EXT_MASK_GENERIC },

    { "snes-3bpp",       BIN_MODE_SNES_3BPP,     "3bpp SNES",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_SNES },

    { "gba-4bpp",        BIN_MODE_GBA_4BPP,      "4bpp GBA",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_GBA },

    { "snes-4bpp",       BIN_MODE_SNES_4BPP,     "4bpp SNES",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_SNES },

    { "ggsms-4bpp",      BIN_MODE_GGSMSWSC_4BPP, "4bpp GG/SMS/WSC",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_SMSGG },

    { "gens-4bpp",       BIN_MODE_GENS_4BPP,     "4bpp GEN",
      ROM_BIN_EXT_MASK_GENERIC },

    { "casioloopy-4bpp", BIN_MODE_CASIOLOOPY_4BPP, "4bpp Casio Loopy",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_CASIOLOOPY },

    { "gba-8bpp",        BIN_MODE_GBA_8BPP,      "8bpp GBA",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_GBA },

    { "snes-8bpp",       BIN_MODE_SNES_8BPP,     "8bpp SNES",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_SNES },

    { "casioloopy-8bpp", BIN_MODE_CASIOLOOPY_8BPP, "8bpp Casio Loopy",
      ROM_BIN_EXT_MASK_GENERIC | ROM_BIN_EXT_MASK_CASIOLOOPY },
};



const rom_bin_mode_entry * rom_bin_mode_list(int * p_count)
{
    if (p_count)
        *p_count = G_N_ELEMENTS(rom_bin_mode_list_data);

    return rom_bin_mode_list_data;
}


const rom_bin_mode_entry * rom_bin_mode_entry_by_mode(int mode)
{
    unsigned int i;

    for (i = 0; i < G_N_ELEMENTS(rom_bin_mode_list_data); i++) {
        if (rom_bin_mode_list_data[i].mode == mode)
            return &rom_bin_mode_list_data[i];
    }

    return NULL;
}


const rom_bin_mode_entry * rom_bin_mode_entry_by_nick(const gchar * nick)
{
    unsigned int i;

    if (nick == NULL)
        return NULL;

    for (i = 0; i < G_N_ELEMENTS(rom_bin_mode_list_data); i++) {
        if (g_strcmp0(rom_bin_mode_list_data[i].nick, nick) == 0)
            return &rom_bin_mode_list_data[i];
    }

    return NULL;
}


bool rom_bin_mode_matches_ext_mode(const rom_bin_mode_entry * p_entry,
                                   int ext_mode)
{
    // The generic (".bin") dialog offers every format
    if (ext_mode == BIN_EXT_MODE_GENERIC)
        return true;

    if (p_entry == NULL)
        return false;

    return (p_entry->ext_mask & ROM_BIN_EXT_MASK_GROUP(ext_mode)) != 0;
}


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
