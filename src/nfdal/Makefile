#
#
#
#

CROSS_COMPILE = /opt/aarch64-ca53-linux-gnueabihf-8.4.01/bin/aarch64-ca53-linux-gnu-

AS      =$(CROSS_COMPILE)as
LD      =$(CROSS_COMPILE)ld
CC      =$(CROSS_COMPILE)gcc
CPP     =$(CC) -E
AR      =$(CROSS_COMPILE)ar
NM      =$(CROSS_COMPILE)nm
STRIP   =$(CROSS_COMPILE)strip
OBJDUMP =$(CROSS_COMPILE)objdump
RANLIB	=$(CROSS_COMPILE)ranlib


TOPDIR = $(shell pwd)
TARGET = nfdal

-include ../make.env
-include ../Makefile.Rules

#TARGET_ROOT ?= /home/nskim/filesys_otm
#NFDVR = $(TARGET_ROOT)/NFDVR_OTM
TARGET_ROOT ?= /home/parangi/filesys_snf
NFDVR = $(TARGET_ROOT)/NFDVR

LIBDIR = -L$(NFDVR)/gnulib -L$(NFDVR)/lib
LIBDIR += -L$(NFDVR)/lib/gobj/lib \
			-L$(TARGET_ROOT)/usr/local/lib \
			-L$(TARGET_ROOT)/usr/lib
			
INCLUDE = -I$(TOPDIR)/ -I$(NFDVR)/src/include -I$(NFDVR)/src/include/gui
INCLUDE += -I$(TARGET_ROOT)/usr/local/include/ \
			-I$(TARGET_ROOT)/usr/local/include/gtk-2.0 \
			-I$(TARGET_ROOT)/usr/local/lib/gtk-2.0/include \
			-I$(TARGET_ROOT)/usr/local/include/freetype2 \
			-I$(TARGET_ROOT)/usr/local/include/libpng12 \
			-I$(TARGET_ROOT)/usr/local/include/atk-1.0 \
			-I$(TARGET_ROOT)/usr/local/include/pango-1.0 \
			-I$(TARGET_ROOT)/usr/local/include/cairo \
			-I$(TARGET_ROOT)/usr/local/include/glib-2.0 \
			-I$(TARGET_ROOT)/usr/local/lib/glib-2.0/include \
			-I$(TARGET_ROOT)/usr/local/include/libglade-2.0 \
			-I$(TARGET_ROOT)/usr/local/include/libxml2 

export LD_LIBRARY_PATH=/opt/aarch64-ca53-linux-gnueabihf-8.4.01/lib
CFLAGS 	:= -march=armv8-a -mtune=cortex-a53 -ftree-vectorize  $(INCLUDE)

MACRO	+= $(RULES_MACRO)

SRCS := $(wildcard *.c) #scomm.o
OBJECTS := $(patsubst %.c,%.o, $(wildcard *.c))


%o: %.c
	@echo ""
	@echo "Y"
	@echo "================================================================================"
	@echo " Compiling.....	[$<] $(MACRO)"
	@echo "================================================================================"
	@echo ""
	$(CC) -c -o $(CFLAGS) $(MACRO) $@ $<

all :	$(TARGET)

nfdal:
	@echo ""
	@echo "X"
	@echo "================================================================================"
	@echo " Compiling.....	[$<] $(MACRO)"
	@echo "================================================================================"
	@echo ""
	$(CC) -c $(CFLAGS) $(SRCS) $(LIBDIR) $(MACRO)
	$(AR) r libnfdal.a $(OBJECTS)
	$(RANLIB) libnfdal.a
	$(AR) rs libnfdal.a $(OBJECTS)

debug:
	$(CC) -c $(CFLAGS) $(SRCS) $(MACRO) -D_DEBUG
	$(AR) r libnfdal.a $(OBJECTS)
	$(RANLIB) libnfdal.a
	$(AR) rs libnfdal.a $(OBJECTS)

install:
	cp libnfdal.a $(NFDVR)/src/lib
	cp *.h $(NFDVR)/src/include/gui

clean:
	rm -rf test
	rm -rf *.so *.a
	rm -rf $(OBJECTS)
	
