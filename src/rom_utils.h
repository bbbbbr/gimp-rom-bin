/*=======================================================================
              ROM bin load / save plugin for the GIMP
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


#ifndef ROM_UTILS_FILE_HEADER
#define ROM_UTILS_FILE_HEADER

#include "lib_rom_bin.h"

    void romimg_log_transparent_tiles(unsigned int , unsigned int *, app_gfx_data *, rom_gfx_attrib);
    void romimg_log_transparent_pixel(unsigned char *, unsigned int *,  app_gfx_data *);
    void romimg_set_decoded_pixel_and_advance(unsigned char **, unsigned char, unsigned char, app_gfx_data *);

    unsigned char * romimg_calc_appimg_offset(int, int, int, app_gfx_data *, rom_gfx_attrib);

    long int romimg_calc_encoded_size(app_gfx_data *, rom_gfx_attrib);
    void romimg_calc_decoded_size(long int, app_gfx_data *, rom_gfx_attrib);

    int romimg_stash_surplus_bytes(app_gfx_data *, rom_gfx_data *);
    int romimg_append_surplus_bytes(app_gfx_data *, rom_gfx_data *);

    int romimg_insert_color_to_map(unsigned char, unsigned char, unsigned char, app_color_data *);
    int romimg_load_color_data(app_color_data *);

#endif // ROM_UTILS_FILE_HEADER
