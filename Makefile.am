## Process this file with automake to produce Makefile.in

DISTCHECK_CONFIGURE_FLAGS = --enable-gcc-werror

SUBDIRS = doc include src config_data

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = meram.pc

# Extra files to distribute in the source tarball.
EXTRA_DIST = meram.pc.in meram-uninstalled.pc.in
