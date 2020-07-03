#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Borys Chylinski");
MODULE_DESCRIPTION("Helper device driver for GPIO library.");
MODULE_VERSION("0.0.1");

static int __init GPIOdriver_init(void) {

	return 0;
}
static void __exit GPIOdriver_exit(void) {
	
}

module_init(GPIOdriver_init);
module_exit(GPIOdriver_exit);