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

/* Dynamic data structure, to be used with buffer_* functions	*/
struct io_buffer
{	
	char* arr;			/* Pointer to the dynamic array			*/
	size_t head;		/* Number of the read bytes				*/
	size_t size;		/* Size of the memory in use			*/
	size_t capacity;	/* Allocated memory size				*/
};

/* Struct representing list node with irq to gpio mapping. */
struct _irq_mapping
{
	unsigned int irq;
	unsigned int gpio;
	struct _irq_mapping* next;
};

/* Helper typedef to avoid using _irq_mapping* directly. */
typedef struct _irq_mapping* irq_mapping;

/* gpiodev device representation */
struct gpiodev
{
	dev_t dev_no;				/* Device minor and major number	*/
	wait_queue_head_t wq;		/* Device wait queue				*/		
	irq_mapping irq_map;		/* List with gpio-irq mapping		*/
	struct cdev cdev;			/* Character device					*/
	struct io_buffer ibuf;		/* Input io_buffer					*/	
	struct io_buffer obuf;		/* Output io_buffer					*/
};

/* 
* Communication with the module is done by executing commands represented
* by the command_t struct.
*/
struct command_t
{
	unsigned char type;
	unsigned int pin_number;
};

/* Helper macros for command_t struct.									*/
#define CMD_DETACH_IRQ (unsigned char)0
#define CMD_ATTACH_IRQ (unsigned char)1
#define CMD_CHECK_SIZE(size) (size == sizeof(struct command_t)) ? 1 : 0

/*
	Function declarations
*/

/* Module entry point 													*/
static int gpiodev_init(void);

/* Module exit point 													*/
static void gpiodev_exit(void);

/* Setup gpiodev structure 												*/
static int gpiodev_setup(struct gpiodev* dev, const char* name);

/* Destroy gpiodev structure 											*/
static void gpiodev_destroy(struct gpiodev* dev);

/* Initialize io_buffer 												*/
static void buffer_init(struct io_buffer* buf);

/* Deallocate io_buffer													*/
static void buffer_free(struct io_buffer* buf);

/* Write data to the io_buffer											*/
static ssize_t buffer_write(struct io_buffer* buf, const char* data, size_t size);

/* Write data from user space to the io_buffer							*/
static ssize_t buffer_from_user(struct io_buffer* buf, const char* __user data, size_t size);

/* Write unsigned int number to the io_buffer							*/
static ssize_t buffer_write_uint(struct io_buffer* buf, unsigned int val);

/* Write ubyte to the io_buffer											*/
static ssize_t buffer_write_byte(struct io_buffer* buf, unsigned char val);

/* Read data from the io_buffer and write it to the specified pointer	*/
static ssize_t buffer_read(struct io_buffer* buf, size_t size, char* dest);

/* Read data from the io_buffer and write it to the user space pointer	*/
static ssize_t buffer_to_user(struct io_buffer* buf, size_t size, char* __user dest);

/* Copy int value from the io_buffer to the dest pointer (MSB first)	*/
static ssize_t buffer_read_uint(struct io_buffer* buf, unsigned int* dest);

/* Copy byte value from the io_buffer to the dest pointer (MSB first)	*/
static ssize_t buffer_read_byte(struct io_buffer* buf, unsigned char* dest);

/* Reallocate io_buffer memory											*/
static int buffer_extend_if_needed(struct io_buffer* buf, size_t size_needed);

/* Initialize irq_mapping struct.										*/
static void irq_mapping_init(irq_mapping* map);

/* Destroy irq_mapping struct and free irqs.							*/
static void irq_mapping_destroy(irq_mapping* map);

/* Request new irq, create mapping with gpio and push new node.			*/
static int irq_mapping_push(irq_mapping* map, unsigned int gpio);

/* Destroy irq mapping node based on given gpio.						*/
static int irq_mapping_erase_gpio(irq_mapping* map, unsigned int gpio);

/* Get gpio mapped to a given irq number.								*/
static unsigned int irq_mapping_get_gpio(irq_mapping* map, unsigned int irq);

/* gpiodev 'open' file operation										*/
static int device_open(struct inode* inode, struct file* file);

