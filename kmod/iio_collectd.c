/* Bridge between industrial I/O and collectd.
 *
 * Copyright (c) 2017 Chi Zhang <zhangchi866@gmail.com>
 *
 * Based on:
 *
 * Hwmon client for industrial I/O devices
 *
 * Copyright (c) 2011 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/of.h>
#include <linux/hwmon-sysfs.h>
#include <linux/iio/iio.h>
#include <linux/iio/consumer.h>
#include <linux/iio/types.h>

struct iio_collectd_state {
  struct iio_channel *channels;
  int num_channels;
  struct device *hwmon_dev;
  struct attribute_group attr_group;
  const struct attribute_group *groups[2];
  struct attribute **attrs;
  int *attr_indices;
};

static ssize_t iio_collectd_read_val(struct device *dev, struct device_attribute *attr, char *buf) {
  int result;
  int ret;
  struct sensor_device_attribute  *sattr = to_sensor_dev_attr(attr);
  struct iio_collectd_state       *state = dev_get_drvdata(dev);

  ret = iio_read_channel_processed(&state->channels[sattr->index], &result);
  if (ret < 0)
    ret = iio_read_channel_raw(&state->channels[sattr->index], &result);
  if (ret < 0) {
    pr_err("iio_collectd: both iio_read_channel_processed() and iio_read_channel_raw() failed!\n");
    return ret;
  }

  return sprintf(buf, "%d\n", result);
}

/* NOTE: currently does not walk up tree. Should be unnecessary. */
static const char *iio_collectd_of_name(struct device_node *np, int index) {
  const char *s = NULL;
  of_property_read_string_index(np, "io-channel-names", index, &s);
  return s;
}

static ssize_t iio_collectd_read_label(struct device *dev, struct device_attribute *attr, char *buf) {
  struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
  struct iio_collectd_state *state = dev_get_drvdata(dev);
  const char *name = iio_collectd_of_name(dev->of_node, sattr->index);
  const int name_index = state->attr_indices[sattr->index];
  const int raw_only = BIT(IIO_CHAN_INFO_RAW) & state->channels[sattr->index].channel->info_mask_separate &&
                      !(BIT(IIO_CHAN_INFO_PROCESSED) & state->channels[sattr->index].channel->info_mask_separate);

  /* Try channel->extend_name if no name specified in DTS. */
  if (name == NULL) {
    name = state->channels[sattr->index].channel->extend_name;
  }

  /* Try channel->datasheet_name if no name specified in channel->extend_name. */
  if (name == NULL) {
    name = state->channels[sattr->index].channel->datasheet_name;
  }

  /* Generate a name if all previous failed. */
  if (name == NULL) {
    switch (state->channels[sattr->index].channel->type) {
    /* NOTE: skipping SI unit for well-defined hwmon type */
    case IIO_VOLTAGE:
      return sprintf(buf, "Voltage %d%s\n", name_index, raw_only ? " RAW" : "");
    case IIO_TEMP:
      return sprintf(buf, "Temperature %d%s\n", name_index, raw_only ? " RAW" : "");
    case IIO_CURRENT:
      return sprintf(buf, "Current %d%s\n", name_index, raw_only ? " RAW" : "");
    case IIO_HUMIDITYRELATIVE:
      return sprintf(buf, "Humidity %d %s\n", name_index, raw_only ? "RAW" : "RH%");
    case IIO_PRESSURE:
      return sprintf(buf, "Pressure %d %s\n", name_index, raw_only ? "RAW" : "kPa");
    case IIO_LIGHT:
      return sprintf(buf, "Light %d %s\n", name_index, raw_only ? "RAW" : "kLx");
    case IIO_INTENSITY:
      return sprintf(buf, "Intensity %d\n", name_index); /* Most likely RAW, for color sensors? */
    case IIO_MAGN:
      return sprintf(buf, "Magnetic flux %d %s\n", name_index, raw_only ? "RAW" : "mT");
    default:
      return sprintf(buf, "Channel %d %s\n", name_index, raw_only ? "RAW" : "SI");
    }
  } else {
    return sprintf(buf, "%s\n", name);
  }
}

