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


#include <string.h>
#include <stdint.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "lib_rom_bin.h"
#include "read-rom-bin.h"
#include "write-rom-bin.h"
#include "export-dialog.h"

const char LOAD_PROCEDURE[]                 = "file-rom-bin-load";
const char LOAD_PROCEDURE_NES2BPP_CHRNES[]  = "file-bin-bin-load-nes2bpp-chrnes";
const char LOAD_PROCEDURE_GB2BPP_GB[]       = "file-bin-bin-load-gb2bpp-gb";
const char LOAD_PROCEDURE_GGSMS4BPP_GGSMS[] = "file-bin-bin-load-ggsms4bpp-ggsms";
const char LOAD_PROCEDURE_SNES[]            = "file-bin-bin-load-snes";
const char LOAD_PROCEDURE_GBA[]             = "file-bin-bin-load-gba";
const char LOAD_PROCEDURE_CASIOLOOPY[]      = "file-bin-bin-load-casioloopy";

const char SAVE_PROCEDURE[]                 = "file-rom-bin-save";
const char SAVE_PROCEDURE_NES2BPP_CHRNES[]  = "file-rom-bin-save-nes2bpp-chrnes";
const char SAVE_PROCEDURE_GB2BPP_GB[]       = "file-rom-bin-save-gb2bpp-gb";
const char SAVE_PROCEDURE_GGSMS4BPP_GGSMS[] = "file-bin-bin-save-ggsms4bpp-ggsms";
const char SAVE_PROCEDURE_SNES[]            = "file-bin-bin-save-snes";
const char SAVE_PROCEDURE_GBA[]             = "file-bin-bin-save-gba";
const char SAVE_PROCEDURE_CASIOLOOPY[]      = "file-bin-bin-save-casioloopy";

const char BINARY_NAME[]    = "file-rom-bin";

// The name of the procedure argument holding the ROM image format is
// defined in lib_rom_bin.h as MODE_ARGUMENT. It is registered as a
// "choice" argument so that GIMP remembers the last used value between
// invocations (it used to be a plain combo box in the plugin's own
// dialog, which was forgotten every time).

// Sentinel for "the format can not be guessed from the extension, ask"
#define ROM_BIN_MODE_ASK (-1)


//
// Describes one file handler (a load and/or an export procedure).
//
typedef struct {
    const gchar * name;
    const gchar * extensions;
    const gchar * menu_label;
    const gchar * blurb;

    // Which subset of the supported formats the format picker offers
    int ext_mode;

    // Format implied by the extension, ROM_BIN_MODE_ASK to show the picker
    int fixed_mode;

    // Pre-selected format: what the dialog starts with, and what is used
    // when no dialog can be shown (batch runs, thumbnailing, ...) unless
    // the caller passes an explicit "mode" argument
    const gchar * default_nick;
} rom_file_handler;


static const rom_file_handler rom_load_handlers[] = {
    { LOAD_PROCEDURE,
      "bin",
      "ROM bin image",
      "Loads images in the ROM bin file format",
      BIN_EXT_MODE_GENERIC,
      ROM_BIN_MODE_ASK,
      "snes-4bpp" },

    { LOAD_PROCEDURE_NES2BPP_CHRNES,
      "chr,nes",
      "ROM NES .chr and .nes image 2-bpp",
      "Loads images in the NES .chr and .nes 2-bpp file format",
      BIN_EXT_MODE_NES,
      BIN_MODE_NES_2BPP,
      NULL },

    { LOAD_PROCEDURE_GB2BPP_GB,
      "gb,gbc,2bpp,duck,md0,md1,md2",
      "ROM GB .gb .gbc .duck .md0 .md1 .md2 image 2-bpp",
      "Loads images in the GameBoy .gb .gbc 2-bpp file format",
      BIN_EXT_MODE_GB,
      BIN_MODE_SNESGB_2BPP,
      NULL },

    { LOAD_PROCEDURE_GGSMS4BPP_GGSMS,
      "gg,sms",
      "ROM SMS/GG .sms .gg image 4-bpp",
      "Loads images in SMS/GameGear .gg .sms 4-bpp file format",
      BIN_EXT_MODE_SMSGG,
      BIN_MODE_GGSMSWSC_4BPP,
      NULL },

    { LOAD_PROCEDURE_SNES,
      "sfc,smc",
      "ROM SNES .sfc .smc image",
      "Loads ROM images in SNES .sfc .smc formats",
      BIN_EXT_MODE_SNES,
      ROM_BIN_MODE_ASK,
      "snes-4bpp" },

    { LOAD_PROCEDURE_GBA,
      "gba",
      "ROM GBA .gba image",
      "Loads ROM images in GBA .gba formats",
      BIN_EXT_MODE_GBA,
      ROM_BIN_MODE_ASK,
      "gba-4bpp" },

    { LOAD_PROCEDURE_CASIOLOOPY,
      "loopy",
      "ROM Casio Loopy .loopy image",
      "Loads ROM images in Casio Loopy .loopy formats",
      BIN_EXT_MODE_CASIOLOOPY,
      ROM_BIN_MODE_ASK,
      "casioloopy-4bpp" },
};


