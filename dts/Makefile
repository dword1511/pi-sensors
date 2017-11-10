.PHONY: all clean dump load unload

NAME = $(basename $(wildcard *.dts))
DTBO = $(NAME:=.dtbo)

all: $(DTBO)

# In the future shall remove -W no-unit_address_vs_reg
%.dtbo: %.dts
	dtc -@ -W no-unit_address_vs_reg -I dts -O dtb -o $@ $<

clean:
	rm -f $(DTBO)

dump: $(DTBO)
	for n in $(DTBO); do \
		fdtdump $$n; \
	done

load: $(DTBO)
	@echo "NOTE: this shuould be called after modules are loaded."
	mount | grep configfs || mount -t configfs none /sys/kernel/config
	for n in $(NAME); do \
		mkdir -p /sys/kernel/config/device-tree/overlays/$$n; \
		cp $(DTBO) /sys/kernel/config/device-tree/overlays/$$n/dtbo; \
	done

unload:
	for n in $(NAME); do \
		rmdir /sys/kernel/config/device-tree/overlays/$$n; \
	done
