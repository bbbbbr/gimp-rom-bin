gimp file-rom-bin plugin
===========

GIMP plugin for reading, writing and converting ROM images and tile files. Useful in emulation ROM modding, hacks and extracting/inserting artwork.

Download compiled executables here: 
 * [Linux GIMP 3.x](/bin/linux)
 * [Windows GIMP 3.x+](/bin/windows)

Note: `master` targets **GIMP 3.x**

![GIMP Image Editor opening Super Nintendo Image Tiles](https://raw.githubusercontent.com/bbbbbr/gimp-rom-bin/master/info/gimp-rom-image-plugin.png)

Supported image formats:
 * NES 1bpp / 2bpp
 * SNES 2bpp / 3bpp / 4bpp / 8bpp
 * GB 2bpp
 * NGP 2bpp
 * GBA 4bpp / 8bpp
 * GG/SMS/WSC 4bpp
 * MD 4bpp
 * Casio Loopy 4bpp / 8bpp
 
Supported file extensions:
 * .bin
 * .sfc, .smc
 * .chr (nes 2bpp)
 * .nes (nes 2bpp)
 * .gb/c (gb 2bpp)
 * .duck, .md1, .md2, .md0 (gb 2bpp)
 * .sms, .gg (sms/gg 4bpp)
 * .gba
 * .loopy
  


## Acknowledgement:
 * Source uses some webp gimp plugin code from Nathan Osman (Copyright 2012)
 * Some format documentation from: https://mrclick.zophar.net/TilEd/download/consolegfx.txt
 * YYchr was used for a lot validating format input/output


## Quick instructions:

Native compile/install on Linux using below.

```
If GIMP & build tools not yet installed:
(example for debian/ubuntu/mint)
 * sudo apt install gimp
 * sudo apt install build-essential
 * sudo apt install libgimp-3.0-dev
 
Then: 
* cd gimp-rom-bin
* make
* make install        # copies into ~/.config/GIMP/3.2/plug-ins/file-rom-bin/

Then restart GIMP so the new plug-in is picked up.

Plug-in folder locations:
 Linux: ~/.config/GIMP/3.2/plug-ins/<plugin-name>/   (GIMP 3.x sub directory per plug-in)
 Windows: C:\Program Files\GIMP 3\lib\gimp\3.0\plug-ins

```
Guide for [Cross-compiling to Windows on Linux](https://github.com/bbbbbr/gimp-rom-bin/blob/master/doc/GIMP%20jhbuild%20for%20Windows%20on%20Linux.md)

## Batch / scripting (GIMP 3)

The image format is a real procedure argument now (`mode`, a choice), instead of
a combo box that only existed inside the plug-in's own dialog. That means batch
runs can pick a format, and GIMP remembers the last one used in the GUI:

```scheme
(file-rom-bin-load #:run-mode RUN-NONINTERACTIVE
                   #:file "tiles.sfc"
                   #:mode "snes-4bpp")

(file-rom-bin-save #:run-mode RUN-NONINTERACTIVE
                   #:image image
                   #:file "out.sfc"
                   #:mode "snes-4bpp")
```

Available `mode` values: `nes-1bpp`, `nes-2bpp`, `snesgb-2bpp`, `ngpc-2bpp`,
`snes-3bpp`, `gba-4bpp`, `snes-4bpp`, `ggsms-4bpp`, `gens-4bpp`,
`casioloopy-4bpp`, `gba-8bpp`, `snes-8bpp`, `casioloopy-8bpp`.
The extension specific procedures (`.chr`/`.nes`, `.gb`, `.sms`/`.gg`) have the
format fixed and take no `mode` argument.

## Known limitations & Issues:
* Palettes: Does not yet import palettes and defaults to internal standard palettes. Which can then be changed using the GIMP color map and Palette tools.

* Image size: ROMs and tile files that are not an even multiple of tile width will get padded with transparent pixels at the end of the image, and have any trailing data stored as gimp image metadata. The plugin will attempt to preserve original file size and integrity as much as possible. Setting transparent pixels (in tiles) at the end of the image to non-transparent will cause those tiles to get written to the file and therefore increase the file size. Be careful. 


## GIMP usage hints:
* Set paintbrush tool to:
  * Brush: Pixel
  * Opacity: 100%
  * Size: 1

* Set grid to 8x8

* Colormap & Palette changing:
  * Colormap (Palette for current image)  
    * Show the Palette: --> Menu --> Windows --> Dockable Dialogs --> Show Colormap
    * Assign a Palette: --> Menu --> Colors --> Map --> Set Color Map
  * Palette (GIMP system wide palettes available for use)
    * Show the Palettes Dialog: --> Menu --> Windows --> Dockable Dialogs --> Show Palettes
    * Import: Palettes Dialog --> Pop-out menu at top -> Palettes Menu -> Import (must be 768 bytes?)
  * Changing 
    * Assign a palette, convert to RGB and then back to indexed to shrink down and discard unused colors
    1. Menu -> Colors -> Map -> Set Color Map -> Select desired color map (could be imported or from another image)
    2. Menu -> Image -> Mode -> RGB
    3. Menu -> Image -> Mode -> Indexed -> Use Custom Palette + check Remove unused -> Select desired color map (could be imported or from another image)




