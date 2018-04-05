/* Seperate Output device driver using the bufer instaniated in the input device

extern - declare character array as extern to use the same character array that was used in input.c

account for mutex locks in code
*/
//character mode linux device driver kernel module
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define SUCCESS 0
#define DEVICE_NAME "outputDev"	//device name as it appears in /proc/devices
#define CLASS_NAME "chardriver"
#define BUFF_LEN 1024			//max length of message

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Evangeliste, Eric Watson, Alexander ALvarez");
MODULE_DESCRIPTION("Assignment 3 COP4600");
MODULE_VERSION("1.0");

static int 		Major;	//major number assigned to our device driver
extern  char 	*msg;	//the msg the device will give when asked
static int 		counter = 0;
extern  short	size_of_message;
extern struct mutex charMutex;
static struct 	class* charClass = NULL;
static struct 	device* charDevice = NULL;
static int 		dev_open(struct inode *, struct file *);
static int 		dev_release(struct inode *, struct file *);
static ssize_t 	dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t 	dev_write(struct file *, const char *, size_t, loff_t *);

//Operations struct
static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

//this function is called when the module is loaded
static int __init outputDevice_init(void)
{
	mutex_init(&charMutex);
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if(Major < 0)
	{
		printk(KERN_ALERT "Output: Registering char device failed with %d\n", Major);
		return Major;	
	}

	printk(KERN_INFO "Output: New Session Created: #%d\n", Major);

	//register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);

	if(IS_ERR(charClass))
	{
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Output: Failed to register device class\n");
		return PTR_ERR(charClass);
	}

	printk(KERN_INFO "Output: Device class created correctly\n");

	charDevice = device_create(charClass, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME);

	if(IS_ERR(charDevice))
	{
		class_destroy(charClass);
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Output: Failed to create device class\n");
		return PTR_ERR(charDevice);
	}

	printk(KERN_INFO "Output: Device class created correctly");
	return SUCCESS;
}

//this function is called when the module is unloaded
static void __exit outputDevice_exit(void)
{
	mutex_destroy(&charMutex);
	device_destroy(charClass, MKDEV(Major, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "Output: Goodbye from the LKM!\n");
}

//called when a process tries to open the device file, like "cat/dev/mycharfile"
static int dev_open(struct inode *inode, struct file *file)
{
	if(!mutex_trylock(&charMutex))
	{
		printk(KERN_ALERT "Output charMutex: Device in use by another process");
		return -EBUSY;
	}

	counter++;
	printk(KERN_INFO "Output: Device opened! Access counter: %d\n", counter);
	return 0;
}

static ssize_t dev_read(struct file * filp, char *buffer, size_t length, loff_t * offset)
{
	//number of bytes actually written to the buffer
	int i, temp, error_count = copy_to_user(buffer, msg, length); //copy to user returns how many bytes were not copied

	if(error_count == 0)
	{
		for(i = 0; i < BUFF_LEN; i++)
		{
			if(i < (BUFF_LEN - length))
				msg[i] = msg[i + length];
			else
				msg[i] = '\0';
		}

		size_of_message -= length;
		printk(KERN_INFO "Output: User has obtained %d characters from system, %d bytes are available\n", length, BUFF_LEN - size_of_message);

		mutex_unlock(&charMutex);

		return length;
	}
	else
	{
		for(i = 0; i < BUFF_LEN; i++)
			msg[i] = '\0';

		mutex_unlock(&charMutex);

		if(size_of_message > 1)
		{
			printk(KERN_INFO "Output: User has obtained %d characters from system, %d bytes are available\n", size_of_message, BUFF_LEN);
			temp = size_of_message;
			size_of_message = 0;
	
			

			return temp;
		}
		else
			return -EFAULT;
	}
	
	
}

//called when a process closes the device file
static int dev_release(struct inode *inode, struct file *file)
{
	mutex_unlock(&charMutex);
	printk(KERN_INFO "Output: Device successfully closed\n");
	return 0;
}

static ssize_t dev_write(struct file *filp, const char *buff, size_t length, loff_t * off)
{
	printk(KERN_ALERT "The output module does not support writing.\n");

	return -EFAULT;
}
module_init(outputDevice_init);
module_exit(outputDevice_exit);
