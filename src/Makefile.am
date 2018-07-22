## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
file_rom_bin_RC = file-rom-bin.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = file-rom-bin

file_rom_bin_SOURCES = \
	file-rom-bin.c     \
	export-dialog.c    \
	lib_rom_bin.c      \
	read-rom-bin.c     \
	write-rom-bin.c         \
	format_nes_1bpp.c  \
	format_nes_2bpp.c  \
	format_gba_4bpp.c  \
	format_gba_8bpp.c  \
	format_ngp_2bpp.c  \
	format_gens_4bpp.c \
	format_snesgb_2bpp.c    \
	format_snes_3bpp.c    \
	format_snespce_4bpp.c
	format_snes_8bpp.c    \
	format_ggsmswsc_4bpp.c  \
	rom_utils.c        \



INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_rom_bin_RC)
