.PHONY: all install clean

KSRC    = /lib/modules/$(shell uname -r)/build/
KMODDIR = /lib/modules/$(shell uname -r)/misc/

KMODS   = tmp007 tsl2591 hmc5843_core hmc5843_i2c hmc5843_spi iio_hwmon

KMODSRC = $(KMODS:=.c)
KOS     = $(KMODS:=.ko)

obj-m  += $(KMODS:=.o)

all: $(KMODSRC)
	make -C $(KSRC) M=$(PWD) modules

install: $(KOS)
	mkdir -p $(KMODDIR)
	for ko in $(KOS); do install -p -m 644 $$ko $(KMODDIR); done
	depmod -a

uninstall:
	if [ -d $(KMODDIR) ]; then for ko in $(KOS); do rm -f $(KMODDIR)/$$ko; done; fi

clean:
	make -C $(KSRC) M=$(PWD) clean
