gimp-rom-bin
===========

GIMP plugin for reading/writing ROM bin image files:
 * NES 2bpp
 * SNES/GB 2bpp
 * SNES 4bpp
 

## Acknowledgement:
 * Source uses some webp gimp plugin code from Nathan Osman (Copyright 2012)
 * Some format documentation from: https://mrclick.zophar.net/TilEd/download/consolegfx.txt


## Quick instructions:

Compile/install it on Unix / Linux using:

```
gimptool-2.0 --install file-rom-binfile.c
    or
 make (and then copy to your GIMP plugin folder)
```

## Known limitations & Issues:
* Import: Requires selecting the correct image file format from the Open dialog instead of a pop-up during import. This is due to the Open dialog preview using the plugin to generate a thumbnail image, and therefore spawning multiple, unwanted import dialogs.

* Palettes: Does not yet import palettes and defaults to internal standard palettes. Which can then be changed using the GIMP color map and Palette tools.

* Image size: Image width and height must be even multiples of 8 (the ROM image tile size, 8x8). If they aren't import and export will fail (quietly, for now).


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