static int iio_collectd_probe(struct platform_device *pdev)
{
  struct device *dev = &pdev->dev;
  struct iio_collectd_state *st;
  struct sensor_device_attribute *a, *l;
  int ret, i;
  int in_i = 1, temp_i = 1, curr_i = 1;
  enum iio_chan_type type;
  struct iio_channel *channels;
  const char *name = "iio_collectd";
  char *sname;

  if (dev->of_node && dev->of_node->name) {
    name = dev->of_node->name;
  }

  channels = iio_channel_get_all(dev);
  if (IS_ERR(channels)) {
    if (PTR_ERR(channels) == -ENODEV) {
      return -EPROBE_DEFER;
    } else {
      return PTR_ERR(channels);
    }
  }

  st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
  if (st == NULL) {
    ret = -ENOMEM;
    goto error_release_channels;
  }

  st->channels = channels;

  /* Count how many attributes we have. */
  while (st->channels[st->num_channels].indio_dev) {
    st->num_channels++;
  }

  /* Each channel has 1 value and 1 label, plus 1 NULL at the end. */
  st->attrs = devm_kzalloc(dev, sizeof(*st->attrs) * (st->num_channels * 2 + 1), GFP_KERNEL);
  if (st->attrs == NULL) {
    ret = -ENOMEM;
    goto error_release_channels;
  }

  /* Store displayed indices of each label. */
  st->attr_indices = devm_kzalloc(dev, sizeof(*st->attr_indices) * st->num_channels, GFP_KERNEL);
  if (st->attr_indices == NULL) {
    ret = -ENOMEM;
    goto error_release_channels;
  }

  for (i = 0; i < st->num_channels; i++) {
    a = devm_kzalloc(dev, sizeof(*a), GFP_KERNEL);
    l = devm_kzalloc(dev, sizeof(*l), GFP_KERNEL);
    if (a == NULL || l == NULL) {
      ret = -ENOMEM;
      goto error_release_channels;
    }

    sysfs_attr_init(&a->dev_attr.attr);
    ret = iio_get_channel_type(&st->channels[i], &type);
    if (ret < 0) {
      goto error_release_channels;
    }

    switch (type) {
      case IIO_VOLTAGE:
      case IIO_HUMIDITYRELATIVE: /* Although humidity is supported by hwmon, collectd has no such support. */
      case IIO_PRESSURE:
      case IIO_LIGHT:
      case IIO_INTENSITY:
      case IIO_MAGN: {
        st->attr_indices[i] = in_i;
        a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "in%d_input", in_i);
        l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "in%d_label", in_i ++);
        break;
      }
      case IIO_TEMP: {
        st->attr_indices[i] = temp_i;
        a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "temp%d_input", temp_i);
        l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "temp%d_label", temp_i++);
        break;
      }
      case IIO_CURRENT: {
        st->attr_indices[i] = curr_i;
        a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "curr%d_input", curr_i);
        l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL, "curr%d_label", curr_i++);
        break;
      }
      default: {
        pr_err("%s: iio type %d is currently unsupported.\n", name, type);
        ret = -EINVAL;
        goto error_release_channels;
      }
    }

    if (a->dev_attr.attr.name == NULL) {
      ret = -ENOMEM;
      goto error_release_channels;
    }

    a->dev_attr.show = iio_collectd_read_val;
    a->dev_attr.attr.mode = S_IRUGO;
    a->index = i;

    l->dev_attr.show = iio_collectd_read_label;
    l->dev_attr.attr.mode = S_IRUGO;
    l->index = i;

    st->attrs[i * 2] = &a->dev_attr.attr;
    st->attrs[i * 2 + 1] = &l->dev_attr.attr;
  }

  st->attr_group.attrs = st->attrs;
  st->groups[0] = &st->attr_group;

  sname = devm_kstrdup(dev, name, GFP_KERNEL);
  if (!sname) {
    ret = -ENOMEM;
    goto error_release_channels;
  }

  strreplace(sname, '-', '_');
  st->hwmon_dev = hwmon_device_register_with_groups(dev, sname, st, st->groups);
  if (IS_ERR(st->hwmon_dev)) {
    ret = PTR_ERR(st->hwmon_dev);
    goto error_release_channels;
  }

  platform_set_drvdata(pdev, st);
  return 0;

error_release_channels:
  iio_channel_release_all(channels);
  return ret;
}

static int iio_collectd_remove(struct platform_device *pdev) {
  struct iio_collectd_state *st = platform_get_drvdata(pdev);

  hwmon_device_unregister(st->hwmon_dev);
  iio_channel_release_all(st->channels);

  return 0;
}

static const struct of_device_id iio_collectd_of_match[] = {
  { .compatible = "iio-collectd", },
  { }
};
MODULE_DEVICE_TABLE(of, iio_collectd_of_match);

static struct platform_driver __refdata iio_collectd_driver = {
  .driver = {
    .name = "iio_collectd",
    .of_match_table = iio_collectd_of_match,
  },
  .probe = iio_collectd_probe,
  .remove = iio_collectd_remove,
};

module_platform_driver(iio_collectd_driver);

MODULE_AUTHOR("Chi Zhang <zhangchi866@gmail.com>");
MODULE_AUTHOR("Jonathan Cameron <jic23@kernel.org>");
MODULE_DESCRIPTION("IIO to collectd bridge");
MODULE_LICENSE("GPL v2");
