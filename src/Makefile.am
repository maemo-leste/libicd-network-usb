MAINTAINERCLEANFILES = \
	Makefile.in

INCLUDES = \
	@GLIB_CFLAGS@ \
	@GCONF_CFLAGS@ \
	@ICD2_CFLAGS@ \
	@OSSO_IC_DEV_CFLAGS@

LDFLAGS = -avoid-version

plugindir = @ICD_PLUGIN_DIR@

plugin_LTLIBRARIES = \
	libicd_network_usb.la

libicd_network_usb_la_SOURCES = \
	libicd_network_usb.h libicd_network_usb.c
