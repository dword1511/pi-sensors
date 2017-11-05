.PHONY: all clean dump

DTBO = mysensors.dtbo

all: $(DTBO)

# In the future shall remove -W no-unit_address_vs_reg
%.dtbo: %.dts
	dtc -@ -W no-unit_address_vs_reg -I dts -O dtb -o $@ $<

clean:
	rm -f $(DTBO)

dump: $(DTBO)
	fdtdump $<
