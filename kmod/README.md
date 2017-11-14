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


`ak8975`
--------
The `ak8975` driver provides support for Asahi Kasei's `AK8975` Hall-effect 3-axis magnetometers.
This electronic compass is found on the auxiliary I2C bus on InvenSense's `MPU-9125` IMU.
Driver for the IMU itself (`inv-mpu6050-i2c`) is already included in the Raspbian distribution.

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/magnetometer/ak8975.txt)

[Documentation for `MPU-9150` and how to enable the auxiliary I2C bus](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/imu/inv_mpu6050.txt)

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


Example `/etc/modules` Configuration
------------------------------------
```
# Select and add the following to /etc/modules, so they can load automatically after boot:

i2c-bcm2835
i2c-dev

ak8975
bh1750
bmp280_i2c
hmc5843_i2c
htu21
inv-mpu6050-i2c
jc42
mlx90614
tmp007
tsl2591
vl6180

iio_hwmon
iio_collectd
```

Example `collectd.conf` Snippet
-------------------------------
```
Interval 5

# Record data on remote machine so Pi's sdcard does not wear out.

LoadPlugin network
#LoadPlugin rrdtool
LoadPlugin sensors

<Plugin network>
  Server "some.server" "25826"
  ReportStats true
</Plugin>

<Plugin sensors>
  UseLabels true
</Plugin>
```
