#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>

#define DRIVER_VERSION "0.0.1"
#define DEVICE_NAME "gpiodev"

/* Return values */
#define SUCCESS 0
#define FAILURE -1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Borys Chylinski");
MODULE_DESCRIPTION("Helper device driver for GPIO library.");
MODULE_VERSION(DRIVER_VERSION);

struct gpiodev
{
	dev_t numbers;
	struct cdev cdev;
};

/* Function declarations */

static int gpiodev_init(void);
static void gpiodev_exit(void);
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);

/* Global variables */

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

static dev_t dev_no;
static struct cdev cdev;

/* Function definitions */

static int __init gpiodev_init(void) 
{
	int result = alloc_chrdev_region(&dev_no, 0U, 1U, DEVICE_NAME);

	if (result != SUCCESS)
	{
		printk(KERN_ALERT "alloc_chrdev_region failed\n");
		return FAILURE;
	}

	cdev_init(&cdev, &fops);

	cdev.owner = THIS_MODULE;
	cdev.ops = &fops;

	result = cdev_add(&cdev, dev_no, 1);

	if (result != SUCCESS)
	{
		printk(KERN_ALERT "cdev_add failed\n");
		return FAILURE;
	}

	printk(KERN_INFO "module loaded\n");
	return SUCCESS;
}

static void __exit gpiodev_exit(void) 
{
	cdev_del(&cdev);
	unregister_chrdev_region(dev_no, 1U);
	printk(KERN_INFO "module unloaded\n");
}

static int device_open(struct inode* inode, struct file* file)
{
	printk(KERN_INFO "open!\n");
	return SUCCESS;
}

static int device_release(struct inode* inode, struct file* file)
{
	printk(KERN_INFO "release!\n");
	return SUCCESS;
}

static ssize_t device_read(struct file* file, char* buff, size_t len, loff_t* offs)
{
	printk(KERN_INFO "read!\n");
	return SUCCESS;
}

static ssize_t device_write(struct file* file, const char* buff, size_t len, loff_t* offs)
{
	printk(KERN_INFO "write!\n");
	return SUCCESS;
}

module_init(gpiodev_init);
module_exit(gpiodev_exit);