CC := arm-linux-gcc
CFLAGS := -ggdb -Wall -Wextra

all:

# dsp-manager

dsp-manager: dsp_manager.o dsp_bridge.o log.o

bins += dsp-manager

all: $(bins)

clean:
	$(RM) $(bins) *.o

install: $(bins)
	mkdir -p $(DESTDIR)/usr/sbin
	install -m 755 dsp-manager $(DESTDIR)/usr/sbin
	mkdir -p $(DESTDIR)/usr/libexec
	install -m 755 scripts/dsp-recover $(DESTDIR)/usr/libexec

%.o:: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(bins):
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
