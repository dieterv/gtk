include $(top_srcdir)/Makefile.decl

EXTRA_DIST += \
	gdk.rc \
	gdk.rc.in \
	gtk.ico

if USE_WIN32
noinst_DATA = gdk-win32-res.o
endif

.PHONY: gtk.ico
gtk.ico:
	cp $(srcdir)/gtk.ico $(builddir)/gtk.ico

gdk-win32-res.o : gdk.rc gtk.ico
	$(WINDRES) gdk.rc $@

-include $(top_srcdir)/git.mk
