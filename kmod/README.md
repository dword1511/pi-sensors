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

`collectd` currently has no industrial IO support.



`ads1015`
---------
The `ads1015` is an in-tree `hwmon` driver for TI's `ADS1015` 4-channel ADC,
and is already included in the Raspbian distribution.
Thus, its code is not included in this repository.
It is unclear whether later this driver will be ported to the industrial IO section,
which seems more suitable for a multi-channel ADC with PGA.

**Test is in progress.**

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/hwmon/ads1015.txt)


`ak8975` (useful with `MPU-9125`)
---------------------------------
The `ak8975` driver is in-tree and provides support for Asahi Kasei's `AK8975` Hall-effect 3-axis magnetometers.
This electronic compass is found on the auxiliary I2C bus on InvenSense's `MPU-9125` IMU.
Driver for the IMU itself (`inv-mpu6050-i2c`) is already included in the Raspbian distribution.

**Test is in progress.**

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/magnetometer/ak8975.txt)

[Documentation for `MPU-9150` and how to enable the auxiliary I2C bus](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/imu/inv_mpu6050.txt)


`bcm2835-hwmon`
---------------
The `bcm2835-hwmon` is a legacy `hwmon` driver found in Pi's 3.x kernel branch.
It was replaced by `bcm2835-thermal` driver, which ditches the `hwmon` interface.
However, from time to time we need a `hwmon` driver for Pi's CPU temperature.
It is hence included here.

**Test is in progress.**

Source: `https://github.com/raspberrypi/linux/tree/rpi-3.19.y`

Version: 108e489c19d92800dfc7f04ec93f57468ffff751


`bh1750`
--------
The `bh1750` is an in-tree driver for Rohm's unassuming
`BH1710` / `BH1715` / `BH1721` / `BH1750` / `BH1751` ambient light sensors.
It has got one single channel and there is not much trick to it.
Kernel developers do not even border to write documentation for it.

