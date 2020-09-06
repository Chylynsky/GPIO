#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/wait.h>

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
	dev_t dev_no;					/* Device minor and major number	*/
	struct cdev cdev;				/* Character device					*/
	struct buffer ibuf;				/* Input buffer						*/	
	struct buffer obuf;				/* Output buffer					*/
};

static wait_queue_head_t wq;
static unsigned int irq = -1;
static unsigned int pin = -1;

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

/* Write unsigned int number to the buffer							*/
static ssize_t buffer_write_uint(struct buffer* buf, unsigned int val);

/* Write ubyte to the buffer							*/
static ssize_t buffer_write_byte(struct buffer* buf, unsigned char val);

/* Read data from the buffer and write it to the specified pointer	*/
static ssize_t buffer_read(struct buffer* buf, size_t size, char* dest);

/* Read data from the buffer and write it to the user space pointer	*/
static ssize_t buffer_to_user(struct buffer* buf, size_t size, char* __user dest);

/* Copy int value from the buffer to the dest pointer (MSB first)	*/
static ssize_t buffer_read_uint(struct buffer* buf, unsigned int* dest);

/* Copy byte value from the buffer to the dest pointer (MSB first)	*/
static ssize_t buffer_read_byte(struct buffer* buf, unsigned char* dest);

/* Reallocate buffer memory											*/
static int buffer_extend_if_needed(struct buffer* buf, size_t size_needed);

/* gpiodev 'open' file operation									*/
static int device_open(struct inode* inode, struct file* file);

/* gpiodev 'release' file operation									*/
static int device_release(struct inode* inode, struct file* file);

/* gpiodev 'read' file operation									*/
static ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs);

/* gpiodev 'write' file operation									*/
static ssize_t device_write(struct file* file, const char* __user buff, size_t size, loff_t* offs);

static irqreturn_t irq_handler(int irq, void* dev_id);

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
	printk(KERN_INFO "module loaded\n");
	init_waitqueue_head(&wq);
	return gpiodev_setup(&dev, DEVICE_NAME);
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
	dev->cdev.ops = &gpiodev_fops;

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
	size_t last_byte = buf->head + buf->size;
	size_t i = 0U;
	while (i < size)
	{
		buf->arr[last_byte + i] = data[i];
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
	unsigned long bytes_not_copied = copy_from_user((void*)&buf->arr[buf->head + buf->size], (const void*)data, (unsigned long)size);
	unsigned long bytes_copied = size - bytes_not_copied;
	buf->size += bytes_copied;

	printk(KERN_INFO "%u bytes copied from user\n", bytes_copied);
	return bytes_copied;
}

ssize_t buffer_write_uint(struct buffer* buf, unsigned int val)
{
	if (buffer_extend_if_needed(buf, sizeof(unsigned int)))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	*((unsigned int*)&buf->arr[buf->head]) = val;
	buf->size += sizeof(unsigned int);

	return 0;
}

ssize_t buffer_write_byte(struct buffer* buf, unsigned char val)
{
	if (buffer_extend_if_needed(buf, sizeof(unsigned char)))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	*((unsigned char*)&buf->arr[buf->head]) = val;
	buf->size += sizeof(unsigned char);

	return 0;
}

ssize_t buffer_read(struct buffer* buf, size_t size, char* dest)
{
	/* Size of data to be read						*/
	const size_t bytes_to_copy = (buf->size > size) ? size : buf->size;

	/* Copy data to dest							*/
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
	if (buf->size == 0U)
	{
		return 0;
	}

	/* Size of data to be read						*/
	const ssize_t bytes_to_copy = (buf->size > size) ? size : buf->size;
	/* Number of bytes that could not be read		*/
	const ssize_t bytes_not_copied = (ssize_t)copy_to_user((void*)dest, (const void*)&buf->arr[buf->head], (unsigned long)bytes_to_copy);
	/* Bytes copied to dest							*/
	const ssize_t bytes_copied = size - bytes_not_copied;
	/* Set new buffer size							*/
	buf->size -= bytes_copied;
	/* Move head									*/
	buf->head += bytes_copied;

	/* Reset head if all data was copied to user	*/
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	printk(KERN_INFO "%u bytes copied to user\n", bytes_copied);
	return bytes_copied;
}

ssize_t buffer_read_uint(struct buffer* buf, unsigned int* dest)
{
	if (buf->size == 0U)
	{
		return -1;
	}
	
	*dest = *((unsigned int*)(&buf->arr[buf->head]));

	buf->head += sizeof(unsigned int);
	buf->size -= sizeof(unsigned int);

	/* Reset head if all data was copied to dest */
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	return 0;
}

ssize_t buffer_read_byte(struct buffer* buf, unsigned char* dest)
{
	if (buf->size == 0U)
	{
		return -1;
	}

	*dest = *((unsigned char*)(&buf->arr[buf->head]));

	buf->head += sizeof(unsigned char);
	buf->size -= sizeof(unsigned char);

	/* Reset head if all data was copied to dest */
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	return 0;
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

		printk(KERN_INFO "succesfully allocated %u bytes\n", requested_size);
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
	wake_up_interruptible(&wq);

	if (irq != -1)
	{
		free_irq(irq, DEVICE_NAME);
	}

	buffer_free(&dev.ibuf);
	buffer_free(&dev.obuf);
	return 0;
}

ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "read operation\n");

	if (dev.obuf.size == 0U)
	{
		wait_event_interruptible_timeout(wq, dev.obuf.size != 0U, 100);
	}

	return buffer_to_user(&dev.obuf, size, buff);
}

ssize_t device_write(struct file* file, const char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "write operation\n");
	ssize_t bytes_read = buffer_from_user(&dev.ibuf, buff, size);
	
	{
		unsigned int pin_no;
		if (buffer_read_uint(&dev.ibuf, &pin_no))
		{
			printk(KERN_INFO "unable to retrieve pin number from buffer\n");
			return -1;
		}

		pin = (unsigned int)pin_no;
	}

	if ((irq = gpio_to_irq(pin)) < 0)
	{
		printk(KERN_INFO "gpio mapping to irq %u failed\n", irq);
		return -1;
	}

	if (request_irq(irq, irq_handler, IRQF_TRIGGER_NONE, "pin", DEVICE_NAME) < 0)
	{
		printk(KERN_INFO "request irq %u failed\n", irq);
		return -1;
	}
	
	printk(KERN_INFO "gpio %u mapped to irq %u\n", pin, irq);
	return bytes_read;
}

irqreturn_t irq_handler(int irq, void* dev_id)
{
	printk(KERN_INFO "interrupt detected on pin %u\n", pin);
	buffer_write_uint(&dev.obuf, pin);
	wake_up_interruptible(&wq);
	return IRQ_HANDLED;
}
