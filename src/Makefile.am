
AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libltnsdi.la

libltnsdi_la_SOURCES  = ltnsdi.c
libltnsdi_la_SOURCES += audio.c
libltnsdi_la_SOURCES += smpte337_detector.c
libltnsdi_la_SOURCES += klringbuffer.c
libltnsdi_la_SOURCES += smpte338.c

libltnsdi_la_CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" \
	-D_FILE_OFFSET_BITS=64 -O3 \
	-D_BSD_SOURCE -I$(top_srcdir)/include

if DEBUG
  libltnsdi_la_CFLAGS += -g
endif

libltnsdi_includedir = $(includedir)/libltnsdi

libltnsdi_include_HEADERS  = libltnsdi/ltnsdi.h
libltnsdi_include_HEADERS += libltnsdi/smpte337_detector.h
libltnsdi_include_HEADERS += libltnsdi/klringbuffer.h
libltnsdi_include_HEADERS += libltnsdi/smpte338.h

