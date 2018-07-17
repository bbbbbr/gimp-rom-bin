Cross compiling GIMP stable for Windows (mingw32) on Debian Linux 8 in a VM
===========

Many of the how-to docs for cross compiling GIMP binaries for Windows on a Linux host appear to be out of date or broken (crossroad, MinGW-w64). 

The most up-to-date how-to at this time is on the GIMP gitlab site (which also hosts their CI setup, another useful reference). The build from that source (at least for GIMP stable) also has some breakage- just less than others. The site happens to not be indexed by Google, so it's harder to stumble across. 
* https://gitlab.gnome.org/GNOME/gimp/tree/master/build/windows/jhbuild

This is a walk-through of how to set up the build environment, fix the bugs errors that crop up and do a full build of GIMP stable.

Disclaimer: There are better ways to do the plugin building and patching below. This is suitable for now though.


## Set up OS VM

1. Download the pre-built Debian 8.10 virtualbox image  
* https://www.osboxes.org/debian/


## OS and User Prep

1. Make the vm tools cd executable
   * for: "/dev/sr0"  --> Add the ",exec" option
    ```
    nano -w /etc/fstab  
    ``` 

3. Fix the apt sources
    ```
    nano -w /etc/apt/sources.list
    ```
    
    * Use these & comment out the rest : 
    
    ```
    deb http://httpredir.debian.org/debian jessie main contrib
    deb-src http://httpredir.debian.org/ jessie  main contrib

    deb http://httpredir.debian.org/debian jessie-updates main contrib
    deb-src http://httpredir.debian.org/ jessie-updates  main contrib

    deb http://security.debian.org/ jessie/updates main contrib
    deb-src http://security.debian.org/ jessie/updates main contrib
    ```

3. Install sudo and add the default user to the sudo group
    ```
    su
    apt-get install sudo
    sudo usermod -aG sudo osboxes
    exit
    newgrp sudo
    ```

# OS Package Prep
1. Install some basics, required for vm tools
    ```
    sudo apt-get install build-essential module-assistant
    ```

2. There should be a step here for installing the tools via virtual CDROM. :) 

3. Install the remaining packages required for the build
    ```
    sudo apt-get install git
    
    sudo apt-get install build-essential mingw-w64 git jhbuild automake autoconf libtool libgtk2.0-dev ragel intltool bison flex gperf gtk-doc-tools nasm ruby cmake libxml-simple-perl
    ```

## Build Prep

    ```
    cd /home/osboxes; mkdir git; cd git
    
    git clone https://gitlab.gnome.org/GNOME/gimp.git
    
    # Optional: Roll to the specific version that was last used (Revert with: git checkout master)
    git checkout 0fa991fb5f1da2985f60aee90ab98264d54b9363
    
    cd /home/osboxes/git/gimp/build/windows/jhbuild
    ```

## Patch some GIMP jhbuild files

1. Fix bug in old Debian version of jhbuild that causes build to fail (fixed in new jhbuild)
    ```
    sudo cp /usr/share/jhbuild/jhbuild/modtypes/__init__.py /usr/share/jhbuild/jhbuild/modtypes/__init__.py.dist
    nano -w /home/osboxes/git/gimp_patch.jhbuild
    ```
    
    * Paste in patch info
    ```    
    --- /usr/share/jhbuild/jhbuild/modtypes/__init__.py.dist	2018-07-16 14:00:53.642002994 -0400
    +++ /usr/share/jhbuild/jhbuild/modtypes/__init__.py	2018-07-16 14:07:05.397638997 -0400
    @@ -225,7 +225,11 @@
                     else:
                         os.mkdir(dest_path)
                     num_copied += self._process_install_files(installroot, src_path, prefix)
    -                os.rmdir(src_path)
    +                try:
    +                    os.rmdir(src_path)
    +                except OSError:
    +                    # files remaining in buildroot, errors reported below
    +                    pass
                 else:
                     num_copied += 1
                     try:
    ```

    * Apply Patch
    ```
    sudo patch /usr/share/jhbuild/jhbuild/modtypes/__init__.py -i /home/osboxes/git/gimp_patch.jhbuild 
    ```

2. MNG plugin is broken, turn it off
    ```
    nano -w /home/osboxes/git/gimp_patch.mng.build.jhbuildrc
    ```    
    
    * Paste in patch info
    ```    
    --- a/build/windows/jhbuild/build.jhbuildrc
    +++ b/build/windows/jhbuild/build.jhbuildrc
    @@ -216,7 +216,8 @@ module_autogenargs['gegl']       = autogenargs + """ --enable-introspection=no \
                                                          --with-sdl=no"""

     module_autogenargs['gimp-stable']= autogenargs + """ --disable-python \
    -                                                     --disable-gtk-doc """
    +                                                     --disable-gtk-doc \
    +                                                     --without-libmng """

     module_autogenargs['gimp-dev']   = autogenargs + """ --disable-python \
                                                          --disable-gtk-doc """
    ```

    * Apply Patch
    ```
    patch /home/osboxes/git/gimp/build/windows/jhbuild/build.jhbuildrc -i /home/osboxes/git/gimp_patch.mng.build.jhbuildrc
    ```

