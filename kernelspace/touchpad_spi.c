#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DRIVER_NAME "touchpad_spi"
#define HEIGHT 320
#define LENGTH 240
#define BY_PER_PIX 2
#define TOT_MEM_SIZE BY_PER_PIX*HEIGHT*LENGTH
#define SPI_SPEED 6600000

#ifdef DEBUG
#define debug_message() printk(KERN_EMERG "DEBUG %s %d\n", __FUNCTION__, \
			       __LINE__)
#else
#define debug_message()
#endif

#define SPI_IOC_MAGIC 'k'
#define SPI_IO_WR_DATA		_IOW(SPI_IOC_MAGIC, 6, struct lcdd_transfer)
#define SPI_IO_WR_CMD		_IOW(SPI_IOC_MAGIC, 7, struct lcdd_transfer)
#define SPI_IO_RD_CMD		_IOR(SPI_IOC_MAGIC, 7, struct lcdd_transfer)

struct lcdd {
	struct cdev *cdev;
	dev_t devt;
	struct file_operations fops;
	struct class *class;
	struct device *device;
	struct spi_device *spi_device;
	uint8_t *tx;
	uint8_t *rx;
};

static struct lcdd lcdd;

struct lcdd_transfer {
	uint32_t byte_cnt;
	const uint8_t __user *tx;
	uint8_t __user *rx;
};


static struct gpio lcd_gpio[] = {
	{ 23, GPIOF_IN, "IRQ" }
};

int lcdd_open(struct inode *inode, struct file *file)
{
	lcdd.tx = kmalloc(TOT_MEM_SIZE + 2, GFP_KERNEL);
	if (!lcdd.tx)
		return -ENOMEM;
	lcdd.rx = kmalloc(TOT_MEM_SIZE + 2, GFP_KERNEL);
	if (!lcdd.rx) {
		kfree(lcdd.tx);
		return -ENOMEM;
	}
	file->private_data = &lcdd;
	return 0;
}

int lcdd_release(struct inode *inode, struct file *file)
{
	kfree(lcdd.rx);
	lcdd.rx = NULL;
	kfree(lcdd.tx);
	lcdd.tx = NULL;
	return 0;
}	

static inline int lcdd_set_gpio(void)
{
	int ret;
	ret = gpio_request_array(lcd_gpio, ARRAY_SIZE(lcd_gpio));
	return ret;	
}

static inline void lcdd_unset_gpio(void)
{
	gpio_free_array(lcd_gpio, ARRAY_SIZE(lcd_gpio));
}

static inline void lcdd_reset(void) 
{
}

static inline void lcdd_set_data_cmd_pin(uint8_t data_cmd)
{
}

static int lcdd_parse_user_data(const char __user *message, 
			        struct lcdd_transfer *transfer)
{
	int ret = copy_from_user(transfer, message, 
			     sizeof(struct lcdd_transfer));
	if (ret)
		return -EAGAIN;
	if (!transfer->byte_cnt) {
		debug_message();
		return -EINVAL;
	}
	else if (transfer->tx == NULL) {
		debug_message();
		return -EINVAL;
	}
	return 0;
}

static int lcdd_init_spi_transfer(struct lcdd_transfer *transfer, 
				  struct spi_transfer *spi_transfer, int op)
{
	int ret;
	memset(spi_transfer, 0, sizeof(struct spi_transfer));
	switch (op) {
	case SPI_IO_WR_DATA:
	case SPI_IO_WR_CMD:
		spi_transfer->tx_buf = lcdd.tx;
		spi_transfer->len = transfer->byte_cnt;
		memset(lcdd.tx, 0, TOT_MEM_SIZE);
		ret = copy_from_user(lcdd.tx, transfer->tx, transfer->byte_cnt);
		if (ret) {
			spi_transfer->tx_buf = NULL;
			return -EAGAIN;
		}
		return 0;
	case SPI_IO_RD_CMD:
		spi_transfer->tx_buf = lcdd.tx;
		spi_transfer->rx_buf = lcdd.rx;
		spi_transfer->len = transfer->byte_cnt;
		memset(lcdd.tx, 0, TOT_MEM_SIZE);
		ret = copy_from_user(lcdd.tx, transfer->tx, 1);
		if (ret) {
			spi_transfer->tx_buf = NULL;
			spi_transfer->rx_buf = NULL;
		}
		return 0;
	}
	return -EINVAL;
}


static void lcdd_complete_transfer(void *context)
{
	complete(context);
}

static int lcdd_message_send(struct spi_transfer *spi_transfer, uint8_t data_cmd)
{
	int ret;
	struct spi_message spi_message;
	DECLARE_COMPLETION_ONSTACK(done);
	spi_message_init(&spi_message);
	spi_message_add_tail(spi_transfer, &spi_message);
	spi_message.complete = lcdd_complete_transfer;
	spi_message.context = &done;
	lcdd_set_data_cmd_pin(data_cmd);
	udelay(1);
	ret = spi_async(lcdd.spi_device, &spi_message);
	if (ret) {
		debug_message();
		return ret;
	}
	wait_for_completion(&done);
	return 0;
}

