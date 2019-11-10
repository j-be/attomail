srcdir ?= .
sbindir ?= /usr/sbin
DESTDIR ?=

INSTALL ?= install
RM ?= rm -f
LN ?= ln
SETCAP ?= setcap
MKDIR ?= mkdir -p

CFLAGS ?= -Wall -Wextra -O2 -g
ifneq ($(MAILBOX_PATH),)
	override CFLAGS += -DMAILBOX_PATH=\"$(MAILBOX_PATH)\"
endif

femtomail: $(srcdir)/femtomail.c
ifeq ($(USERNAME),)
	$(error USERNAME must be set and non-empty)
endif
	$(MKDIR) target
	$(CC) -DUSERNAME=\"$(USERNAME)\" $(CFLAGS) -o target/$@ $<

clean:
	$(RM) target/femtomail
