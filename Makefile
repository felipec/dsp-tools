CROSS_COMPILE ?= arm-linux-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -O2 -Wall -Wextra -Wno-unused-parameter

override CFLAGS += -D_GNU_SOURCE

all:

# dsp-manager

dsp-manager: dsp_manager.o dsp_bridge.o log.o

bins += dsp-manager

dsp-load: dsp_load.o dsp_bridge.o

bins += dsp-load

all: $(bins)

# pretty print
V = @
Q = $(V:y=)
QUIET_CC    = $(Q:@=@echo '   CC         '$@;)
QUIET_LINK  = $(Q:@=@echo '   LINK       '$@;)
QUIET_CLEAN = $(Q:@=@echo '   CLEAN      '$@;)
QUIET_DLL   = $(Q:@=@echo '   DLLCREATE  '$@;)

%.o:: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(bins):
	$(QUIET_LINK)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(bins) *.o

install: $(bins)
	mkdir -p $(DESTDIR)/usr/sbin
	install -m 755 dsp-manager $(DESTDIR)/usr/sbin
	mkdir -p $(DESTDIR)/usr/libexec
	install -m 755 scripts/dsp-recover $(DESTDIR)/usr/libexec
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 dsp-load $(DESTDIR)/usr/bin
