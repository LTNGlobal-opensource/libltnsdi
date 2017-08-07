
AUTOMAKE_OPTIONS = foreign
GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags)

BLACKMAGIC_SDK_PATH = $(top_srcdir)/include/decklink-sdk

AM_CPPFLAGS = -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 \
	-D_BSD_SOURCE \
	-I$(BLACKMAGIC_SDK_PATH) \
	-I$(top_srcdir)/src \
	-D__STDC_FORMAT_MACROS \
	-DGIT_VERSION=\"$(GIT_VERSION)\"

AM_CFLAGS = -Wall -O3

LDADD = ../src/libltnsdi.la -lpthread -lz -ldl

if DEBUG
	CFLAGS += -g
endif

CXXFLAGS = $(CFLAGS)

SRC  = ltnsdi_util.c
SRC += $(BLACKMAGIC_SDK_PATH)//DeckLinkAPIDispatch.cpp
SRC += demo.c

bin_PROGRAMS  = ltnsdi_util
bin_PROGRAMS += ltnsdi_demo

ltnsdi_util_SOURCES = $(SRC)
ltnsdi_demo_SOURCES = $(SRC)

libltnsdi_noinst_includedir = $(includedir)

noinst_HEADERS  = hexdump.h
noinst_HEADERS += version.h
