# The GPL applies to this program.
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL.  If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so.  If you
# do not wish to do so, delete this exception statement from your
# version.  If you delete this exception statement from all source
# files in the program, then also delete it here.
# $Revision$

-include makefile.inc

# *** configure script ***
# support for tcp fast open?
#TFO=yes
# disable SSL? (no = disable so the default is use openssl)
# SSL=no
# enable NCURSES interface?
#NC=yes
# do fft in ncurses interface? (requires libfftw3)
#FW=yes

############# do not change anything below here #############

include version

TARGET=httping

DEBUG=yes
WFLAGS=-Wall -W
OFLAGS=
CFLAGS+=$(WFLAGS) $(OFLAGS) -DVERSION=\"$(VERSION)\"
LDFLAGS+=-lm

PACKAGE=$(TARGET)-$(VERSION)
PREFIX?=/usr
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
DOCDIR=$(PREFIX)/share/doc/$(TARGET)

INSTALL=install
INSTALLDIR=$(INSTALL) -m 0755 -d
INSTALLBIN=$(INSTALL) -m 0755
INSTALLMAN=$(INSTALL) -m 0644
INSTALLDOC=$(INSTALL) -m 0644
STRIP=/usr/bin/strip
RMDIR=/bin/rm -rf
MKDIR=/bin/mkdir
ARCHIVE=/bin/tar cf -
COMPRESS=/bin/gzip -9

OBJS=gen.o http.o io.o str.o error.o utils.o main.o tcp.o res.o socks5.o kalman.o cookies.o

MANS=httping.1

DOCS=license.txt license.OpenSSL readme.txt

ifeq ($(SSL),no)
CFLAGS+=-DNO_SSL
else
OBJS+=mssl.o
LDFLAGS+=-lssl -lcrypto
endif

ifeq ($(TFO),yes)
CFLAGS+=-DTCP_TFO
endif

ifeq ($(NC),yes)
CFLAGS+=-DNC
OBJS+=nc.o
LDFLAGS+=-lncurses
endif

ifeq ($(FW),yes)
CFLAGS+=-DFW
OBJS+=fft.o
LDFLAGS+=-lfftw3
endif

ifeq ($(DEBUG),yes)
CFLAGS+=-D_DEBUG -ggdb
LDFLAGS+=-g
endif

ifeq ($(ARM),yes)
CC=arm-linux-gcc
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(WFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)
	#
	# Oh, blatant plug: http://www.vanheusden.com/wishlist.php

install: $(TARGET)
	$(INSTALLDIR) $(DESTDIR)/$(BINDIR)
	$(INSTALLBIN) $(TARGET) $(DESTDIR)/$(BINDIR)
	$(INSTALLDIR) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLMAN) $(MANS) $(DESTDIR)/$(MANDIR)/man1
	$(INSTALLDIR) $(DESTDIR)/$(DOCDIR)
	$(INSTALLDOC) $(DOCS) $(DESTDIR)/$(DOCDIR)
ifneq ($(DEBUG),yes)
	$(STRIP) $(DESTDIR)/$(BINDIR)/$(TARGET)
endif

makefile.inc:
	./configure

clean:
	$(RMDIR) $(OBJS) $(TARGET) *~ core cov-int

distclean: clean
	rm -f makefile.inc

package:
	# source package
	$(RMDIR) $(PACKAGE)*
	$(MKDIR) $(PACKAGE)
	$(INSTALLDOC) *.c *.h configure Makefile version $(MANS) $(DOCS) $(PACKAGE)
	$(INSTALLBIN) configure $(PACKAGE)
	$(ARCHIVE) $(PACKAGE) | $(COMPRESS) > $(PACKAGE).tgz
	$(RMDIR) $(PACKAGE)

check:
	cppcheck -v --force -j 3 --enable=all --std=c++11 --inconclusive -I. . 2> err.txt
	#
	make clean
	./configure --with-tfo --with-ncurses --with-openssl --with-fftw3
	scan-build make

coverity: makefile.inc
	make clean
	rm -rf cov-int
	CC=gcc cov-build --dir cov-int make all
	tar vczf ~/site/coverity/httping.tgz README cov-int/
	putsite -q
	/home/folkert/.coverity-hp.sh