static const rom_file_handler rom_export_handlers[] = {
    { SAVE_PROCEDURE,
      "bin",
      "ROM bin image",
      "Saves files in the ROM bin image format",
      BIN_EXT_MODE_GENERIC,
      ROM_BIN_MODE_ASK,
      "snes-4bpp" },

    { SAVE_PROCEDURE_NES2BPP_CHRNES,
      "chr,nes",
      "CHR NES 2bpp bin image",
      "Saves files in the NES .chr and .nes 2-bpp image format",
      BIN_EXT_MODE_NES,
      BIN_MODE_NES_2BPP,
      NULL },

    { SAVE_PROCEDURE_GB2BPP_GB,
      "gb,gbc,2bpp",
      "GameBoy .gb .gbc 2bpp bin image",
      "Saves files in the GameBoy .gb .gbc 2-bpp image format",
      BIN_EXT_MODE_GB,
      BIN_MODE_SNESGB_2BPP,
      NULL },

    { SAVE_PROCEDURE_GGSMS4BPP_GGSMS,
      "gg,sms",
      "SMS/GG .sms .gg image 4-bpp",
      "Saves files in SMS/GameGear .gg .sms 4-bpp file format",
      BIN_EXT_MODE_SMSGG,
      BIN_MODE_GGSMSWSC_4BPP,
      NULL },

    { SAVE_PROCEDURE_SNES,
      "sfc,smc",
      "SNES .sfc .smc image",
      "Saves files in SNES .sfc .smc ROM formats",
      BIN_EXT_MODE_SNES,
      ROM_BIN_MODE_ASK,
      "snes-4bpp" },

    { SAVE_PROCEDURE_GBA,
      "gba",
      "GBA .gba image",
      "Saves files in GBA .gba ROM formats",
      BIN_EXT_MODE_GBA,
      ROM_BIN_MODE_ASK,
      "gba-4bpp" },

    { SAVE_PROCEDURE_CASIOLOOPY,
      "loopy",
      "Casio Loopy .loopy image",
      "Saves files in Casio Loopy .loopy ROM formats",
      BIN_EXT_MODE_CASIOLOOPY,
      ROM_BIN_MODE_ASK,
      "casioloopy-4bpp" },
};


//
// Plug-in class boilerplate (GIMP 3 replaced the PLUG_IN_INFO struct and
// the query()/run() entry points with a GObject derived from GimpPlugIn)
//
typedef struct _RomBin      RomBin;
typedef struct _RomBinClass RomBinClass;

struct _RomBin
{
    GimpPlugIn parent_instance;
};

struct _RomBinClass
{
    GimpPlugInClass parent_class;
};

#define ROM_BIN_TYPE  (rom_bin_get_type ())
#define ROM_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ROM_BIN_TYPE, RomBin))

GType                   rom_bin_get_type         (void) G_GNUC_CONST;

static GList          * rom_bin_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * rom_bin_create_procedure (GimpPlugIn            *plug_in,
                                                  const gchar           *name);
static gboolean         rom_bin_set_i18n         (GimpPlugIn            *plug_in,
                                                  const gchar           *procedure_name,
                                                  gchar                 **gettext_domain,
                                                  gchar                 **catalog_dir);

static GimpValueArray * rom_bin_load             (GimpProcedure         *procedure,
                                                  GimpRunMode            run_mode,
                                                  GFile                 *file,
                                                  GimpMetadata          *metadata,
                                                  GimpMetadataLoadFlags *flags,
                                                  GimpProcedureConfig   *config,
                                                  gpointer               run_data);

static GimpValueArray * rom_bin_export           (GimpProcedure         *procedure,
                                                  GimpRunMode            run_mode,
                                                  GimpImage             *image,
                                                  GFile                 *file,
                                                  GimpExportOptions     *options,
                                                  GimpMetadata          *metadata,
                                                  GimpProcedureConfig   *config,
                                                  gpointer               run_data);

