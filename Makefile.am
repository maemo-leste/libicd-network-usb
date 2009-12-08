
SUBDIRS = src etc

EXTRA_DIST = \
	autogen.sh \
	debian/changelog \
	debian/compat \
	debian/control \
	debian/copyright \
	debian/libicd-network-usb.install \
	debian/libicd-network-usb.postinst \
	debian/libicd-network-usb.prerm \
	debian/rules

MAINTAINERCLEANFILES = \
	Makefile.in aclocal.m4 config.guess config.h.in config.sub \
	install-sh ltmain.sh missing

.PHONY: doxygen-doc

doxygen-doc:
if DOXYGEN_DOCS_ENABLED
	@DOXYGEN@ Doxyfile
endif
