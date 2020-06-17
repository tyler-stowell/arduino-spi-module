#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "spimod.h"

#define DEV_NAME "Arduino Mini Spi Dev"

MODULE_LICENSE("Dual BSD/GPL");

struct arduino_dev *arduino_spi = NULL;
static dev_t arduino_spi_num;
static struct cdev *arduino_spi_cdev;
static struct class *arduino_spi_class;
static struct device *arduino_spi_dev;

static int arduino_spi_open(struct inode *inode, struct file *filp){
	struct arduino_dev *dev = arduino_spi;

	spin_lock_irq(&dev->spinlock);

	if(dev->opened){
		printk(KERN_INFO "Cannot open device file twice");
		spin_unlock_irq(&dev->spinlock);
		return -EIO;
	}

	dev->opened = true;
	filp->private_data = dev;

	spin_unlock_irq(&dev->spinlock);

	return 0;
}

static int arduino_spi_release(struct inode *inode, struct file *filp){
	struct arduino_dev *dev;
	dev = filp->private_data;

	spin_lock_irq(&dev->spinlock);

	filp->private_data = NULL;
	dev->opened = false;

	spin_unlock_irq(&dev->spinlock);

	return 0;
}

static int arduino_spi_message(struct arduino_dev *dev, unsigned int len){
	struct spi_message msg = { };
	struct spi_transfer transfer = { };

	spi_message_init(&msg);

	transfer.tx_buf = dev->tx_buf;
	transfer.rx_buf = dev->rx_buf;
	transfer.len = len;
	transfer.speed_hz = dev->max_speed_hz;

	spi_message_add_tail(&transfer, &msg);
	int status = spi_sync(dev->spi, &msg);

	if(status == 0){
		status = msg.actual_length;
	}
	return status;
}

static int arduino_spi_read(struct file *filp, char __user *buf, size_t maxBytes, loff_t *f_pos){
	struct arduino_dev *dev = filp->private_data;
	if (maxBytes > BUF_SIZE)
		return -EMSGSIZE;

	struct spi_device *spi = dev->spi;

	struct spi_transfer t = {
		.rx_buf = buf,
		.len = maxBytes,
		.speed_hz = dev->max_speed_hz,
	};

	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

static int arduino_spi_write(struct file *filp, const char __user *buf, size_t maxBytes, loff_t *f_pos){
	struct arduino_dev *dev = filp->private_data;
	if (maxBytes > BUF_SIZE)
		return -EMSGSIZE;

	spin_lock_irq(&dev->spinlock);
	if(!copy_from_user(dev->tx_buf, buf, maxBytes)){
		arduino_spi_message(dev, maxBytes);
	}else{
		spin_unlock_irq(&dev->spinlock);
		printk(KERN_INFO "Error copying from user space address");
		return -EFAULT;
	}
	spin_unlock_irq(&dev->spinlock);
	return maxBytes;
}

static int arduino_probe(struct spi_device *spi){
	if(!arduino_spi){
		arduino_spi = kzalloc(sizeof *arduino_spi, GFP_KERNEL);
	}
	if (!arduino_spi){
		printk(KERN_INFO "Failed to allocate memory for spi device");
		return -ENOMEM;
	}

	arduino_spi->spi = spi;
	arduino_spi->opened = false;
	arduino_spi->max_speed_hz = 1000000;

	spi_set_drvdata(spi, arduino_spi);

	return 0;
}

static int arduino_remove(struct spi_device *spi){
	struct arduino_dev *mydev = spi_get_drvdata(spi);

	kfree(mydev);

	return 0;
}

static struct spi_driver arduino_driver = {
	.driver = {
		.name = "spitest",
		.owner = THIS_MODULE,
	},
	.probe = arduino_probe,
	.remove = arduino_remove,
};

static const struct file_operations arduino_fops = {
	.owner = THIS_MODULE,
	.open = arduino_spi_open,
	.release = arduino_spi_release,
	.read = arduino_spi_read,
	.write = arduino_spi_write,
};

static int __init arduino_spi_init(void){
	printk(KERN_INFO "Loading module...");

	arduino_spi = kzalloc(sizeof(*arduino_spi), GFP_KERNEL);
	spin_lock_init(&arduino_spi->spinlock);

	if(alloc_chrdev_region(&arduino_spi_num, 0, 1, DEV_NAME) < 0){
		printk(KERN_INFO "Error while allocating device number");
		kfree(arduino_spi);
		return -ENOMEM;
	}

	if(!(arduino_spi_cdev = cdev_alloc())){
		printk(KERN_INFO "Error allocating memory for cdev struct");
		kfree(arduino_spi);
		unregister_chrdev_region(arduino_spi_num, 1);
		return -ENOMEM;
	}

	arduino_spi_cdev->owner = THIS_MODULE;
	arduino_spi_cdev->ops = &arduino_fops;

	if(cdev_add(arduino_spi_cdev, arduino_spi_num, 1)){
		printk(KERN_INFO "Failed to add cdev object");
		kfree(arduino_spi);
		unregister_chrdev_region(arduino_spi_num, 1);
		return -ENOMEM;
	}

	if(!(arduino_spi_class = class_create(THIS_MODULE, DEV_NAME))){
		printk(KERN_INFO "Error while creating device class");
		kfree(arduino_spi);
		unregister_chrdev_region(arduino_spi_num, 1);
		cdev_del(arduino_spi_cdev);
		return -ENOMEM;
	}

	arduino_spi_dev = device_create(arduino_spi_class, NULL, arduino_spi_num, NULL, "%s", DEV_NAME);

	int registered = spi_register_driver(&arduino_driver);
	if(registered){
		printk(KERN_INFO "Error while registering driver");
		kfree(arduino_spi);
		unregister_chrdev_region(arduino_spi_num, 1);
		cdev_del(arduino_spi_cdev);
		device_destroy(arduino_spi_class, arduino_spi_num);
		class_destroy(arduino_spi_class);
		return -ENOMEM;
	}
	return 0;
}

static void __exit arduino_spi_exit(void){
	printk(KERN_INFO "Unloading module...");
	device_destroy(arduino_spi_class, arduino_spi_num);
	class_destroy(arduino_spi_class);
	cdev_del(arduino_spi_cdev);
	unregister_chrdev_region(arduino_spi_num, 1);
	kfree(arduino_spi);
}

module_init(arduino_spi_init);
module_exit(arduino_spi_exit);