MCP2221_DIR = $(shell pwd)
KSRC = /lib/modules/$(KVER)/build
KVER = $(shell uname -r)
MODDESTDIR = /lib/modules/$(KVER)/kernel/drivers/i2c/busses

CROSS_COMPILE	=

AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
DEPMOD          = /sbin/depmod
INSTALL			= /usr/bin/install
GREP			= /bin/grep

obj-m += i2c-mcp2221.o

all: modules

modules:
	$(MAKE) -C $(KSRC) M=$(MCP2221_DIR) CC=$(CC) modules

install: modules
	@$(GREP) i2c-mcp2221.ko /lib/modules/$(shell uname -r)/modules.dep && rm -fr $(MODDESTDIR)/i2c-mcp2221.ko || echo "No system i2c-mcp2221.ko file found. Install the new i2c-mcp2221.ko driver into the system"
	$(INSTALL) -p -m 644 i2c-mcp2221.ko $(MODDESTDIR)
	@$(DEPMOD) -a
	
uninstall:
	@$(GREP) i2c-mcp2221.ko /lib/modules/$(shell uname -r)/modules.dep && rm -fr $(MODDESTDIR)/i2c-mcp2221.ko || echo "Remove the i2c-mcp2221.ko driver from the system"
	@$(DEPMOD) -a
	
clean:
	$(MAKE) -C $(KSRC) M=$(MCP2221_DIR) CC=$(CC) clean
