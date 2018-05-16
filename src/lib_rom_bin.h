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


// TODO: update naming convention
enum rom_bin_modes {
    BIN_MODE_NES_1BPP,
    BIN_MODE_NES_2BPP,
    BIN_MODE_SNESGB_2BPP,
    BIN_MODE_SNES_4BPP,
    BIN_MODE_GGSMSWSC_4BPP,
    BIN_MODE_GENS_4BPP,
};




int rom_bin_decode_to_indexed(void *, long int, int *, int *, unsigned char **, unsigned char **, int *, int);
int rom_bin_encode_to_indexed(unsigned char *, int, int, long int *, unsigned char **, int);