/* gpiodev 'release' file operation										*/
static int device_release(struct inode* inode, struct file* file);

/* gpiodev 'read' file operation										*/
static ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs);

/* gpiodev 'write' file operation										*/
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

	buffer_init(&dev->ibuf);
	buffer_init(&dev->obuf);
	init_waitqueue_head(&dev->wq);
	irq_mapping_init(&dev->irq_map);

	return 0;

delete_cdev:
	cdev_del(&dev->cdev);

unregister:
	unregister_chrdev_region(dev->dev_no, 1U);

	return -1;
}

void gpiodev_destroy(struct gpiodev* dev)
{
	irq_mapping_destroy(&dev->irq_map);
	wake_up_interruptible(&dev->wq);
	buffer_free(&dev->ibuf);
	buffer_free(&dev->obuf);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->dev_no, 1U);
}

void buffer_init(struct io_buffer* buf)
{
	buf->arr = NULL;
	buf->head = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

void buffer_free(struct io_buffer* buf)
{
	kfree((void*)buf->arr);
	buf->arr = NULL;
	buf->head = 0U;
	buf->size = 0U;
	buf->capacity = 0U;
}

ssize_t buffer_write(struct io_buffer* buf, const char* data, size_t size)
{
	/*
		Size of the data to write summed with number of unread bytes
		should be less or equal the io_buffer's capacity
	*/
	if (buffer_extend_if_needed(buf, size))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	/* Copy data to io_buffer */
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

ssize_t buffer_from_user(struct io_buffer* buf, const char* __user data, size_t size)
{
	/*
		Size of the data to write summed with number of unread bytes
		should be less or equal the io_buffer's capacity
	*/
	if (buffer_extend_if_needed(buf, size))
	{
		printk(KERN_ALERT "buffer_extend_if_needed failed\n");
		return -1;
	}

	/* Copy data to io_buffer */
	unsigned long bytes_not_copied = copy_from_user((void*)&buf->arr[buf->head + buf->size], (const void*)data, (unsigned long)size);
	unsigned long bytes_copied = size - bytes_not_copied;
	buf->size += bytes_copied;

	printk(KERN_INFO "%u bytes copied from user\n", bytes_copied);
	return bytes_copied;
}

ssize_t buffer_write_uint(struct io_buffer* buf, unsigned int val)
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

ssize_t buffer_write_byte(struct io_buffer* buf, unsigned char val)
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

ssize_t buffer_read(struct io_buffer* buf, size_t size, char* dest)
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
	/* Set new io_buffer size							*/
	buf->size -= bytes_to_copy;

	/* Reset head if all data was copied to user	*/
	if (buf->size == 0U)
	{
		buf->head = 0U;
	}

	return bytes_to_copy;
}

ssize_t buffer_to_user(struct io_buffer* buf, size_t size, char* __user dest)
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
	/* Set new io_buffer size							*/
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

ssize_t buffer_read_uint(struct io_buffer* buf, unsigned int* dest)
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

ssize_t buffer_read_byte(struct io_buffer* buf, unsigned char* dest)
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

int buffer_extend_if_needed(struct io_buffer* buf, size_t size_needed)
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
		buf->arr		= tmp;				/* Assign new memory to the io_buffer		*/
		buf->head		= 0U;				/* Reset head							*/
		buf->size		= unread_bytes;		/* Set new size							*/
		buf->capacity	= requested_size;	/* Set new capacity						*/

		printk(KERN_INFO "succesfully allocated %u bytes\n", requested_size);
	}

	return 0;
}

void irq_mapping_init(irq_mapping* map)
{
	*map = NULL;
}

void irq_mapping_destroy(irq_mapping* map)
{
	struct _irq_mapping* curr = *map;
	struct _irq_mapping* next = NULL;

	if (curr == NULL)
	{
		return;
	}

	while (curr != NULL)
	{
		next = curr->next;
		free_irq(curr->irq, DEVICE_NAME);
		printk(KERN_INFO "irq %u freed\n", curr->irq);
		kfree(curr);
		curr = next;
	}
}

