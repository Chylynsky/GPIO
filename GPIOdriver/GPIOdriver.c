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
#include <linux/uaccess.h>

#define DRIVER_VERSION "0.0.1"
#define DEVICE_NAME "gpiodev"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Borys Chylinski");
MODULE_DESCRIPTION("Helper device driver for GPIO library.");
MODULE_VERSION(DRIVER_VERSION);


/*
	Structs
*/

/* Dynamic data structure, to be used with buffer_* functions	*/
struct buffer
{
	char* arr;			/* Pointer to the dynamic array			*/
	size_t head;		/* Number of the read bytes				*/
	size_t size;		/* Size of the memory in use			*/
	size_t capacity;	/* Allocated memory size				*/
};

/* gpiodev device representation */
struct gpiodev
{
	dev_t dev_no;			/* Device minor and major number	*/
	struct cdev cdev;		/* Character device					*/
	struct buffer ibuf;		/* Input buffer						*/	
	struct buffer obuf;		/* Output buffer					*/			
};

/*
	Function declarations
*/

/* Module entry point 												*/
static int gpiodev_init(void);

/* Module exit point 												*/
static void gpiodev_exit(void);

/* Setup gpiodev structure 											*/
static int gpiodev_setup(struct gpiodev* dev, const char* name);

/* Destroy gpiodev structure 										*/
static void gpiodev_destroy(struct gpiodev* dev);

/* Initialize buffer 												*/
static void buffer_init(struct buffer* buf);

/* Deallocate buffer												*/
static void buffer_free(struct buffer* buf);

/* Write data to the buffer											*/
static ssize_t buffer_write(struct buffer* buf, const char* data, size_t size);

/* Write data from user space to the buffer							*/
static ssize_t buffer_from_user(struct buffer* buf, const char* __user data, size_t size);

/* Read data from the buffer and write it to the specified pointer	*/
static ssize_t buffer_read(struct buffer* buf, size_t size, char* dest);

/* Read data from the buffer and write it to the user space pointer	*/
static ssize_t buffer_to_user(struct buffer* buf, size_t size, char* __user dest);

/* Reallocate buffer memory */
static int buffer_extend_if_needed(struct buffer* buf, size_t size_needed);

/* gpiodev 'open' file operation									*/
static int device_open(struct inode* inode, struct file* file);

/* gpiodev 'release' file operation									*/
static int device_release(struct inode* inode, struct file* file);

/* gpiodev 'read' file operation									*/
static ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs);

/* gpiodev 'write' file operation									*/
static ssize_t device_write(struct file* file, const char* __user buff, size_t size, loff_t* offs);

/* 
	Global variables 
*/

/* gpiodev instance */
static struct gpiodev dev;

