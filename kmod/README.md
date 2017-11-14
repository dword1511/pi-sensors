Kernel Modules for Sensors and Bridging
=======================================

`make` and then `sudo make install`.
I think you all know the drill.

In addition to in-tree and out-of-tree ports listed below,
this repository refers to additional useful resources.
Use `git submodule init` and `git submodule update` to pull them in.
The `mcp2221` driver is from Microchip
([Details](http://www.microchip.com/wwwproducts/en/MCP2221A),
[Download](http://ww1.microchip.com/downloads/en/DeviceDoc/mcp2221_0_1.tar.gz)),
and supports a handy USB-to-I2C bridge for debugging.


`iio-hwmon`
-----------
The `iio-hwmon` is a in-tree kernel that mirros industrial IO channels to `hwmon`.
However, since `hwmon` only support a few types of channels
(voltage, temperature, humidity, fan speed, current, power, energy),
some channels cannot be mapped with this driver,
and probing will result in -22 (`EINVAL`).

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`

`iio-collectd`
--------------
The `iio-collectd` driver is based on the in-tree iio-hwmon driver.
The main goal is to bridge iio devices to collectd via hwmon.
So certain compromises have been made, *e.g.* mapping humidity to voltage.
There are also enhancements such as channel labeling.
Usage in device tree is similar to `iio-hwmon`,
except that `io-channel-names` can be used to override automatically generated labels.

