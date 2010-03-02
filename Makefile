CROSS_COMPILE ?= arm-linux-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -O2 -Wall -Wextra -Wno-unused-parameter

override CFLAGS += -D_GNU_SOURCE

ifdef NEW
  override CFLAGS += -DNEW_API
endif

all:

# dsp-manager

dsp-manager: dsp_manager.o dsp_bridge.o log.o
bins += dsp-manager

dsp-load: dsp_load.o dsp_bridge.o
bins += dsp-load

dsp-probe: dsp_probe.o dsp_bridge.o log.o
bins += dsp-probe

dsp-ping: dsp_ping.o dsp_bridge.o log.o
bins += dsp-ping

all: $(bins)

D = $(DESTDIR)

# pretty print
ifndef V
QUIET_CC    = @echo '   CC         '$@;
QUIET_LINK  = @echo '   LINK       '$@;
QUIET_CLEAN = @echo '   CLEAN      '$@;
endif

%.o:: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(bins):
	$(QUIET_LINK)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(QUIET_CLEAN)$(RM) $(bins) *.o

install: $(bins)
	install -m 755 dsp-manager -D $(D)/usr/sbin/dsp-manager
	install -m 755 scripts/dsp-recover -D $(D)/usr/libexec/dsp-recover
	install -m 755 dsp-load -D $(D)/usr/bin/dsp-load
	install -m 755 dsp-probe -D $(D)/usr/bin/dsp-probe
	install -m 755 dsp-ping -D $(D)/usr/bin/dsp-ping
