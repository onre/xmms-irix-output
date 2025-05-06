lib_LTLIBRARIES = libIRIX.la

CFLAGS = @GTK_CFLAGS@ -I../../xmms -O6

libIRIX_la_LDFLAGS = -export-dynamic -avoid-version @GTK_LIBS@ -lpthread
libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libIRIX_la_SOURCES = 	\
IRIX.c IRIX.h \
about.c \
configure.c \
init.c \
audio.c


