/*
 * vl6180.c - Support for STMicroelectronics VL6180 ALS, range and proximity
 * sensor
 *
 * Copyright 2017 Peter Meerwald-Stadler <pmeerw@pmeerw.net>
 * Copyright 2017 Manivannan Sadhasivam <manivannanece23@gmail.com>
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * IIO driver for VL6180 (7-bit I2C slave address 0x29)
 *
 * Range: 0 to 100mm
 * ALS: < 1 Lux up to 100 kLux
 * IR: 850nm
 *
 * TODO: irq, threshold events, continuous mode, hardware buffer
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/version.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define VL6180_DRV_NAME "vl6180"

/* Device identification register and value */
#define VL6180_MODEL_ID	0x000
#define VL6180_MODEL_ID_VAL 0xb4

/* Configuration registers */
#define VL6180_INTR_CONFIG 0x014
#define VL6180_INTR_CLEAR 0x015
#define VL6180_OUT_OF_RESET 0x016
#define VL6180_HOLD 0x017
#define VL6180_RANGE_START 0x018
#define VL6180_ALS_START 0x038
#define VL6180_ALS_GAIN 0x03f
#define VL6180_ALS_IT 0x040

/* Status registers */
#define VL6180_RANGE_STATUS 0x04d
#define VL6180_ALS_STATUS 0x04e
#define VL6180_INTR_STATUS 0x04f

/* Result value registers */
#define VL6180_ALS_VALUE 0x050
#define VL6180_RANGE_VALUE 0x062
#define VL6180_RANGE_RATE 0x066

/* bits of the RANGE_START and ALS_START register */
#define VL6180_MODE_CONT BIT(1) /* continuous mode */
#define VL6180_STARTSTOP BIT(0) /* start measurement, auto-reset */

/* bits of the INTR_STATUS and INTR_CONFIG register */
#define VL6180_ALS_READY BIT(5)
#define VL6180_RANGE_READY BIT(2)

/* bits of the INTR_CLEAR register */
#define VL6180_CLEAR_ERROR BIT(2)
#define VL6180_CLEAR_ALS BIT(1)
#define VL6180_CLEAR_RANGE BIT(0)

/* bits of the HOLD register */
#define VL6180_HOLD_ON BIT(0)

/* default value for the ALS_IT register */
#define VL6180_ALS_IT_100 0x63 /* 100 ms */

/* values for the ALS_GAIN register */
#define VL6180_ALS_GAIN_1 0x46
#define VL6180_ALS_GAIN_1_25 0x45
#define VL6180_ALS_GAIN_1_67 0x44
#define VL6180_ALS_GAIN_2_5 0x43
#define VL6180_ALS_GAIN_5 0x42
#define VL6180_ALS_GAIN_10 0x41
#define VL6180_ALS_GAIN_20 0x40
#define VL6180_ALS_GAIN_40 0x47

struct vl6180_data {
	struct i2c_client *client;
	struct mutex lock;
};

enum { VL6180_ALS, VL6180_RANGE, VL6180_PROX };

/**
 * struct vl6180_chan_regs - Registers for accessing channels
 * @drdy_mask:			Data ready bit in status register
 * @start_reg:			Conversion start register
 * @value_reg:			Result value register
 * @word:			Register word length
 */
struct vl6180_chan_regs {
	u8 drdy_mask;
	u16 start_reg, value_reg;
	bool word;
};

static const struct vl6180_chan_regs vl6180_chan_regs_table[] = {
	[VL6180_ALS] = {
		.drdy_mask = VL6180_ALS_READY,
		.start_reg = VL6180_ALS_START,
		.value_reg = VL6180_ALS_VALUE,
		.word = true,
	},
	[VL6180_RANGE] = {
		.drdy_mask = VL6180_RANGE_READY,
		.start_reg = VL6180_RANGE_START,
		.value_reg = VL6180_RANGE_VALUE,
		.word = false,
	},
	[VL6180_PROX] = {
		.drdy_mask = VL6180_RANGE_READY,
		.start_reg = VL6180_RANGE_START,
		.value_reg = VL6180_RANGE_RATE,
		.word = true,
	},
};

static int vl6180_read(struct i2c_client *client, u16 cmd, void *databuf,
		       u8 len)
{
	__be16 cmdbuf = cpu_to_be16(cmd);
	struct i2c_msg msgs[2] = {
		{ .addr = client->addr, .len = sizeof(cmdbuf), .buf = (u8 *) &cmdbuf },
		{ .addr = client->addr, .len = len, .buf = databuf,
		  .flags = I2C_M_RD } };
	int ret;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		dev_err(&client->dev, "failed reading register 0x%04x\n", cmd);

	return ret;
}

static int vl6180_read_byte(struct i2c_client *client, u16 cmd)
{
	u8 data;
	int ret;

	ret = vl6180_read(client, cmd, &data, sizeof(data));
	if (ret < 0)
		return ret;

	return data;
}

static int vl6180_read_word(struct i2c_client *client, u16 cmd)
{
	__be16 data;
	int ret;

	ret = vl6180_read(client, cmd, &data, sizeof(data));
	if (ret < 0)
		return ret;

	return be16_to_cpu(data);
}

