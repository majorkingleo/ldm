CFLAGS= -g -Wall
LDFLAGS=-lX11
VERSION=0.1
FILES=ldm.cc
EXEC=ldm
DIST=ldm-$(VERSION)
BINARY=$(EXEC)
EXTRAFILES=Makefile COPYING startall.sh startup.sh willing.sh

all:
	echo targets: linux, hpux, aix, clean, dist

linux: $(FILES)
	c++ -o $(BINARY) $(FILES) $(CFLAGS) $(LDFLAGS) -DLINUX -DVERSION='"$(VERSION)"' -O2 -L/usr/X11/lib

aix: $(FILES)
	c++ -o $(BINARY) $(FILES) $(CFLAGS) $(LDFLAGS) -O2 -DAIX

hpux: $(FILES)
	c++ -o $(BINARY) $(FILES) $(CFLAGS) $(LDFLAGS) -Aa -DHPUX -D_HPUX_SOURCE +w1

clean:
	rm -f $(BINARY)

dist: clean
	rm -rf $(DIST)
	rm -rf $(DIST).tar.gz
	mkdir $(DIST)
	cp $(FILES) $(EXTRAFILES) $(DIST)
	tar cf $(DIST).tar $(DIST)
	gzip $(DIST).tar
	rm -rf $(DIST)
	mv $(DIST).tar.gz ..


