REPO     := ./repo
DTSS_ALL := $(notdir $(wildcard $(REPO)/*.dts))
DTSS_EN  := $(wildcard *.dts)
NAME     := $(basename $(DTSS_EN))
DTBO     := $(NAME:=.dtbo)
ENABLES  := $(addprefix enable_,$(basename $(DTSS_ALL)))
DISABLES := $(addprefix disable_,$(basename $(DTSS_EN)))
DUMPS    := $(addprefix dump_,$(basename $(DTSS_EN)))

.PHONY: all clean distclean load unload enable_all disable_all $(ENABLES) $(DISABLES) $(DUMPS)

all: $(DTBO)

# In the future shall remove -W no-unit_address_vs_reg
%.dtbo: %.dts
	dtc -@ -W no-unit_address_vs_reg -I dts -O dtb -o $@ $<

clean:
	rm -f $(DTBO)

distclean: clean disable_all

$(DUMPS):
	@$(MAKE) $(@:dump_%=%).dtbo
	@fdtdump $(@:dump_%=%).dtbo

load: $(DTBO)
	@echo "Checking / mounting configfs"
	@mount | grep configfs || mount -t configfs none /sys/kernel/config
	@for n in $(NAME); do \
	  echo $$n;
	  mkdir -p /sys/kernel/config/device-tree/overlays/$$n; \
	  cp $$n.dtbo /sys/kernel/config/device-tree/overlays/$$n/dtbo; \
	done

unload:
	@echo "WARNING: more likely than not this will not work!"
	@echo "         The patch for dynamic unloading via configfs has not been merged as of kernel v4.9."
	@for n in $(NAME); do \
	  echo $$n;
	  rmdir /sys/kernel/config/device-tree/overlays/$$n; \
	done

$(ENABLES):
	@BASENAME="$(@:enable_%=%)"; \
	ln -sfT "repo/$${BASENAME}.dts" "$${BASENAME}.dts"

$(DISABLES):
	@BASENAME="$(@:disable_%=%)"; \
	if [ -L "$${BASENAME}.dts" ]; then \
	  rm "$${BASENAME}.dts"; \
	  rm -f "$${BASENAME}.dtbo"; \
	else \
	  echo "'$${BASENAME}.dts' is not a symlink, is it your file?"; \
	  exit 1; \
	fi

enable_all: $(ENABLES)
disable_all: clean $(DISABLES)