static int vl6180_write_byte(struct i2c_client *client, u16 cmd, u8 val)
{
	u8 buf[3];
	struct i2c_msg msgs[1] = {
		{ .addr = client->addr, .len = sizeof(buf), .buf = (u8 *) &buf } };
	int ret;

	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xff;
	buf[2] = val;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(&client->dev, "failed writing register 0x%04x\n", cmd);
		return ret;
	}

	return 0;
}

static int vl6180_write_word(struct i2c_client *client, u16 cmd, u16 val)
{
	__be16 buf[2];
	struct i2c_msg msgs[1] = {
		{ .addr = client->addr, .len = sizeof(buf), .buf = (u8 *) &buf } };
	int ret;

	buf[0] = cpu_to_be16(cmd);
	buf[1] = cpu_to_be16(val);

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(&client->dev, "failed writing register 0x%04x\n", cmd);
		return ret;
	}

	return 0;
}

static int vl6180_measure(struct vl6180_data *data, int addr)
{
	struct i2c_client *client = data->client;
	int tries = 20, ret;
	u16 value;

	mutex_lock(&data->lock);
	/* Start single shot measurement */
	ret = vl6180_write_byte(client,
		vl6180_chan_regs_table[addr].start_reg, VL6180_STARTSTOP);
	if (ret < 0)
		goto fail;

	while (tries--) {
		ret = vl6180_read_byte(client, VL6180_INTR_STATUS);
		if (ret < 0)
			goto fail;

		if (ret & vl6180_chan_regs_table[addr].drdy_mask)
			break;
		msleep(20);
	}

	if (tries < 0) {
		ret = -EIO;
		goto fail;
	}

	/* Read result value from appropriate registers */
	ret = vl6180_chan_regs_table[addr].word ?
		vl6180_read_word(client, vl6180_chan_regs_table[addr].value_reg) :
		vl6180_read_byte(client, vl6180_chan_regs_table[addr].value_reg);
	if (ret < 0)
		goto fail;
	value = ret;

	/* Clear the interrupt flag after data read */
	ret = vl6180_write_byte(client, VL6180_INTR_CLEAR,
		VL6180_CLEAR_ERROR | VL6180_CLEAR_ALS | VL6180_CLEAR_RANGE);
	if (ret < 0)
		goto fail;

	ret = value;

fail:
	mutex_unlock(&data->lock);

	return ret;
}

static const struct iio_chan_spec vl6180_channels[] = {
	{
		.type = IIO_LIGHT,
		.address = VL6180_ALS,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_INT_TIME) |
			BIT(IIO_CHAN_INFO_SCALE) |
			BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	}, {
		.type = IIO_DISTANCE,
		.address = VL6180_RANGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_SCALE),
	}, {
		.type = IIO_PROXIMITY,
		.address = VL6180_PROX,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	}
};

/*
 * Columns 3 & 4 represent the same value in decimal and hex notations.
 * Kept in order to avoid the datatype conversion while reading the
 * hardware_gain.
 */
static const int vl6180_als_gain[8][4] = {
	{ 1,	0,	70,	VL6180_ALS_GAIN_1 },
	{ 1,    250000, 69,	VL6180_ALS_GAIN_1_25 },
	{ 1,    670000, 68,	VL6180_ALS_GAIN_1_67 },
	{ 2,    500000, 67,	VL6180_ALS_GAIN_2_5 },
	{ 5,    0,      66,	VL6180_ALS_GAIN_5 },
	{ 10,   0,      65,	VL6180_ALS_GAIN_10 },
	{ 20,   0,      64,	VL6180_ALS_GAIN_20 },
	{ 40,   0,      71,	VL6180_ALS_GAIN_40 }
};

static int vl6180_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct vl6180_data *data = iio_priv(indio_dev);
	int ret, i;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = vl6180_measure(data, chan->address);
		if (ret < 0)
			return ret;
		*val = ret;

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_INT_TIME:
		ret = vl6180_read_word(data->client, VL6180_ALS_IT);
		if (ret < 0)
			return ret;
		*val = 0; /* 1 count = 1ms (0 = 1ms) */
		*val2 = (ret + 1) * 1000; /* convert to seconds */

		return IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_LIGHT:
			*val = 0; /* one ALS count is 0.32 Lux */
			*val2 = 320000;
			break;
		case IIO_DISTANCE:
			*val = 0; /* sensor reports mm, scale to meter */
			*val2 = 1000;
			break;
		default:
			return -EINVAL;
		}

		return IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		ret = vl6180_read_byte(data->client, VL6180_ALS_GAIN);
		if (ret < 0)
			return -EINVAL;
		for (i = 0; i < ARRAY_SIZE(vl6180_als_gain); i++) {
			if (ret == vl6180_als_gain[i][2]) {
				*val = vl6180_als_gain[i][0];
				*val2 = vl6180_als_gain[i][1];
			}
		}

		return IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}
}

static IIO_CONST_ATTR(als_gain_available, "1 1.25 1.67 2.5 5 10 20 40");