G_DEFINE_TYPE (RomBin, rom_bin, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ROM_BIN_TYPE)


static void rom_bin_class_init (RomBinClass * klass)
{
    GimpPlugInClass * plug_in_class = GIMP_PLUG_IN_CLASS (klass);

    plug_in_class->query_procedures = rom_bin_query_procedures;
    plug_in_class->create_procedure = rom_bin_create_procedure;

    // This plugin ships no translations; without this GIMP warns about a
    // missing locale/ directory for every registered procedure
    plug_in_class->set_i18n         = rom_bin_set_i18n;
}

static gboolean rom_bin_set_i18n (GimpPlugIn   * plug_in,
                                  const gchar  * procedure_name,
                                  gchar       ** gettext_domain,
                                  gchar       ** catalog_dir)
{
    return FALSE;
}

static void rom_bin_init (RomBin * rom_bin)
{
}

static GList * rom_bin_query_procedures (GimpPlugIn * plug_in)
{
    GList      * list = NULL;
    unsigned int i;

    for (i = 0; i < G_N_ELEMENTS(rom_load_handlers); i++)
        list = g_list_append (list, g_strdup (rom_load_handlers[i].name));

    for (i = 0; i < G_N_ELEMENTS(rom_export_handlers); i++)
        list = g_list_append (list, g_strdup (rom_export_handlers[i].name));

    return list;
}

// Find the description of a handler by procedure name
static const rom_file_handler * find_handler (const gchar * name)
{
    unsigned int i;

    for (i = 0; i < G_N_ELEMENTS(rom_load_handlers); i++)
        if (!strcmp (name, rom_load_handlers[i].name))
            return &rom_load_handlers[i];

    for (i = 0; i < G_N_ELEMENTS(rom_export_handlers); i++)
        if (!strcmp (name, rom_export_handlers[i].name))
            return &rom_export_handlers[i];

    return NULL;
}

// Add the "which ROM format" picker, unless the extension already decided
static void add_mode_argument (GimpProcedure              * procedure,
                               const rom_file_handler     * handler)
{
    const rom_bin_mode_entry * modes;
    GimpChoice               * choice;
    int                        count;
    int                        i;

    if (handler->fixed_mode != ROM_BIN_MODE_ASK)
        return;

    modes  = rom_bin_mode_list (&count);
    choice = gimp_choice_new ();

    for (i = 0; i < count; i++) {

        if (! rom_bin_mode_matches_ext_mode (&modes[i], handler->ext_mode))
            continue;

        gimp_choice_add (choice,
                         modes[i].nick,
                         modes[i].mode,
                         modes[i].label,
                         NULL);
    }

    // gimp_procedure_add_choice_argument() takes ownership of the choice
    gimp_procedure_add_choice_argument (procedure,
                                        MODE_ARGUMENT,
                                        "_Image format",
                                        handler->blurb,
                                        choice,
                                        handler->default_nick,
                                        G_PARAM_READWRITE);
}

static GimpProcedure * rom_bin_create_procedure (GimpPlugIn  * plug_in,
                                                 const gchar * name)
{
    const rom_file_handler * handler;
    GimpProcedure          * procedure = NULL;
    gboolean                 is_load;
    unsigned int             i;

    handler = find_handler (name);
    if (handler == NULL)
        return NULL;

    is_load = FALSE;
    for (i = 0; i < G_N_ELEMENTS(rom_load_handlers); i++)
        if (!strcmp (name, rom_load_handlers[i].name))
            is_load = TRUE;

    if (is_load) {

        procedure = gimp_load_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             rom_bin_load,
                                             (gpointer) handler, NULL);

        gimp_procedure_set_menu_label (procedure, handler->menu_label);

        gimp_procedure_set_documentation (procedure,
                                          handler->blurb,
                                          handler->blurb,
                                          name);

        gimp_procedure_set_attribution (procedure,
                                        "bbbbbr & others",
                                        "GPLv3",
                                        "2018");

        gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                            handler->extensions);
    }
    else {

        procedure = gimp_export_procedure_new (plug_in, name,
                                               GIMP_PDB_PROC_TYPE_PLUGIN,
                                               FALSE,
                                               rom_bin_export,
                                               (gpointer) handler, NULL);

        gimp_procedure_set_image_types (procedure, "INDEXED");

        gimp_procedure_set_menu_label (procedure, handler->menu_label);

        gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                             handler->menu_label);

        gimp_procedure_set_documentation (procedure,
                                          handler->blurb,
                                          handler->blurb,
                                          name);

        gimp_procedure_set_attribution (procedure,
                                        "bbbbbr & others",
                                        "GPLv3",
                                        "2018");

        gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                            handler->extensions);

        // Indexed pixel data (and the alpha channel that marks the
        // "no data here" padding tiles) is what this plugin writes
        gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                                GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                                GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                                NULL, NULL, NULL);
    }

    add_mode_argument (procedure, handler);

    return procedure;
}

