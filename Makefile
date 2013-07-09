srcdir ?= .
sbindir ?= /usr/sbin
DESTDIR ?=

INSTALL ?= install
RM ?= rm -f
LN ?= ln
SETCAP ?= setcap

CFLAGS ?= -Wall -Wextra -O2 -g
ifneq ($(MAILBOX_PATH),)
	override CFLAGS += -DMAILBOX_PATH=\"$(MAILBOX_PATH)\"
endif

all: femtomail
.PHONY: all install install-link-sendmail setcap clean uninstall

femtomail: $(srcdir)/femtomail.c
ifeq ($(USERNAME),)
	$(error USERNAME must be set and non-empty)
endif
	$(CC) -DUSERNAME=\"$(USERNAME)\" $(CFLAGS) -o $@ $<

clean:
	$(RM) femtomail

install: femtomail
	$(INSTALL) -m 755 -d $(DESTDIR)$(sbindir)
	$(INSTALL) -m 755 femtomail $(DESTDIR)$(sbindir)/femtomail

install-link-sendmail: install
	$(LN) -s femtomail $(DESTDIR)$(sbindir)/sendmail

uninstall:
	$(RM) $(DESTDIR)$(sbindir)

setcap: install
	$(SETCAP) cap_setuid,cap_setgid=ep $(DESTDIR)$(sbindir)/femtomail