static int lcdd_write(struct file *file, unsigned long arg, int op) 
{
	int ret;
	struct lcdd_transfer lcdd_transfer;
	struct spi_transfer spi_transfer;
	int data_cmd;

	if (op == SPI_IO_WR_DATA)
		data_cmd = 1;
	else
		data_cmd = 0;

	ret = lcdd_parse_user_data((const char __user *)arg, &lcdd_transfer);
	if (ret)
		goto err;
	ret = lcdd_init_spi_transfer(&lcdd_transfer, &spi_transfer, op);
	if (ret) 
		goto err;
	ret = lcdd_message_send(&spi_transfer, data_cmd);
	if (ret)
		goto err;
	if (op == SPI_IO_RD_CMD) {
		/*
		 * addition and subtraction to remove not important data
		 * (answer for command write and dummy byte removed)
		 */
		ret = copy_to_user(lcdd_transfer.rx, (void *)(spi_transfer.rx_buf + 2),
			           spi_transfer.len - 2);
		if (ret) {
			goto err;
		}	
	}
	return 1;
err:
	debug_message();
	return ret;
}
	
static long lcdd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{	
	return lcdd_write(file, arg, cmd);
}

static struct lcdd lcdd = {
	.cdev = NULL,
	.devt = 0,
	.class = NULL,
	.fops = {.owner = THIS_MODULE,
		.open = lcdd_open,
		.release = lcdd_release,
		.unlocked_ioctl = lcdd_ioctl,
		.compat_ioctl = lcdd_ioctl
	},
	.device = NULL };

static const struct of_device_id lcd_spi_dt_ids[] = {
	{.compatible = DRIVER_NAME },
	{},
};

MODULE_DEVICE_TABLE(of, lcd_spi_dt_ids);

static int lcdd_spi_device_set(struct spi_device *spi)
{
	spi->max_speed_hz = SPI_SPEED;
	spi->bits_per_word = 8;
	spi->mode = 0;
	return spi_setup(spi);	
}

static int lcdd_probe(struct spi_device *spi)
{
	int ret;
	if (spi->dev.of_node && !of_match_device(lcd_spi_dt_ids, &spi->dev)) {
		debug_message();
		dev_err(&spi->dev, "buggy DT: touchpad_spi listed directly in DT\n");
		WARN_ON(spi->dev.of_node &&
			!of_match_device(lcd_spi_dt_ids, &spi->dev));
	}
	lcdd.device = device_create(lcdd.class, &spi->dev, lcdd.devt, &lcdd, 
				    DRIVER_NAME);

	if (IS_ERR(lcdd.device)) {
		debug_message();
		dev_err(&spi->dev, "buggy DT: device already exists in system\n");
		return PTR_ERR(lcdd.device);		
	}
	lcdd.spi_device = spi;
	ret = lcdd_spi_device_set(spi);
	if (ret)
		debug_message();
	return 0;	
}

static int lcdd_remove(struct spi_device *spi)
{
	device_destroy(lcdd.class, lcdd.devt);
	return 0;
}

static struct spi_driver lcd_spi_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lcd_spi_dt_ids),
	},
	.probe = lcdd_probe,
	.remove = lcdd_remove,
};

static int __init lcdd_init(void)
{
	int rt;
	rt = alloc_chrdev_region(&lcdd.devt, 0, 1, DRIVER_NAME);
	if (rt)
		return rt;
	lcdd.class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(lcdd.class)) {
		rt = PTR_ERR(lcdd.class);
		goto err;
	}
	lcdd.cdev = cdev_alloc();
	if (!lcdd.cdev) {
		rt = -ENOMEM;
		goto err;
	}
	cdev_init(lcdd.cdev, &lcdd.fops);
	rt = cdev_add(lcdd.cdev, lcdd.devt, 1);
	if (rt) {
		cdev_del(lcdd.cdev);
		goto err;
	}
	rt = spi_register_driver(&lcd_spi_driver);
	if (rt < 0) {
		cdev_del(lcdd.cdev);
		goto err;
	}
	lcdd_set_gpio();
	lcdd_reset();
	return 0;
err:
	if (lcdd.class)
		class_destroy(lcdd.class);
	unregister_chrdev_region(lcdd.devt, 1);
	return rt;
}

static void __exit lcdd_exit(void)
{
	lcdd_unset_gpio();
	spi_unregister_driver(&lcd_spi_driver);
	device_destroy(lcdd.class, lcdd.devt);
	cdev_del(lcdd.cdev);
	class_destroy(lcdd.class);
	unregister_chrdev_region(lcdd.devt, 1);
}

module_init(lcdd_init);
module_exit(lcdd_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:touchpad_spi");