int irq_mapping_push(irq_mapping* map, unsigned int gpio)
{
	struct _irq_mapping* node = (struct _irq_mapping*)kmalloc(sizeof(struct _irq_mapping), GFP_KERNEL);

	if (node == NULL)
	{
		return -1;
	}

	if ((node->irq = gpio_to_irq(gpio)) < 0)
	{
		goto free_node;
	}

	if (request_irq(node->irq, irq_handler, IRQF_TRIGGER_NONE, "irq", DEVICE_NAME) < 0)
	{
		goto free_node;
	}

	node->gpio = gpio;
	node->next = NULL;

	if (*map == NULL)
	{
		*map = node;
	}
	else
	{
		node->next = *map;
		(*map) = node;
	}

	printk(KERN_INFO "irq %u mapped to gpio %u\n", node->irq, node->gpio);
	return 0;

free_node:
	kfree(node);
	return -1;
}

int irq_mapping_erase_gpio(irq_mapping* map, unsigned int gpio)
{
	struct _irq_mapping* curr = *map;
	struct _irq_mapping* prev = NULL;
	struct _irq_mapping* next = curr->next;

	if (curr == NULL)
	{
		return -1;
	}

	if (curr->gpio == gpio)
	{
		free_irq(curr->irq, DEVICE_NAME);
		printk(KERN_INFO "irq %u freed\n", curr->irq);
		kfree(curr);
		*map = next;

		return 0;
	}

	while (curr != NULL)
	{
		prev = curr;
		curr = next;
		next = curr->next;

		if (curr->gpio == gpio)
		{
			free_irq(curr->irq, DEVICE_NAME);
			printk(KERN_INFO "irq %u freed\n", curr->irq);
			kfree(curr);
			prev->next = next;
			return 0;
		}
	}

	return -1;
}

unsigned int irq_mapping_get_gpio(irq_mapping* map, unsigned int irq)
{
	struct _irq_mapping* curr = *map;
	struct _irq_mapping* next = NULL;

	if (curr == NULL)
	{
		return -1;
	}

	while (curr != NULL)
	{
		next = curr->next;

		if (curr->irq == irq)
		{
			return curr->gpio;
		}
	}

	return -1;
}

int device_open(struct inode* inode, struct file* file)
{
	return 0;
}

int device_release(struct inode* inode, struct file* file)
{
	wake_up_interruptible(&dev.wq);
	return 0;
}

ssize_t device_read(struct file* file, char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "read operation\n");

	if (dev.obuf.size == 0U)
	{
		wait_event_interruptible_timeout(dev.wq, dev.obuf.size != 0U, 100);
	}

	return buffer_to_user(&dev.obuf, size, buff);
}

ssize_t device_write(struct file* file, const char* __user buff, size_t size, loff_t* offs)
{
	printk(KERN_INFO "write operation\n");

	if (!CMD_CHECK_SIZE(size))
	{
		printk(KERN_INFO "bad command size\n");
		return -1;
	}

	ssize_t bytes_read = buffer_from_user(&dev.ibuf, buff, size);
	struct command_t cmd;

	if (buffer_read(&dev.ibuf, sizeof(cmd), (char*)&cmd) != sizeof(cmd))
	{
		printk(KERN_INFO "unable to retrieve pin number from buffer\n");
		return -1;
	}

	if (cmd.type == CMD_ATTACH_IRQ)
	{
		if (irq_mapping_push(&dev.irq_map, cmd.pin_number) < 0)
		{
			printk(KERN_INFO "unable to request irq\n");
			return -1;
		}

		return bytes_read;
	}
	else if (cmd.type == CMD_DETACH_IRQ)
	{
		if (irq_mapping_erase_gpio(&dev.irq_map, cmd.pin_number) < 0)
		{
			printk(KERN_INFO "unable to free irq\n");
			return -1;
		}

		return bytes_read;
	}
	else
	{
		printk(KERN_INFO "unknown command\n");
		return -1;
	}
}

irqreturn_t irq_handler(int irq, void* dev_id)
{
	buffer_write_uint(&dev.obuf, irq_mapping_get_gpio(&dev.irq_map, irq));
	wake_up_interruptible(&dev.wq);
	return IRQ_HANDLED;
}
