SBINDIR ?= /usr/sbin
OBJDIR ?= .
DESTDIR ?=

INSTALL ?= install
RM ?= rm
LN ?= ln
SETCAP ?= setcap

CFLAGS ?= -Wall -Wextra -O2 -g
ifneq ($(MAILBOX_PATH),)
	override CFLAGS += -DMAILBOX_PATH="$(MAILBOX_PATH)"
endif

all: $(OBJDIR)/femtomail
.PHONY: all install install-link-sendmail setcap clean

$(OBJDIR)/femtomail: femtomail.c
ifeq ($(USERNAME),)
	$(error USERNAME must be set and non-empty)
endif
	$(CC) -DUSERNAME="$(USERNAME)" $(CFLAGS) -o $(DESTDIR)$@ $<

clean:
	$(RM) $(OBJDIR)/femtomail

install: $(OBJDIR)/femtomail
	$(INSTALL) -m 755 -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 755 $(OBJDIR)/femtomail $(DESTDIR)$(SBINDIR)/femtomail

install-link-sendmail: install
	$(LN) -s femtomail $(DESTDIR)$(SBINDIR)/sendmail

setcap: install
	$(SETCAP) cap_setuid,cap_setgid=ep $(DESTDIR)$(SBINDIR)/femtomail
