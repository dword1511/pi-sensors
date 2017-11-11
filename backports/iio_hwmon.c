/* Hwmon client for industrial I/O devices
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

/**
 * struct iio_hwmon_state - device instance state
 * @channels:		filled with array of channels from iio
 * @num_channels:	number of channels in channels (saves counting twice)
 * @hwmon_dev:		associated hwmon device
 * @attr_group:	the group of attributes
 * @attrs:		null terminated array of attribute pointers.
 */
struct iio_hwmon_state {
	struct iio_channel *channels;
	int num_channels;
	struct device *hwmon_dev;
	struct attribute_group attr_group;
	const struct attribute_group *groups[2];
	struct attribute **attrs;
	int *attr_indices;
};

/*
 * Assumes that IIO and hwmon operate in the same base units.
 * This is supposed to be true, but needs verification for
 * new channel types.
 */
static ssize_t iio_hwmon_read_val(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int result;
	int ret;
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct iio_hwmon_state *state = dev_get_drvdata(dev);

	ret = iio_read_channel_processed(&state->channels[sattr->index],
					&result);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", result);
}

static ssize_t iio_hwmon_read_label(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	struct iio_hwmon_state *state = dev_get_drvdata(dev);
	const char *name = state->channels[sattr->index].channel->extend_name;
	const int is_raw = BIT(IIO_CHAN_INFO_RAW) & state->channels[sattr->index].channel->info_mask_separate;
	const int name_index = state->attr_indices[sattr->index];

	if (name == NULL) {
		switch (state->channels[sattr->index].channel->type) {
		/* NOTE: skipping SI unit for well-defined hwmon type */
		case IIO_VOLTAGE:
			return sprintf(buf, "Voltage %d%s\n", name_index, is_raw ? " RAW" : "");
		case IIO_TEMP:
			return sprintf(buf, "Temperature %d%s\n", name_index, is_raw ? " RAW" : "");
		case IIO_CURRENT:
			return sprintf(buf, "Current %d%s\n", name_index, is_raw ? " RAW" : "");
		case IIO_HUMIDITYRELATIVE:
			return sprintf(buf, "Humidity %d%s\n", name_index, is_raw ? " RAW" : "");
		case IIO_PRESSURE:
			return sprintf(buf, "Pressure %d %s\n", name_index, is_raw ? "RAW" : "kPa");
		case IIO_LIGHT:
			return sprintf(buf, "Light %d %s\n", name_index, is_raw ? "RAW" : "kLx");
		case IIO_INTENSITY:
			return sprintf(buf, "Intensity %d\n", name_index); /* Most likely RAW, for color sensors? */
		case IIO_MAGN:
			return sprintf(buf, "Magnetic flux %d %s\n", name_index, is_raw ? "RAW" : "mT");
		default:
			return sprintf(buf, "Channel %d %s\n", name_index, is_raw ? "RAW" : "SI");
		}
	}

	return sprintf(buf, "%s\n", name);
}

static int iio_hwmon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct iio_hwmon_state *st;
	struct sensor_device_attribute *a, *l;
	int ret, i;
	int in_i = 1, temp_i = 1, curr_i = 1, humidity_i = 1;
	enum iio_chan_type type;
	struct iio_channel *channels;
	const char *name = "iio_hwmon";
	char *sname;

	if (dev->of_node && dev->of_node->name)
		name = dev->of_node->name;

	channels = iio_channel_get_all(dev);
	if (IS_ERR(channels)) {
		if (PTR_ERR(channels) == -ENODEV)
			return -EPROBE_DEFER;
		return PTR_ERR(channels);
	}

	st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
	if (st == NULL) {
		ret = -ENOMEM;
		goto error_release_channels;
	}

	st->channels = channels;

	/* count how many attributes we have */
	while (st->channels[st->num_channels].indio_dev)
		st->num_channels++;

	st->attrs = devm_kzalloc(dev,
				 sizeof(*st->attrs) * (st->num_channels * 2 + 1),
				 GFP_KERNEL);
	if (st->attrs == NULL) {
		ret = -ENOMEM;
		goto error_release_channels;
	}

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
		if (ret < 0)
			goto error_release_channels;

		switch (type) {
		case IIO_VOLTAGE:
			st->attr_indices[i] = in_i;
			a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "in%d_input",
							       in_i);
			l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "in%d_label",
							       in_i++);
			break;
		case IIO_TEMP:
			st->attr_indices[i] = temp_i;
			a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "temp%d_input",
							       temp_i);
			l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "temp%d_label",
							       temp_i++);
			break;
		case IIO_CURRENT:
			st->attr_indices[i] = curr_i;
			a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "curr%d_input",
							       curr_i);
			l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "curr%d_label",
							       curr_i++);
			break;
		case IIO_HUMIDITYRELATIVE:
			st->attr_indices[i] = humidity_i;
			a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "humidity%d_input",
							       humidity_i);
			l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "humidity%d_label",
							       humidity_i++);
			break;
		case IIO_PRESSURE:
		case IIO_LIGHT:
		case IIO_INTENSITY:
		case IIO_MAGN:
			st->attr_indices[i] = in_i;
			a->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "in%d_input",
							       in_i);
			l->dev_attr.attr.name = devm_kasprintf(dev, GFP_KERNEL,
							       "in%d_label",
							       in_i++);
			break;
		default:
			ret = -EINVAL;
			goto error_release_channels;
		}
		if (a->dev_attr.attr.name == NULL) {
			ret = -ENOMEM;
			goto error_release_channels;
		}
		a->dev_attr.show = iio_hwmon_read_val;
		a->dev_attr.attr.mode = S_IRUGO;
		a->index = i;
		l->dev_attr.show = iio_hwmon_read_label;
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
	st->hwmon_dev = hwmon_device_register_with_groups(dev, sname, st,
							  st->groups);
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

static int iio_hwmon_remove(struct platform_device *pdev)
{
	struct iio_hwmon_state *st = platform_get_drvdata(pdev);

	hwmon_device_unregister(st->hwmon_dev);
	iio_channel_release_all(st->channels);

	return 0;
}

static const struct of_device_id iio_hwmon_of_match[] = {
	{ .compatible = "iio-hwmon", },
	{ }
};
MODULE_DEVICE_TABLE(of, iio_hwmon_of_match);

static struct platform_driver __refdata iio_hwmon_driver = {
	.driver = {
		.name = "iio_hwmon",
		.of_match_table = iio_hwmon_of_match,
	},
	.probe = iio_hwmon_probe,
	.remove = iio_hwmon_remove,
};

module_platform_driver(iio_hwmon_driver);

MODULE_AUTHOR("Jonathan Cameron <jic23@kernel.org>");
MODULE_DESCRIPTION("IIO to hwmon driver");
MODULE_LICENSE("GPL v2");
