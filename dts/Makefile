.PHONY: all clean dump install

NAME = mysensors
DTBO = $(NAME).dtbo

all: $(DTBO)

# In the future shall remove -W no-unit_address_vs_reg
%.dtbo: %.dts
	dtc -@ -W no-unit_address_vs_reg -I dts -O dtb -o $@ $<

clean:
	rm -f $(DTBO)

dump: $(DTBO)
	fdtdump $<

install: $(DTBO)
	-mount -t configfs none /sys/kernel/config
	mkdir -p /sys/kernel/config/device-tree/overlays/$(NAME)
	cp $(DTBO) /sys/kernel/config/device-tree/overlays/$(NAME)/dtbo