// SPDX-License-Identifier: GPL-2.0-only
/*
 * Analog Devices ad5592r_s driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_S_REG_ADC_SEQ      0x2
#define  AD5592R_S_MASK_REP	   BIT(9)
#define AD5592R_S_REG_ADC_CFG 	   0x4
#define AD5592R_S_REG_SW_RST	   0xF
#define  AD5592R_S_MASK_RST 	   0x5AC

#define AD5592R_S_REG_READBACK     0x7
#define  AD5592R_S_RDB_EN_MASK     BIT(6)
#define  AD5592R_S_RDB_REG_MASK    GENMASK(5,2)

#define AD5592R_S_MASK_REF_CFG	   0xB
#define  AD5592R_S_MASK_REF_EN	   BIT(9)
   
#define AD5592R_S_ADDR_MASK        GENMASK(14,11)
#define AD5592R_S_VAL_MASK         GENMASK(10,0)
#define AD5592R_S_ADC_CHAN(x)	   BIT(x)
#define AD5592R_S_ADC_ADDR_MASK	   GENMASK(14,12)
#define AD5592R_S_ADC_VAL_MASK	   GENMASK(11,0)
#define AD5592R_S_ADC_DEF_PIN_MASK GENMASK(5,0)

struct ad5592r_s_state {
	struct spi_device *spi;
	bool en;
	
};

static int ad5592r_s_spi_nop(struct ad5592r_s_state *st, u16 *val)
{
	u16 tx = 0;
        struct spi_transfer xfer = {
                .tx_buf = &tx,
                .rx_buf = val,
                .len = 2,
        };
        return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_spi_read_ctl(struct ad5592r_s_state *st, u8 reg, u16 *val)
{
        u16 tx = 0;
        u16 msg = 0;
        u16 rx = 0;
        int ret;
        struct spi_transfer xfer = {
		.tx_buf = &tx,
		.len = 2,
        };

        msg |= FIELD_PREP(AD5592R_S_ADDR_MASK, AD5592R_S_REG_READBACK);
        msg |= AD5592R_S_RDB_EN_MASK;
        msg |= FIELD_PREP(AD5592R_S_RDB_REG_MASK ,reg);
        put_unaligned_be16(msg, &tx);

        ret = spi_sync_transfer(st->spi, &xfer, 1);
        if(ret){
                dev_err(&st->spi->dev, "Failed at SPI WR transfer");
                return ret;
        }
        ret = ad5592r_s_spi_nop(st, &rx);
        if(ret){
                dev_err(&st->spi->dev, "Failed at SPI NOP transfer");
                return ret;
        }
        *val = get_unaligned_be16(&rx);
        return 0;
}

static int ad5592r_s_spi_write(struct ad5592r_s_state *st, u16 reg, u16 val)
{
        u16 msg = 0;
        u16 tx = 0;

        struct spi_transfer xfer  = {
                .tx_buf = &tx,
                .len = 2,
        };

        msg |= FIELD_PREP(AD5592R_S_ADDR_MASK, reg);
        msg |= FIELD_PREP(AD5592R_S_VAL_MASK , val);

        dev_info(&st->spi->dev, "msg at write = 0%x", msg);
        put_unaligned_be16(msg, &tx);
        dev_info(&st->spi->dev, "tx at write = 0%x", tx);

        return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_s_read_adc(struct ad5592r_s_state *st, int chan, int *val)
{
	u16 tx = 0;
        u16 msg = 0;
        u16 rx = 0;
	u16 tmp;
	u16 addr;
        int ret;
        struct spi_transfer xfer = {
		.tx_buf = &tx,
		.len = 2,
        };

        msg |= FIELD_PREP(AD5592R_S_ADDR_MASK, AD5592R_S_REG_ADC_SEQ);
	msg |= AD5592R_S_ADC_CHAN(chan);

        put_unaligned_be16(msg, &tx);
        ret = spi_sync_transfer(st->spi, &xfer, 1);
        if(ret){
                dev_err(&st->spi->dev, "Failed at SPI WR transfer");
                return ret;
        }

        ret = ad5592r_s_spi_nop(st, NULL);
        if(ret){
                dev_err(&st->spi->dev, "Failed at SPI NOP transfer");
                return ret;
        }

	ret = ad5592r_s_spi_nop(st, &rx);
        if(ret){
                dev_err(&st->spi->dev, "Failed at SPI NOP transfer");
                return ret;
        }
        tmp = get_unaligned_be16(&rx);

	addr = tmp;
	addr &= AD5592R_S_ADC_ADDR_MASK;
	addr = (addr >> 12);
	//dev_info(&st->spi->dev, "req chan = 0x%x, recv chan = 0x%x", chan, addr);

	if (addr != chan)
	{
		dev_err(&st->spi->dev, "Requested channel does not match read channel");
		return -EINVAL;
	}

	tmp &= AD5592R_S_ADC_VAL_MASK;
	*val = tmp;

        return 0;
}

int ad5592r_s_read_raw(struct iio_dev *indio_dev,
		       struct iio_chan_spec const *chan,
		       int *val,
		       int *val2,
		       long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ad5592r_s_read_adc(st, chan->channel, val);
		return IIO_VAL_INT;	
	case IIO_CHAN_INFO_ENABLE:
		st->en = val;
		return IIO_VAL_INT;
	default:
		return -EINVAL;	
	}	
}

static int ad5592r_s_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan,
			       int val,
			       int val2,
			       long mask)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		st->en = val;
		return 0;
	default:
		return -EINVAL;	
	}
}

// static int ad5592r_s__preenable(struct iio_dev *indio_dev)
// {
// 	// struct ad5592r_s_state *st = iio_priv(indio_dev);
// 	// u16 tmp = 0;
// 	// int ret;

// 	// tmp |= *indio_dev->active_scan_mask;
// 	// tmp |= AD5592R_S_MASK_REP;
	
// 	// ret = ad5592r_s_spi_write(st, AD5592R_S_REG_ADC_SEQ, tmp);

// 	// if (ret)
// 	// {
// 	// 	dev_err(&st->spi->dev, "Failed to set up ADC Seq.");
// 	// 	return ret;
// 	// }

// 	// return ad5592r_s_spi_nop(st, NULL);
// }

irqreturn_t ad5592r_s_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int bit = 0;
	int i = 0;
	int val;
	//u16 tmp;
	u16 buff[6];
	int ret;

	for_each_set_bit(bit, indio_dev->active_scan_mask, indio_dev->num_channels)
	{
		ret = ad5592r_s_read_adc(st, bit, &val);
		if (ret)
		{
			dev_err(&st->spi->dev, "Buffer read error @ ch: %d", bit);
			return ret;
		}

		//tmp = get_unaligned_be16(&val);
		buff[i++] = (u16)val;
	}

	iio_push_to_buffers(indio_dev, buff);
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

// static int ad5592r_s_predisable(struct iio_dev *indio_dev)
// {
// 	// struct ad5592r_s_state *st = iio_priv(indio_dev);
	
// 	// return ad5592r_s_spi_write(st, AD5592R_S_REG_ADC_SEQ, 0x0);
// }

static int ad5592r_s_reg_acces(struct iio_dev *indio_dev,
                               unsigned reg, unsigned writeval,
                               unsigned *readval)
{
        struct ad5592r_s_state *st = iio_priv(indio_dev);
        if(readval)
        {
                return ad5592r_s_spi_read_ctl(st, reg, (u16 *)readval);
        }
        return ad5592r_s_spi_write(st, reg, writeval);
}

static const struct iio_info ad5592r_s_info = {
	.read_raw  = &ad5592r_s_read_raw,
	.write_raw = &ad5592r_s_write_raw,
	.debugfs_reg_access = &ad5592r_s_reg_acces,
};

static const struct  iio_buffer_setup_ops ad5592r_s_setup_ops = {
	// .preenable  = &ad5592r_s__preenable,
	// .predisable = &ad5592r_s_predisable,
};

static const struct iio_chan_spec ad5592r_s_channel[] = {

	{
		.type    = IIO_VOLTAGE,
		.channel = 0,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),
		.scan_index = 0,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type    = IIO_VOLTAGE,
		.channel = 1,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = 1,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type    = IIO_VOLTAGE,
		.channel = 2,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = 2,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type    = IIO_VOLTAGE,
		.channel = 3,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = 3,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type    = IIO_VOLTAGE,
		.channel = 4,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = 4,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type    = IIO_VOLTAGE,
		.channel = 5,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = 5,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
};


static int ad5592r_s_int(struct iio_dev *indio_dev)
{
	struct ad5592r_s_state *st = iio_priv(indio_dev);
	int ret;

	ret = ad5592r_s_spi_write(st, AD5592R_S_REG_SW_RST, AD5592R_S_MASK_RST);
	usleep_range(250, 300);
	
	if (ret) {
		return ret;
	}

	ret = ad5592r_s_spi_write(st, AD5592R_S_REG_ADC_CFG,
				  AD5592R_S_ADC_DEF_PIN_MASK);
	
	if (ret) {
		return ret;
	}

	ret = ad5592r_s_spi_write(st, AD5592R_S_MASK_REF_CFG,
				  AD5592R_S_MASK_REF_EN);
	
	if (ret) {
		return ret;
	}

	return 0;
}

static int ad5592r_s_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592r_s_state *st;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev); 
	st->spi = spi;
	st->en = 0;
	
	indio_dev->name = "ad5592r_s";
	indio_dev->info = &ad5592r_s_info;
	indio_dev->channels = ad5592r_s_channel;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_s_channel);
	
	
        ret = ad5592r_s_int(indio_dev);
        if(ret)
	{
		dev_err(&spi->dev, "Failed at initialization");
                return ret;
	}

	ret = devm_iio_triggered_buffer_setup(&spi->dev, indio_dev, NULL, 
					      &ad5592r_s_trigger_handler, 
					      &ad5592r_s_setup_ops);

	if (ret)
		return ret;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_s_driver = {
	.driver = {
		.name = "ad5592r_s",
	},	
	.probe = ad5592r_s_probe,
};

module_spi_driver(ad5592r_s_driver);

MODULE_AUTHOR("Aron Kis <kis.aron@analog.com>");
MODULE_DESCRIPTION("Analog Devices AD5592R ADC driver");
MODULE_LICENSE("GPL v2");
