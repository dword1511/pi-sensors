.PHONY: all install clean uninstall

KSRC    = /lib/modules/$(shell uname -r)/build/
KMODDIR = /lib/modules/$(shell uname -r)/misc/

KMODS   = ak8975 bcm2835-hwmon bh1750 hmc5843_core hmc5843_i2c hmc5843_spi mlx90614 tmp007 tsl2591 vl6180 iio_collectd iio_hwmon

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
