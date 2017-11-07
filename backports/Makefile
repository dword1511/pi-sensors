.PHONY: all install clean

obj-m += tmp007.o tsl2591.o iio_hwmon.o hmc5843_core.o hmc5843_i2c.o hmc5843_spi.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
	install -p -m 644 tmp007.ko /lib/modules/$(shell uname -r)/misc
	install -p -m 644 tsl2591.ko /lib/modules/$(shell uname -r)/misc
	install -p -m 644 iio_hwmon.ko /lib/modules/$(shell uname -r)/misc
	install -p -m 644 hmc5843_core.ko /lib/modules/$(shell uname -r)/misc
	install -p -m 644 hmc5843_i2c.ko /lib/modules/$(shell uname -r)/misc
	install -p -m 644 hmc5843_spi.ko /lib/modules/$(shell uname -r)/misc
	depmod -a

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
