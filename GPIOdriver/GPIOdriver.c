#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>

#define DRIVER_VERSION "0.0.1"
#define DEVICE_NAME "gpiodev"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Borys Chylinski");
MODULE_DESCRIPTION("Helper device driver for GPIO library.");
MODULE_VERSION(DRIVER_VERSION);


/*
	Structs
*/

struct buffer
{
	char* arr;
	unsigned int index;
	unsigned int size;
	unsigned int capacity;
};

/* gpiodev device representation */
struct gpiodev
{
	dev_t dev_no;
	struct cdev cdev;
	struct buffer ibuf;
	struct buffer obuf;
};


/*
	Function declarations
*/

/* Module entry point */
static int gpiodev_init(void);

/* Module exit point */
static void gpiodev_exit(void);

/* Setup gpiodev structure */
static int gpiodev_setup(struct gpiodev* dev, const char* name);

/* Destroy gpiodev structure */
static void gpiodev_destroy(struct gpiodev* dev);

/* Initialize buffer */
static void buffer_init(struct buffer* buf);

/* Deallocate buffer */
static void buffer_free(struct buffer* buf);

/* Write data to the buffer */
static int buffer_write(struct buffer* buf, const char* data, size_t size);

/* Read data from the buffer and write it to the specified pointer */
static void buffer_read(struct buffer* buf, size_t size, char* dest);

/* gpiodev 'read' file operation */
static ssize_t device_read(struct file* file, char* __user buff, size_t len, loff_t* offs);

/* gpiodev 'write' file operation */
static ssize_t device_write(struct file* file, const char* __user buff, size_t len, loff_t* offs);


/* 
	Global variables 
*/

/* gpiodev instance */
static struct gpiodev dev;

/* File operations function pointers assignments */
static struct file_operations gpiodev_fops = {
	.owner		= THIS_MODULE,
	.read		= device_read,
	.write		= device_write
};

/* Declare module entry and exit points */
module_init(gpiodev_init);
module_exit(gpiodev_exit);


/* 
	Function definitions 
*/

int __init gpiodev_init(void) 
{
	int err = gpiodev_setup(&dev, DEVICE_NAME);

	if (err)
	{
		printk(KERN_ALERT "an error occured while loading the module\n");
		return -1;
	}

	printk(KERN_INFO "module loaded\n");
	return 0;
}

void __exit gpiodev_exit(void) 
{
	gpiodev_destroy(&dev);
	printk(KERN_INFO "module unloaded\n");
}

int gpiodev_setup(gpiodev* dev, const char* name)
{
	int err = alloc_chrdev_region(&dev->dev_no, 0U, 1U, name);

	if (err)
	{
		printk(KERN_ALERT "alloc_chrdev_region failed\n");
		goto unregister;
	}

	cdev_init(&dev->cdev, &gpiodev_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &gpiodev_fops;
	buffer_init(&dev->ibuf);
	buffer_init(&dev->obuf);

	err = cdev_add(&dev->cdev, dev->dev_no, 1);

	if (err)
	{
		printk(KERN_ALERT "cdev_add failed\n");
		goto delete_cdev;
	}

	return 0;

delete_cdev:
	cdev_del(&dev->cdev);

unregister:
	unregister_chrdev_region(dev->dev_no, 1U);

	return -1;
}

void gpiodev_destroy(gpiodev* dev)
{
	buffer_free(&dev->ibuf);
	buffer_free(&dev->obuf);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->dev_no, 1U);
}

void buffer_init(struct buffer* buf)
{
	buf->arr = NULL;
	buf->index = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

void buffer_free(struct buffer* buf)
{
	kfree((void*)buf->arr);
	buf->arr = NULL;
	buf->index = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

int buffer_write(struct buffer* buf, const char* data, size_t size)
{
	unsigned int unread_bytes = buf->size - buf->index + 1U;
	unsigned int requested_size = unread_bytes + size;

	/*
		Size of the data to write summed with number of unread bytes
		should be less or equal the buffer's capacity
	*/
	if (buf->capacity < requested_size)
	{
		/* Allocate more memory and assign it to temporary pointer */
		char* tmp = (char*)kmalloc(requested_size * sizeof(char), GFP_KERNEL);

		if (tmp == NULL)
		{
			return -1;
		}

		/* Copy unread bytes to the new memory */
		size_t new_index = 0U;
		for (; buf->index != buf->capacity; new_index++, buf->index++)
		{
			tmp[new_index] = buf->arr[buf->index];
		}

		kfree((void*)buf->arr);				/* Deallocate old memory */
		buf->arr = tmp;						/* Assign new memory to the buffer */
		buf->index = new_index;				/* Set index to the end of unread data */
		buf->size = requested_size;			/* Set new size */
		buf->capacity = requested_size;		/* Set new capacity */
	}

	/* Copy data to buffer */
	for (size_t i = 0U; i < size; i++)
	{
		buf->index++;
		buf->arr[buf->index] = data[i];
	}

	return 0;
}

void buffer_read(struct buffer* buf, size_t size, char* dest)
{
	
}

ssize_t device_read(struct file* file, char* __user buff, size_t len, loff_t* offs)
{
	printk(KERN_INFO "read!\n");
	return 0;
}

ssize_t device_write(struct file* file, const char* __user buff, size_t len, loff_t* offs)
{
	printk(KERN_INFO "write!\n");
	return 0;
}