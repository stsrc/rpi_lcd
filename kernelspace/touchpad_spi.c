/*
 * based a little on spidev.c
 *	Copyright (C) 2006 SWAPP
 *		Andrea Paterniani <a.paterniani@swapp-eng.it>
 *	Copyright (C) 2007 David Brownell (simplification, cleanup)
 *	
 *	This program is free software; you can redistribute it and/or modify
 *	it under the temrs of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 */

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
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/export.h>
#include <linux/timer.h>
#include <linux/spinlock.h>

#include "touchpad_notifier.h"

#define DRIVER_NAME "touchpad_spi"
#define SPI_SPEED 100000

#ifdef DEBUG
#define debug_message() printk(KERN_EMERG "DEBUG %s %d\n", __FUNCTION__, \
			       __LINE__)
#else
#define debug_message()
#endif

#define SPI_IOC_MAGIC 'k'
#define SPI_IO_RD_CMD		_IOR(SPI_IOC_MAGIC, 7, struct touch_transfer)

#define TIMER_DELAY  1 * HZ

struct touch {
	struct cdev *cdev;
	dev_t devt;
	struct file_operations fops;
	struct class *class;
	struct device *device;
	struct spi_device *spi_device;
	uint8_t flag; 
	uint8_t *tx;
	uint8_t *rx;
	uint32_t irq;
	spinlock_t lock;
};

static struct touch touch;

struct touch_transfer {
	uint32_t byte_cnt;
	const uint8_t __user *tx;
	uint8_t __user *rx;
};


static struct gpio touch_gpio[] = {
	{ 23, GPIOF_IN, "IRQ" }
};

static ATOMIC_NOTIFIER_HEAD(touchpad_notifier_list);

int register_touchpad_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&touchpad_notifier_list, nb);
}

EXPORT_SYMBOL_GPL(register_touchpad_notifier);

int unregister_touchpad_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&touchpad_notifier_list, nb);
}

EXPORT_SYMBOL_GPL(unregister_touchpad_notifier);

/*
 * Interrupt handling routines:
 *	touchpad_turn_on_interrupt
 *	touchpad_interrupt
 */

void touchpad_turn_on_interrupt(unsigned long arg)
{
	unsigned long flags;
	spin_lock_irqsave(&touch.lock, flags);
	touch.flag = 1;
	spin_unlock_irqrestore(&touch.lock, flags);
}

static struct timer_list touchpad_timer;

static irq_handler_t touchpad_interrupt(unsigned int irq, void *dev_id,
					struct pt_regs *regs)
{
	int rt;
	unsigned long flags;
	spin_lock_irqsave(&touch.lock, flags);
	if (touch.flag) {
		rt = atomic_notifier_call_chain(&touchpad_notifier_list, 0, NULL); 
		rt = mod_timer(&touchpad_timer, jiffies + TIMER_DELAY);
		touch.flag = 0;
	}
	spin_unlock_irqrestore(&touch.lock, flags);
	return (irq_handler_t)IRQ_HANDLED;
}

int touch_open(struct inode *inode, struct file *file)
{
	/*device can be opened only once at a moment*/
	if  (atomic_read(&file->f_count) != 1) 
		return -EINVAL;
	touch.tx = kzalloc(3, GFP_KERNEL);
	if (!touch.tx)
		return -ENOMEM;
	touch.rx = kzalloc(3, GFP_KERNEL);
	if (!touch.rx) {
		kfree(touch.tx);
		return -ENOMEM;
	}
	file->private_data = &touch;
	return 0;
}

int touch_release(struct inode *inode, struct file *file)
{
	kfree(touch.rx);
	touch.rx = NULL;
	kfree(touch.tx);
	touch.tx = NULL;
	return 0;
}	

static inline int touch_set_gpio(void)
{
	int ret;
	ret = gpio_request_array(touch_gpio, ARRAY_SIZE(touch_gpio));
	touch.irq = gpio_to_irq(touch_gpio[0].gpio);
	return ret;	
}

static inline void touch_unset_gpio(void)
{
	gpio_free_array(touch_gpio, ARRAY_SIZE(touch_gpio));
}

static int touch_parse_user_data(const char __user *message, 
			        struct touch_transfer *transfer)
{
	int ret = copy_from_user(transfer, message, 
			     sizeof(struct touch_transfer));
	if (ret)
		return -EAGAIN;
	else if (transfer->tx == NULL) {
		debug_message();
		return -EINVAL;
	}
	return 0;
}

static int touch_init_spi_transfer(struct touch_transfer *transfer, 
				  struct spi_transfer *spi_transfer, int op)
{
	int ret;
	memset(spi_transfer, 0, sizeof(struct spi_transfer));
	switch (op) {
	case SPI_IO_RD_CMD:
		spi_transfer->tx_buf = touch.tx;
		spi_transfer->rx_buf = touch.rx;
		spi_transfer->len = 3;
		ret = copy_from_user(touch.tx, transfer->tx, 1);
		if (ret) {
			spi_transfer->tx_buf = NULL;
			spi_transfer->rx_buf = NULL;
		}
		return 0;
	}
	return -EINVAL;
}


static void touch_complete_transfer(void *context)
{
	complete(context);
}

