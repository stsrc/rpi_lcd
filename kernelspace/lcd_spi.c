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

#define DRIVER_NAME "lcd_spi"

//#ifdef DEBUG
#define debug_message() printk(KERN_EMERG "DEBUG %s %d\n", __FUNCTION__, \
			       __LINE__)
//#else
//#define debug_message()
//#endif

struct lcdd {
	struct cdev *cdev;
	dev_t devt;
	struct file_operations fops;
	struct class *class;
	struct device *device;
	struct spi_device *spi_device;
};

static struct lcdd lcdd;

int lcdd_open(struct inode *inode, struct file *file)
{
	spi_dev_put(lcdd.spi_device);
	return 0;
}

int lcdd_release(struct inode *inode, struct file *file)
{
	spi_dev_get(lcdd.spi_device);
	return 0;
}	

struct lcdd_transfer {
	uint32_t byte_cnt;
	uint8_t data_cmd;
	const uint8_t __user *bufer;
};


static struct gpio lcd_gpio[] = {
	{ 24, GPIOF_OUT_INIT_LOW, "DC" },
	{ 25, GPIOF_OUT_INIT_HIGH, "RESET" },
	{ 18, GPIOF_OUT_INIT_HIGH, "LED" }
};


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
	gpio_set_value(lcd_gpio[1].gpio, 0);
	udelay(25);
	gpio_set_value(lcd_gpio[1].gpio, 1);
	mdelay(10);
}

static inline void lcdd_set_data_cmd_pin(uint8_t data_cmd)
{
	if (data_cmd)
		gpio_set_value(lcd_gpio[0].gpio, 1);
	else
		gpio_set_value(lcd_gpio[0].gpio, 0);
}

/*
 * Remember to call kfree when spi_transfer will not be used anymore - function 
 * dynamically alocates memory!
 */


static int lcdd_init_spi_transfer(const char __user *message, struct spi_transfer 
				  *spi_transfer)
{ 
	int ret;
	struct lcdd_transfer transfer;
	void *temp;
	memset(spi_transfer, 0, sizeof(struct spi_transfer));
	ret = copy_from_user((char *)&transfer, message, 
			     sizeof(struct lcdd_transfer));
	if (ret)
		return -EAGAIN;
	if (!transfer.byte_cnt) {
		debug_message();
		return -EINVAL;
	}
	else if (transfer.bufer == NULL) {
		debug_message();
		return -EINVAL;
	}
	temp = kmalloc(transfer.byte_cnt, GFP_KERNEL);
	if (temp == NULL)
		return -ENOMEM;
	spi_transfer->rx_buf = kmalloc(transfer.byte_cnt, GFP_KERNEL);
	if (spi_transfer->rx_buf == NULL) {
		kfree(temp);
		spi_transfer->tx_buf = NULL;
		return -ENOMEM;
	}
	//MOVE IT SOMEWHERE ELSE
	lcdd_set_data_cmd_pin(transfer.data_cmd);
	ret = copy_from_user(temp, transfer.bufer, transfer.byte_cnt);
	if (ret) {
		kfree(temp);
		kfree(spi_transfer->rx_buf);
		spi_transfer->tx_buf = NULL;
		spi_transfer->rx_buf = NULL;
		return -EAGAIN;
	}
	spi_transfer->tx_buf = temp;
	spi_transfer->len = transfer.byte_cnt;
	spi_transfer->speed_hz = 9600;
	return 0;
}


static void lcdd_complete_transfer(void *context)
{
	complete(context);
}

static inline void lcdd_deinit_spi_transfer(struct spi_transfer *spi_transfer)
{
	kfree(spi_transfer->tx_buf);
	kfree(spi_transfer->rx_buf);
}

void lcdd_spi_device(struct spi_device *spi)
{
	printk("struct device dev = %lu", (unsigned long)&spi->dev);
	printk("struct spi_master = %lu", (unsigned long)spi->master);
	printk("struct max_speed_hz = %u", spi->max_speed_hz);
	printk("chip select = %u", spi->chip_select);
	printk("bits per word = %u", spi->bits_per_word);
	printk("mode = %u", spi->mode);
	printk("irq = %d", spi->irq);
	printk("controller_state = %lu", (unsigned long)spi->controller_state);
	printk("controller_data = %lu", (unsigned long)spi->controller_data);
	printk("modalias = %s", spi->modalias);
	printk("cs gpio = %d", spi->cs_gpio);
	printk("stats = %lu", (unsigned long)spi->controller_state);
}

static long lcdd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct spi_message spi_message;
	struct spi_transfer spi_transfer;
	DECLARE_COMPLETION_ONSTACK(done);
	spi_message_init(&spi_message);
	ret = lcdd_init_spi_transfer((const char __user *)arg, &spi_transfer);
	if (ret) {
		debug_message();
		return ret;
	}
	spi_message_add_tail(&spi_transfer, &spi_message);
	spi_message.complete = lcdd_complete_transfer;
	spi_message.context = &done;
	debug_message();	
	ret = spi_async(lcdd.spi_device, &spi_message);
	if (ret) {
		//ok?
		debug_message();
		lcdd_deinit_spi_transfer(&spi_transfer);
		return ret;
	}
	debug_message();
	wait_for_completion(&done);
	lcdd_deinit_spi_transfer(&spi_transfer);
	debug_message();
	return 1;
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
	spi->max_speed_hz = 50000;
	spi->bits_per_word = 8;
	spi->mode = 0;
	return spi_setup(spi);	
}

static int spidev_probe(struct spi_device *spi)
{
	int ret;
	/*TODO: WHAT DOES IT DO?*/
	if (spi->dev.of_node && !of_match_device(lcd_spi_dt_ids, &spi->dev)) {
		debug_message();
		dev_err(&spi->dev, "buggy DT: lcd_spi listed directly in DT\n");
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
	spi_set_drvdata(spi, &lcdd);
	return 0;	
}

static int spidev_remove(struct spi_device *spi)
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
	.probe = spidev_probe,
	.remove = spidev_remove,
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
MODULE_ALIAS("spi:lcd_spi");
