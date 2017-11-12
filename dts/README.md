How to Use
==========

1. Copy or symlink desired **dts** file from **repo/**.
2. Type `make` to build.
3. Type `sudo make load` to apply.

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