static int touch_message_send(struct spi_transfer *spi_transfer, uint8_t data_cmd)
{
	int ret;
	struct spi_message spi_message;
	DECLARE_COMPLETION_ONSTACK(done);
	spi_message_init(&spi_message);
	spi_message_add_tail(spi_transfer, &spi_message);
	spi_message.complete = touch_complete_transfer;
	spi_message.context = &done;
	/*
	 * disabling irqline because in time of measure there are false irqs
	 */
	disable_irq_nosync(touch.irq);
	ret = spi_async(touch.spi_device, &spi_message);
	if (ret) {
		debug_message();
		return ret;
	}
	wait_for_completion(&done);
	enable_irq(touch.irq);
	return 0;
}

static int touch_write(struct file *file, unsigned long arg, int op) 
{
	int ret;
	struct touch_transfer touch_transfer;
	struct spi_transfer spi_transfer;
	int data_cmd;
	data_cmd = 0;
	ret = touch_parse_user_data((const char __user *)arg, &touch_transfer);
	if (ret) {
		debug_message();
		goto err;
	}
	ret = touch_init_spi_transfer(&touch_transfer, &spi_transfer, op);
	if (ret) {
		debug_message();
		goto err;
	}
	ret = touch_message_send(&spi_transfer, data_cmd);
	if (ret) {
		debug_message();
		goto err;
	}
	ret = copy_to_user(touch_transfer.rx, (void *)(spi_transfer.rx_buf),
		           spi_transfer.len);
	if (ret) {
		debug_message();
		goto err;	
	}
	return 1;
err:
	debug_message();
	return ret;
}
	
static long touch_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{	
	return touch_write(file, arg, cmd);
}

static struct touch touch = {
	.cdev = NULL,
	.devt = 0,
	.class = NULL,
	.fops = {.owner = THIS_MODULE,
		.open = touch_open,
		.release = touch_release,
		.unlocked_ioctl = touch_ioctl,
		.compat_ioctl = touch_ioctl
	},
	.device = NULL };

static const struct of_device_id touch_spi_dt_ids[] = {
	{.compatible = DRIVER_NAME },
	{},
};

MODULE_DEVICE_TABLE(of, touch_spi_dt_ids);

static int touch_spi_device_set(struct spi_device *spi)
{
	spi->max_speed_hz = SPI_SPEED;
	spi->bits_per_word = 8;
	spi->mode = 0;
	return spi_setup(spi);	
}

static int touch_probe(struct spi_device *spi)
{
	int ret;
	if (spi->dev.of_node && !of_match_device(touch_spi_dt_ids, &spi->dev)) {
		debug_message();
		dev_err(&spi->dev, "buggy DT: touchpad_spi listed directly in DT\n");
		WARN_ON(spi->dev.of_node &&
			!of_match_device(touch_spi_dt_ids, &spi->dev));
	}
	touch.device = device_create(touch.class, &spi->dev, touch.devt, &touch, 
				    DRIVER_NAME);

	if (IS_ERR(touch.device)) {
		debug_message();
		dev_err(&spi->dev, "buggy DT: device already exists in system\n");
		return PTR_ERR(touch.device);		
	}
	touch.spi_device = spi;
	ret = touch_spi_device_set(spi);
	if (ret)
		debug_message();
	return 0;	
}

static int touch_remove(struct spi_device *spi)
{
	device_destroy(touch.class, touch.devt);
	return 0;
}

static struct spi_driver touch_spi_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(touch_spi_dt_ids),
	},
	.probe = touch_probe,
	.remove = touch_remove,
};

static int __init touch_init(void)
{
	int rt;
	rt = alloc_chrdev_region(&touch.devt, 0, 1, DRIVER_NAME);
	if (rt)
		return rt;
	touch.class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(touch.class)) {
		rt = PTR_ERR(touch.class);
		goto err;
	}
	touch.cdev = cdev_alloc();
	if (!touch.cdev) {
		rt = -ENOMEM;
		goto err;
	}
	cdev_init(touch.cdev, &touch.fops);
	rt = cdev_add(touch.cdev, touch.devt, 1);
	if (rt) {
		cdev_del(touch.cdev);
		goto err;
	}
	rt = spi_register_driver(&touch_spi_driver);
	if (rt < 0) {
		cdev_del(touch.cdev);
		goto err;
	}
	spin_lock_init(&touch.lock);
	touch_set_gpio();
	rt = request_irq(touch.irq, (irq_handler_t)touchpad_interrupt, 
			 IRQF_TRIGGER_FALLING, "touchpad_interrupt",
			 NULL);
	if (rt) {
		cdev_del(touch.cdev);
		touch_unset_gpio();
		goto err;
	}
	setup_timer(&touchpad_timer, touchpad_turn_on_interrupt, 0);
	rt = mod_timer(&touchpad_timer, jiffies + TIMER_DELAY);
	if (rt) {
		debug_message();
		del_timer(&touchpad_timer);
		cdev_del(touch.cdev);
		touch_unset_gpio();
		free_irq(touch.irq, NULL);
		goto err;
	}
	return 0;
err:
	if (touch.class)
		class_destroy(touch.class);
	unregister_chrdev_region(touch.devt, 1);
	return rt;
}

static void __exit touch_exit(void)
{
	del_timer(&touchpad_timer);
	free_irq(touch.irq, NULL);
	touch_unset_gpio();
	spi_unregister_driver(&touch_spi_driver);
	device_destroy(touch.class, touch.devt);
	cdev_del(touch.cdev);
	class_destroy(touch.class);
	unregister_chrdev_region(touch.devt, 1);
}

module_init(touch_init);
module_exit(touch_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:touchpad_spi");