/* File operations function pointers assignments */
static struct file_operations gpiodev_fops = {
	.owner		= THIS_MODULE,
	.open		= device_open,
	.release	= device_release,
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

int gpiodev_setup(struct gpiodev* dev, const char* name)
{
	int err = alloc_chrdev_region(&dev->dev_no, 0U, 1U, name);

	if (err)
	{
		printk(KERN_ALERT "alloc_chrdev_region failed\n");
		goto unregister;
	}

	cdev_init(&dev->cdev, &gpiodev_fops);
	dev->cdev.owner	= THIS_MODULE;
	dev->cdev.ops	= &gpiodev_fops;

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

void gpiodev_destroy(struct gpiodev* dev)
{
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->dev_no, 1U);
}

void buffer_init(struct buffer* buf)
{
	buf->arr = NULL;
	buf->head = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

void buffer_free(struct buffer* buf)
{
	kfree((void*)buf->arr);
	buf->arr = NULL;
	buf->head = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

ssize_t buffer_write(struct buffer* buf, const char* data, size_t size)
{
	/*
		Size of the data to write summed with number of unread bytes
		should be less or equal the buffer's capacity
	*/
	if (buffer_extend_if_needed(buf, size))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	/* Copy data to buffer */
	size_t i = 0U;
	while (i < size)
	{
		buf->arr[buf->head + i] = data[i];
		i++;
	}

	buf->size += size;	/* Set new size */
		
	return size;
}

ssize_t buffer_from_user(struct buffer* buf, const char* __user data, size_t size)
{
	/*
		Size of the data to write summed with number of unread bytes
		should be less or equal the buffer's capacity
	*/
	if (buffer_extend_if_needed(buf, size))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	/* Copy data to buffer */
	unsigned long bytes_not_copied = copy_from_user((void*)&buf->arr[buf->size], (const void*)data, (unsigned long)size);
	unsigned long bytes_copied = size - bytes_not_copied;
	buf->size += bytes_copied;

	printk(KERN_INFO "copied %u bytes from user\n", bytes_copied);
	return bytes_copied;
}

ssize_t buffer_read(struct buffer* buf, size_t size, char* dest)
{
	/* Size of data to be read						*/
	const size_t bytes_to_copy = (buf->size > size) ? size : buf->size;

	/* Copy data to dest */
	size_t i = 0U;
	while (i < bytes_to_copy)
	{
		dest[i] = buf->arr[buf->head];
		buf->head++;
		i++;
	}
	/* Set new buffer size							*/
	buf->size -= bytes_to_copy;

	/* Reset head if all data was copied to user	*/
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	return bytes_to_copy;
}

ssize_t buffer_to_user(struct buffer* buf, size_t size, char* __user dest)
{
	/* Size of data to be read						*/
	const size_t bytes_to_copy = (buf->size > size) ? size : buf->size;
	/* Number of bytes that could not be read */
	const size_t bytes_not_copied = (size_t)copy_to_user((void*)dest, (const void*)&buf->arr[buf->head], (unsigned long)bytes_to_copy);
	/* Bytes copied to dest */
	const size_t bytes_copied = size - bytes_not_copied;
	/* Set new buffer size							*/
	buf->size -= bytes_copied;
	/* Move head									*/
	buf->head += bytes_copied;

	/* Reset head if all data was copied to user	*/
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	printk(KERN_INFO "copied %u bytes to user\n", bytes_copied);
	return bytes_copied;
}

int buffer_extend_if_needed(struct buffer* buf, size_t size_needed)
{
	const size_t unread_bytes = buf->size;
	const size_t space_left = buf->capacity - buf->head + unread_bytes;

	/*
		Resize if data can't fit in the space left
	*/
	if (space_left < size_needed)
	{
		const size_t requested_size = unread_bytes + size_needed;
		/* Allocate more memory and assign it to temporary pointer */
		char* tmp = (char*)kmalloc(requested_size * sizeof(char), GFP_KERNEL);

		if (tmp == NULL)
		{
			printk(KERN_ALERT "buffer_extend_if_needed kmalloc error\n");
			return -1;
		}

		/* Copy unread bytes to the new memory location */
		size_t i = buf->head;
		while (i != buf->size)
		{
			tmp[i] = buf->arr[buf->head + i];
			i++;
		}

		kfree((void*)buf->arr);				/* Deallocate old memory				*/
		buf->arr		= tmp;				/* Assign new memory to the buffer		*/
		buf->head		= 0U;				/* Reset head							*/
		buf->size		= unread_bytes;		/* Set new size							*/
		buf->capacity	= requested_size;	/* Set new capacity						*/

		printk(KERN_INFO "succesfully allocated %i bytes\n", requested_size);
	}

	return 0;
}

int device_open(struct inode* inode, struct file* file)
{
	buffer_init(&dev.ibuf);
	buffer_init(&dev.obuf);
	return 0;
}

int device_release(struct inode* inode, struct file* file)
{
	buffer_free(&dev.ibuf);
	buffer_free(&dev.obuf);
	return 0;
}

ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "read!\n");
	return (ssize_t)buffer_to_user(&dev.ibuf, size, buff);;
}

ssize_t device_write(struct file* file, const char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "write!\n");
	return (ssize_t)buffer_from_user(&dev.ibuf, buff, size);;
}