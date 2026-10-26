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
#include <stdbool.h>
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
        BIN_MODE_CASIOLOOPY_4BPP,

        BIN_MODE_GBA_8BPP,
        BIN_MODE_SNES_8BPP,
        BIN_MODE_CASIOLOOPY_8BPP,

        BIN_MODE_LAST
    };

    enum rom_bin_extension_modes {
        BIN_EXT_MODE_GENERIC,
        BIN_EXT_MODE_GB,
        BIN_EXT_MODE_NES,
        BIN_EXT_MODE_SMSGG,
        BIN_EXT_MODE_SNES,
        BIN_EXT_MODE_GBA,
        BIN_EXT_MODE_CASIOLOOPY,
    };

    enum rom_bin_pixel_modes {
        BIN_BITDEPTH_INDEXED = 1,
        BIN_BITDEPTH_INDEXED_ALPHA = 2,
        BIN_BITDEPTH_LAST
    };

    // Name of the GIMP procedure argument that carries the image format
    #define MODE_ARGUMENT  "mode"

    //
    // Description of a single image format as it is offered in the
    // import/export dialog and stored in the "mode" procedure argument.
    //
    // The nick is a stable, machine readable id (it ends up in GIMP's
    // sessionrc as the last used value), the label is what the user sees.
    //
    typedef struct rom_bin_mode_entry {
        const gchar * nick;      // stable id for the procedure "mode" argument
        int           mode;      // matching enum rom_bin_modes value
        const gchar * label;     // human readable name shown in the dialog
        unsigned int  ext_mask;  // extension groups that offer this mode
    } rom_bin_mode_entry;

    // Which dialog groups a mode is listed in. BIN_EXT_MODE_GENERIC means
    // "listed in the catch-all .bin dialog", so a bit per extension group.
    #define ROM_BIN_EXT_MASK_ALL       (0xFFFFFFFFu)
    #define ROM_BIN_EXT_MASK_GROUP(g)  (1u << (g))

    #define ROM_BIN_EXT_MASK_GENERIC    ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_GENERIC)
    #define ROM_BIN_EXT_MASK_GB         ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_GB)
    #define ROM_BIN_EXT_MASK_NES        ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_NES)
    #define ROM_BIN_EXT_MASK_SMSGG      ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_SMSGG)
    #define ROM_BIN_EXT_MASK_SNES       ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_SNES)
    #define ROM_BIN_EXT_MASK_GBA        ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_GBA)
    #define ROM_BIN_EXT_MASK_CASIOLOOPY ROM_BIN_EXT_MASK_GROUP(BIN_EXT_MODE_CASIOLOOPY)

    const rom_bin_mode_entry * rom_bin_mode_list(int * p_count);
    const rom_bin_mode_entry * rom_bin_mode_entry_by_mode(int mode);
    const rom_bin_mode_entry * rom_bin_mode_entry_by_nick(const gchar * nick);
    bool rom_bin_mode_matches_ext_mode(const rom_bin_mode_entry * p_entry,
                                       int ext_mode);

    // TODO: move these to lib_rom_bin.h

        typedef struct rom_gfx_attrib {
            unsigned int  IMAGE_WIDTH_DEFAULT;
            bool          FORCE_DEFAULT_IMAGE_WIDTH;
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