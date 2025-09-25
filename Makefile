CC = gcc
CFLAGS = -Wall -O2
LIBS = -lX11 

BIN = scwm scwmctl

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: scwm scwmctl

scwm: scwm.c
	$(CC) $(CFLAGS) -g scwm.c ewmh.c $(LIBS) -o scwm

scwmctl: scwmctl.c
	$(CC) $(CFLAGS) -o scwmctl scwmctl.c $(LIBS)

clean: 
	rm -f $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)

.PHONY: all clean install uninstall