3. Fix broken fontconfig package link

    ```
    nano -w /home/osboxes/git/gimp_patch.fontconfig.gtk+.moduleset
    ```    
    
    * Paste in patch info
    ```    
    --- gtk+.moduleset.dist	2018-07-16 14:17:32.250673023 -0400
    +++ gtk+.moduleset	2018-07-16 14:18:07.136106813 -0400
    @@ -67,9 +67,9 @@
     </autotools>

     <autotools id="fontconfig" autogen-sh="configure" makeinstallargs="install -j1">
    -	<branch version="2.12.1" repo="freedesktop.org"
    -			module="fontconfig/release/fontconfig-2.12.1.tar.bz2"
    -			hash="sha256:b449a3e10c47e1d1c7a6ec6e2016cca73d3bd68fbbd4f0ae5cc6b573f7d6c7f3">
    +        <branch version="2.12.4" repo="freedesktop.org"
    +                        module="fontconfig/release/fontconfig-2.12.4.tar.bz2"
    +                        hash="sha256:668293fcc4b3c59765cdee5cee05941091c0879edcc24dfec5455ef83912e45c">
      </branch>
      <dependencies>
        <dep package="freetype2"/>
    ```
    
    * Apply Patch
    ```
    patch /home/osboxes/git/gimp/build/windows/jhbuild/gtk+.moduleset -i /home/osboxes/git/gimp_patch.fontconfig.gtk+.moduleset
    ```
    

4. Fix broken isocodes package link
    ```    
    nano -w /home/osboxes/git/gimp_patch.isocodes.misclibs.moduleset
    ```    
    
    * Paste in patch info
    ```    
    --- misclibs.moduleset.orig	2018-07-16 14:20:41.851760995 -0400
    +++ misclibs.moduleset	2018-07-16 14:21:27.703760995 -0400
    @@ -9,7 +9,7 @@
      <repository type="tarball" name="sf.net"
        href="http://downloads.sourceforge.net/project/" />
      <repository type="tarball" name="pkg-isocodes"
    -		href="http://pkg-isocodes.alioth.debian.org/downloads/" />
    +		href="http://ftp.osuosl.org/pub/blfs/conglomeration/iso-codes/" />
      <repository type="tarball" name="poppler"
        href="http://poppler.freedesktop.org/" />
      <repository type="tarball" name="exiv2"
    ```
    
    * Apply Patch
    ```
    patch /home/osboxes/git/gimp/build/windows/jhbuild/misclibs.moduleset -i /home/osboxes/git/gimp_patch.isocodes.misclibs.moduleset
    ```

## Run the build (Stable tree) once

This pulls in the GIMP source and then runs a build. Hopefully it builds all the way through without error

    ```
    cd /home/osboxes/git/gimp/build/windows/jhbuild
    MODULE=gimp-stable ./build
    ```
Wait a while as it builds...


## Get plugin source and graft it onto the gimp build tree

Yeah, it's hackish... 

    ```
    cd /home/osboxes/git/gimp/build/windows/jhbuild/checkout/gimp/plug-ins
    
    git clone https://github.com/bbbbbr/gimp-rom-bin.git
    ```

## Modify the build to add the plugin 
(and disable broken MNG plugin)

1. Add the plugin to configure.ac
    ```    
    nano -w /home/osboxes/git/gimp_patch.gimp.configure.ac
    ```    
    
    * Paste in patch info
    ```    
    --- a/configure.ac
    +++ b/configure.ac
    @@ -2335,6 +2335,7 @@ plug-ins/Makefile
     plug-ins/color-rotate/Makefile
     plug-ins/color-rotate/images/Makefile
     plug-ins/file-bmp/Makefile
    +plug-ins/gimp-rom-bin/src/Makefile
     plug-ins/file-compressor/Makefile
     plug-ins/file-faxg3/Makefile
     plug-ins/file-fits/Makefile
    ```
    
    * Apply Patch
    ```
    patch /home/osboxes/git/gimp/build/windows/jhbuild/checkout/gimp/configure.ac -i /home/osboxes/git/gimp_patch.gimp.configure.ac
    ```

2. Add plugin to automake
    ```    
    nano -w /home/osboxes/git/gimp_patch.gimp.plug-ins.makefile.am
    ```    
    
    * Paste in patch info (mind the tabs!)
    ```    
    --- Makefile.am.orig	2018-07-16 16:08:18.431405702 -0400
    +++ Makefile.am	2018-07-16 16:08:45.731405702 -0400
    @@ -59,6 +59,7 @@
    	$(pygimp)		\
    	color-rotate		\
    	file-bmp		\
    +	gimp-rom-bin/src/       \
    	$(file_compressor)	\
    	file-faxg3		\
    	file-fits		\
    ```
    
    * Apply Patch
    ```
    patch /home/osboxes/git/gimp/build/windows/jhbuild/checkout/gimp/plug-ins/Makefile.am -i /home/osboxes/git/gimp_patch.gimp.plug-ins.makefile.am
    ```

## Run the build and the plugin should now get built
    ```
    cd /home/osboxes/git/gimp/build/windows/jhbuild
    MODULE=gimp-stable ./build
    ```
Wait a while as it builds...

The binary gets placed here :
    ```
    home/osboxes/git/gimp/build/windows/jhbuild/targets/gimp-stable-i686/lib/gimp/2.0/plug-ins/
    ```
    
    