static struct attribute *vl6180_attributes[] = {
	&iio_const_attr_als_gain_available.dev_attr.attr,
	NULL
};

static const struct attribute_group vl6180_attribute_group = {
	.attrs = vl6180_attributes,
};

/* HOLD is needed before updating any config registers */
static int vl6180_hold(struct vl6180_data *data, bool hold)
{
	return vl6180_write_byte(data->client, VL6180_HOLD,
		hold ? VL6180_HOLD_ON : 0);
}

static int vl6180_set_als_gain(struct vl6180_data *data, int val, int val2)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(vl6180_als_gain); i++) {
		if (val == vl6180_als_gain[i][0] &&
			val2 == vl6180_als_gain[i][1]) {
			mutex_lock(&data->lock);
			ret = vl6180_hold(data, true);
			if (ret < 0)
				goto fail;
			ret = vl6180_write_byte(data->client, VL6180_ALS_GAIN,
				vl6180_als_gain[i][3]);
fail:
			vl6180_hold(data, false);
			mutex_unlock(&data->lock);
			return ret;
		}
	}

	return -EINVAL;
}

static int vl6180_set_it(struct vl6180_data *data, int val2)
{
	int ret;

	mutex_lock(&data->lock);
	ret = vl6180_hold(data, true);
	if (ret < 0)
		goto fail;
	ret = vl6180_write_word(data->client, VL6180_ALS_IT,
		(val2 - 500) / 1000); /* write value in ms */
fail:
	vl6180_hold(data, false);
	mutex_unlock(&data->lock);

	return ret;
}

static int vl6180_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct vl6180_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_INT_TIME:
		if (val != 0 || val2 < 500 || val2 >= 512500)
			return -EINVAL;

		return vl6180_set_it(data, val2);
	case IIO_CHAN_INFO_HARDWAREGAIN:
		if (chan->type != IIO_LIGHT)
			return -EINVAL;

		return vl6180_set_als_gain(data, val, val2);
	default:
		return -EINVAL;
	}
}

static const struct iio_info vl6180_info = {
	.read_raw = vl6180_read_raw,
	.write_raw = vl6180_write_raw,
	.attrs = &vl6180_attribute_group,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 15, 0)
	.driver_module = THIS_MODULE,
#endif
};

static int vl6180_init(struct vl6180_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

	ret = vl6180_read_byte(client, VL6180_MODEL_ID);
	if (ret < 0)
		return ret;

	if (ret != VL6180_MODEL_ID_VAL) {
		dev_err(&client->dev, "invalid model ID %02x\n", ret);
		return -ENODEV;
	}

	ret = vl6180_hold(data, true);
	if (ret < 0)
		return ret;

	ret = vl6180_read_byte(client, VL6180_OUT_OF_RESET);
	if (ret < 0)
		return ret;

	/*
	 * Detect false reset condition here. This bit is always set when the
	 * system comes out of reset.
	 */
	if (ret != 0x01)
		dev_info(&client->dev, "device is not fresh out of reset\n");

	/* Enable ALS and Range ready interrupts */
	ret = vl6180_write_byte(client, VL6180_INTR_CONFIG,
				VL6180_ALS_READY | VL6180_RANGE_READY);
	if (ret < 0)
		return ret;

	/* ALS integration time: 100ms */
	ret = vl6180_write_word(client, VL6180_ALS_IT, VL6180_ALS_IT_100);
	if (ret < 0)
		return ret;

	/* ALS gain: 1 */
	ret = vl6180_write_byte(client, VL6180_ALS_GAIN, VL6180_ALS_GAIN_1);
	if (ret < 0)
		return ret;

	ret = vl6180_write_byte(client, VL6180_OUT_OF_RESET, 0x00);
	if (ret < 0)
		return ret;

	return vl6180_hold(data, false);
}

static int vl6180_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct vl6180_data *data;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;
	mutex_init(&data->lock);

	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &vl6180_info;
	indio_dev->channels = vl6180_channels;
	indio_dev->num_channels = ARRAY_SIZE(vl6180_channels);
	indio_dev->name = VL6180_DRV_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = vl6180_init(data);
	if (ret < 0)
		return ret;

	return devm_iio_device_register(&client->dev, indio_dev);
}

static const struct of_device_id vl6180_of_match[] = {
	{ .compatible = "st,vl6180", },
	{ },
};
MODULE_DEVICE_TABLE(of, vl6180_of_match);

static const struct i2c_device_id vl6180_id[] = {
	{ "vl6180", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, vl6180_id);

static struct i2c_driver vl6180_driver = {
	.driver = {
		.name   = VL6180_DRV_NAME,
		.of_match_table = of_match_ptr(vl6180_of_match),
	},
	.probe  = vl6180_probe,
	.id_table = vl6180_id,
};

module_i2c_driver(vl6180_driver);

MODULE_AUTHOR("Peter Meerwald-Stadler <pmeerw@pmeerw.net>");
MODULE_AUTHOR("Manivannan Sadhasivam <manivannanece23@gmail.com>");
MODULE_DESCRIPTION("STMicro VL6180 ALS, range and proximity sensor driver");
MODULE_LICENSE("GPL");
