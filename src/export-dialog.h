/*=======================================================================
              ROM bin load / save plugin for the GIMP
                 Copyright 2018 - Others & Nathan Osman (webp plugin base)

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

#ifndef ROM_BIN_EXPORT_DIALOG_HEADER
#define ROM_BIN_EXPORT_DIALOG_HEADER

#include <glib.h>
#include <libgimp/gimp.h>

// Builds the format picker on top of the procedure's own arguments and
// runs it. Returns FALSE when the user cancelled.
gboolean rom_bin_settings_dialog(GimpProcedure * procedure,
                                 GimpProcedureConfig * config,
                                 const gchar * title,
                                 const gchar * ok_label);

#endif // ROM_BIN_EXPORT_DIALOG_HEADER
