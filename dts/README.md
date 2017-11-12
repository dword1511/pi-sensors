How to Use
==========

1. Type `make enable_some.dts` to enable desired DTS.
   Use bash completion.
   Browse **./repo** for available DTS.
   Do not forget to include `00_i2c1.dts` for I2C sensors.
1. Type `make` to build.
1. Type `sudo make load` to apply.
1. Do not forget to build and load kernel drivers in **../kmod**.

Notes & Lessons
===============
* Split your dts file, so when kernel crashes you know the offending part.
  Also pay attention to sysfs size limits.
* i2c_arm is no longer a thing, it is probably left-over in the tutorial. Use i2c1.
* iio-hwmon can be included at any place in the device tree,
  but must have access to the target phandle within the same overlay.
* iio-hwmon needs modification to support pressure, light intensity, magnet flux, etc.,
  which are absent in hwmon types. Otherwise probing returns EINVAL.
* iio-hwmon needs to parse of_node in order to get io-channel-names.
* dt-overlay unloading does not work in the userspace (yet).
  Currently you need something like BeagleBone's cape manager.
  (That is to say `make unload` will not get you anything.)