// Work out which ROM format to use, showing the picker when needed
static gboolean resolve_image_mode (const rom_file_handler   * handler,
                                    GimpProcedure            * procedure,
                                    GimpProcedureConfig      * config,
                                    GimpRunMode               run_mode,
                                    gint                    * p_image_mode)
{
    // The extension already told us (NES .chr, GameBoy .gb, SMS .sms, ...)
    if (handler->fixed_mode != ROM_BIN_MODE_ASK) {
        *p_image_mode = handler->fixed_mode;
        return TRUE;
    }

    // Otherwise the format comes from the "mode" argument: either what the
    // caller passed (scripted / batch use, or GIMP's remembered last value
    // in RUN-WITH-LAST-VALS mode), or what the user picks in the dialog.
    if (GIMP_RUN_INTERACTIVE == run_mode) {

        gboolean is_load = GIMP_IS_LOAD_PROCEDURE (procedure);

        gimp_ui_init (BINARY_NAME);

        if (! rom_bin_settings_dialog (procedure, config,
                                       is_load ? "Import ROM bin image"
                                               : "Export ROM bin image",
                                       is_load ? "_Open" : NULL))
            return FALSE; // cancelled by the user
    }

    *p_image_mode = gimp_procedure_config_get_choice_id (config, MODE_ARGUMENT);

    return TRUE;
}

static GimpValueArray * rom_bin_load (GimpProcedure         * procedure,
                                      GimpRunMode            run_mode,
                                      GFile                 * file,
                                      GimpMetadata          * metadata,
                                      GimpMetadataLoadFlags * flags,
                                      GimpProcedureConfig   * config,
                                      gpointer               run_data)
{
    const rom_file_handler * handler = (const rom_file_handler *) run_data;
    GimpValueArray         * return_vals;
    GimpImage              * image;
    GError                 * error = NULL;
    gint                     image_mode;

    // The plugin talks to GIMP's GeglBuffers directly, which needs GEGL
    // (and babl) to be initialized in the plugin process
    gegl_init (NULL, NULL);

    if (! resolve_image_mode (handler, procedure, config, run_mode, &image_mode))
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);

    image = read_rom_bin (file, image_mode, &error);

    if (image == NULL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 error);

    return_vals = gimp_procedure_new_return_values (procedure,
                                                    GIMP_PDB_SUCCESS,
                                                    NULL);

    GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

    return return_vals;
}

static GimpValueArray * rom_bin_export (GimpProcedure        * procedure,
                                        GimpRunMode           run_mode,
                                        GimpImage            * image,
                                        GFile                * file,
                                        GimpExportOptions    * options,
                                        GimpMetadata         * metadata,
                                        GimpProcedureConfig  * config,
                                        gpointer              run_data)
{
    const rom_file_handler * handler = (const rom_file_handler *) run_data;
    GimpPDBStatusType        status       = GIMP_PDB_SUCCESS;
    GimpExportReturn         export_ret   = GIMP_EXPORT_IGNORE;
    GimpImage              * export_image = image;
    GimpDrawable           * drawable     = NULL;
    GList                  * layers;
    GError                 * error        = NULL;
    gint                     image_mode;

    // See rom_bin_load(): the GeglBuffer API needs GEGL initialized
    gegl_init (NULL, NULL);

    if (! resolve_image_mode (handler, procedure, config, run_mode, &image_mode))
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);

    // Let GIMP do the "this image can not be exported as-is" conversions
    // (flatten layers, drop unsupported modes, ...) for us
    export_ret = gimp_export_options_get_image (options, &export_image);

    layers = gimp_image_list_layers (export_image);

    if (layers != NULL)
        drawable = GIMP_DRAWABLE (layers->data);

    if (drawable == NULL) {

        status = GIMP_PDB_EXECUTION_ERROR;
        g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "The image has no layers to export");
    }
    else if (! write_rom_bin (file, export_image, drawable, image_mode, &error)) {

        status = GIMP_PDB_EXECUTION_ERROR;
    }

    g_list_free (layers);

    // The converted copy is ours to destroy
    if (GIMP_EXPORT_EXPORT == export_ret)
        gimp_image_delete (export_image);

    return gimp_procedure_new_return_values (procedure, status, error);
}
