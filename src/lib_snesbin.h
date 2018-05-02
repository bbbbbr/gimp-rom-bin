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

int snesbin_decode_to_indexed(void * ptr_file_data, long int file_size, int * ptr_width, int * ptr_height, unsigned char ** ptr_ptr_image_data, unsigned char ** ptr_ptr_color_map_data, int * color_map_size);