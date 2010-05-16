CROSS_COMPILE ?= arm-linux-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -O2 -Wall -Wextra -Wno-unused-parameter -std=c99

override CFLAGS += -D_GNU_SOURCE

ifndef OLD
  override CFLAGS += -DNEW_API
endif

ifdef DEBUG
  override CFLAGS += -DDEBUG
endif

version := $(shell ./get-version)

all:

dsp-load: dsp_load.o dsp_bridge.o
bins += dsp-load

dsp-probe: dsp_probe.o dsp_bridge.o log.o
bins += dsp-probe

dsp-test: dsp_test.o dsp_bridge.o log.o
bins += dsp-test

all: $(bins)

D = $(DESTDIR)

# pretty print
ifndef V
QUIET_CC    = @echo '   CC         '$@;
QUIET_LINK  = @echo '   LINK       '$@;
QUIET_CLEAN = @echo '   CLEAN      '$@;
endif

%.o:: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) -MMD -o $@ -c $<

$(bins):
	$(QUIET_LINK)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(QUIET_CLEAN)$(RM) $(bins) *.o *.d

install: $(bins)
	install -m 755 dsp-load -D $(D)/usr/bin/dsp-load
	install -m 755 dsp-probe -D $(D)/usr/bin/dsp-probe
	install -m 755 dsp-test -D $(D)/usr/bin/dsp-test

dist: base := dsp-tools-$(version)
dist:
	git archive --format=tar --prefix=$(base)/ HEAD > /tmp/$(base).tar
	mkdir -p $(base)
	echo $(version) > $(base)/.version
	chmod 664 $(base)/.version
	tar --append -f /tmp/$(base).tar --owner root --group root $(base)/.version
	rm -r $(base)
	gzip /tmp/$(base).tar

-include *.d
