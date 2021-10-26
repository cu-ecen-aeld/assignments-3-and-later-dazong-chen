/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "aesd-circular-buffer.h"
#include "aesdchar.h"


int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Dazong Chen"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	struct aesd_dev* 	dev;
	
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);    // find addr of aesd_dev structure and return to pointer
	filp->private_data = dev;    // stores the pointer in private data;
	 
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t                       retval = 0;
	size_t                        bytes_to_read = 0;
	struct aesd_dev*              dev = filp->private_data;
	struct aesd_buffer_entry*     entry = NULL;
	size_t                        entry_offset = 0;                       
	unsigned long                 uncopied_bytes = 0;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	
	/**
	 * TODO: handle read
	 */
	if(mutex_lock_interruptible(&dev->locker))
	{
		return -ERESTARTSYS;
	}
	
	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cbuff, *f_pos, &entry_offset);
	
	if(entry == NULL)
	{
		retval = 0;
		goto out;
		
	}
	
	bytes_to_read = entry->size - entry_offset;	// bytes need to be read in the rest of entry
	
	if(bytes_to_read > count)
	{
		bytes_to_read = count;
	}
	
	uncopied_bytes = copy_to_user(buf, (entry->buffptr+entry_offset), bytes_to_read);
	
	if(uncopied_bytes != 0)
	{
		PDEBUG("%lu bytes were not copied to user", uncopied_bytes);
		retval = -EFAULT;
		goto out;
	}
	
	*f_pos += bytes_to_read;	// update f_pos position
	retval = bytes_to_read;
	
    out:
    	mutex_unlock(&dev->locker);
    	return retval;
}


ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	unsigned long                 	uncopied_bytes = 0;
	struct aesd_dev*              	dev = filp->private_data;
	ssize_t			    	retval = -ENOMEM; 
	char*                            newline = NULL;
	char*				discard = NULL;
	
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	 
	if(mutex_lock_interruptible(&dev->locker))
	{
		return -ERESTARTSYS;
	}
	
	
	if(dev->buffer_entry.size == 0)		// doesn't exist
	{
		// allocate memory in kernel buffer
		dev->buffer_entry.buffptr = kmalloc(count*sizeof(char), GFP_KERNEL);
		
		if(dev->buffer_entry.buffptr == NULL)
		{
			goto out;
		}
	}
	
	else 	// realloc new buffptr that contains old and new data
	{
		dev->buffer_entry.buffptr = krealloc(dev->buffer_entry.buffptr, dev->buffer_entry.size+count, GFP_KERNEL);
		
		if(dev->buffer_entry.buffptr == NULL)
		{
			goto out;
		}
	}
	
	
	// copy user-space buffer into kernel buffer
	uncopied_bytes = copy_from_user( (dev->buffer_entry.buffptr+dev->buffer_entry.size), buf, count);    // copy to new data space (+dev->buffer_entry.size) and return how many bytes were not copied
	
	if(uncopied_bytes != 0)
	{
		PDEBUG("%lu bytes were not copied from user", uncopied_bytes);
	}
	
	retval = count - uncopied_bytes;
	
	dev->buffer_entry.size += retval;        
	
	// Write operations which do not include a \n character should be saved and appended by future write operations.
	newline = (char*)memchr(dev->buffer_entry.buffptr, '\n', dev->buffer_entry.size);    // find '\n' character
	
	if(newline != NULL)
	{
		discard = aesd_circular_buffer_add_entry(&dev->cbuff, &dev->buffer_entry);
		
		if(discard != NULL)
		{
			kfree(discard);
		}
		
		dev->buffer_entry.size = 0;
		dev->buffer_entry.buffptr = NULL;
	}
	
	*f_pos = 0;
    out:
	mutex_unlock(&dev->locker);
	return retval;
}


struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};


static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}


int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	
	mutex_init(&aesd_device.locker);
	
	aesd_circular_buffer_init(&aesd_device.cbuff);
	
	result = aesd_setup_cdev(&aesd_device);

	if( result )
	{
		unregister_chrdev_region(dev, 1);
	}
	
	return result;

}


void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	
	aesd_circular_buffer_free(&aesd_device.cbuff);
	
	unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