**Test is in progress.**

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`


`bmp280_i2c`
------------
The `bmp280_i2c` is the latest in-tree driver for Bosch's `BMP085`, `BMP180`, `BMP280` and `BME280` barometric pressure sensors.
All of them comes with temperature sensors, but on `BMP085` and `BMP180` the resolution is not so great.
`BME280` also provides humidity, thus it has 3 channels while all the others have 2.
The driver automatically detects sensor types.
It will always throw some error in the kernel log if interrupt pin is undefined, but will then work perfectly.
It is already included in the Raspbian distribution, so its code is not included in this repository.

**Tested with DTS on a Pi 2.**

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/pressure/bmp085.txt)


`hmc5843_i2c` and `hmc5843_spi`
-------------------------------
The `hmc5843` driver is in-tree and supports Honeywell's magneto-resistive magnetometers.
The most famous is probably the `HMC5883L`, which is easily accessible on Amazon,
and is much more sensitive / low-noise than its Hall-effect-based counterparts.
Other supported parts include, of course, `HMC5843`, and `HMC5983`.

The `hmc5843_core` part has been modified so the micro-Tesla output fits `hwmon`'s voltage,
which is a twisted way to make it work with `collectd`.

**Tested with DTS on a Pi 2.**

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/magnetometer/hmc5843.txt)


`htu21`
-------
The `htu21` is an in-tree driver, and is already included in the Raspbian distribution.
Thus, its code is not included in this repository.
It supports Measurement Specialist's `HTU21D` temperature and humidity sensor.
Resolution is decent.

**Tested with DTS on a Pi 2.**

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/humidity/htu21.txt)


`inv-mpu6050-i2c`
-----------------
The `inv-mpu6050` is an in-tree driver, and is already included in the Raspbian distribution.
Thus, its code is not included in this repository.
It supports InvenSense's `MPU6050`, `MPU6500`, `MPU9150`, `MPU9250` and `ICM20608` IMU,
with built-in 3-axis accelerometers and gyroscopes.
These sensors sometimes have a secondary I2C bus connecting to an internal compass.
Please see the datasheet for what is included,
and the corresponding compass driver's documentation for setting the compass up.

**Test is in progress.**

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/imu/inv_mpu6050.txt)


`jc42`
------
The `jc42` is an in-tree `hwmon` driver, and is already included in the Raspbian distribution.
Thus, its code is not included in this repository.
It provides support for JEDEC JC-42.4 compatible temperature sensors,
which is a long list of sensors including Microchip's precision temperature sensor `MCP9808`.
These sensors are originally intended for monitoring memory modules,
some of them even have integrated 24xx EEPROMs for SPD data.
See the documentation for a complete list of supported sensors.
On a PC this driver would automatically probe all I2C buses (maybe unsafe, who knows), and it works fine.
On the Pi it seems to require device tree overlay to function.
Unfortunately, all my trials ended up with -22 (`EINVAL`) during probing.
Manual probing by writing to `/sys/bus/i2c/devices/i2c-1/new_device` works perfectly, however.

**Tested with manual probing on a Pi 2. Test with DTS is in progress.**

[Documentation and supported devices](https://www.kernel.org/doc/Documentation/devicetree/bindings/hwmon/jc42.txt)


`mlx90614`
----------
The `mlx90614` is an in-tree driver supporting Melexis' `MLX90614` contactless temperature sensor.
This sensor has single zone and dual zone variants,
the driver supports both of them and can automatically detect them.
All variants also measures local temperature as most remote temperature sensors do.
However, when it comes to device tree you need to be careful,
since the number of channels is different.

**Test is in progress.**

Source: git://git.kernel.org/pub/scm/linux/kernel/git/jic23/parrot.git

Version: 362f6b3329cd8e799a298961b934ac4e839f0a7d

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/temperature/mlx90614.txt)


`tmp007`
--------
The `tmp007` is an in-tree driver, and is already included in the Raspbian distribution.
Thus, its code is not included in this repository.
It measures both local and remote temperatures as most remote temperature sensors do.
Remote temperature measurement can get significant interference (~2 Celsius)
when local temperature changes rapidly due to airflow.
So relatively stable environment is recommended.
You may shroud the sensor but be aware of its FoV.

**Tested with DTS on a Pi 2.**

Source: git://git.kernel.org/pub/scm/linux/kernel/git/jic23/parrot.git

Version: 362f6b3329cd8e799a298961b934ac4e839f0a7d

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/temperature/tmp007.txt)


`tsl2591`
---------
The `tsl2591` is an out-of-tree driver for AMS's `TSL2591` IR and visible ambient light sensor.
No documentation is available.

The default sensitivity has been modified from `0` to `1` as I found it much more suitable for indoor use.
However, if you are leaving the sensor in some place dark or with direct sunlight,
consider changing this value, or configure it dynamically in runtime.

**Tested with DTS on a Pi 2.**

Source: git://git.kernel.org/pub/scm/linux/kernel/git/jic23/parrot.git

Version: 362f6b3329cd8e799a298961b934ac4e839f0a7d


`vl6180`
--------
The `vl6180` is an in-tree driver for ST's revolutionary `VL6180X` time-of-flight (ToF) proximity sensor.
Inside are an ambient light sensor and a laser range finder.
The range is limited (tens of centimeters), but its successor `VL53L0X` is much better marketed,
since these sensors are most useful for camera focus assist and decimeter range would be insufficient.

**Test is in progress.**

Source: git://git.kernel.org/pub/scm/linux/kernel/git/jic23/parrot.git

Version: 362f6b3329cd8e799a298961b934ac4e839f0a7d

[Documentation](https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/light/vl6180.txt)



`iio-hwmon`
-----------
The `iio-hwmon` is a in-tree kernel that mirrors industrial IO channels to `hwmon`.
However, since `hwmon` only support a few types of channels
(voltage, temperature, humidity, fan speed, current, power, energy),
some channels cannot be mapped with this driver,
and probing will result in -22 (`EINVAL`).

Source: `git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`

Version: `2d6349944d967129c1da3c47287376f10121dbe1`


`iio-collectd`
--------------
The `iio-collectd` driver is based on the in-tree `iio-hwmon` driver.
The main goal is to bridge `iio` devices to `collectd` via `hwmon`.
So certain compromises have been made, *e.g.* mapping humidity to voltage.
There are also enhancements such as channel labeling.
Usage in device tree is similar to `iio-hwmon`,
except that `io-channel-names` can be used to override automatically generated labels.

**Please kindly report bugs to this repository.**



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
