#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include "chatbird.h"

extern struct usb_driver chatbird_driver;

static int chatbird_open(struct inode *inode, struct file *file)
{
  struct chatbird_dev *dev;
  struct usb_interface *interface;
  int subminor;
  int retval = 0;
  
  subminor = iminor(inode);
  
  interface = usb_find_interface(&chatbird_driver, subminor);
  if (!interface) {
    pr_err("%s - error, can't find device for minor %d\n",
	   __func__, subminor);
    retval = -ENODEV;
    goto exit;
  }
  
  dev = usb_get_intfdata(interface);
  if (!dev) {
    retval = -ENODEV;
    goto exit;
  }
  
  retval = usb_autopm_get_interface(interface);
  if (retval)
    goto exit;
  
  /* save our object in the file's private structure */
  file->private_data = dev;
  
exit:
  return retval;
}

static int chatbird_release(struct inode *inode, struct file *file)
{
  struct chatbird_dev *dev;
  
  dev = file->private_data;
  if (dev == NULL)
    return -ENODEV;
  
  /* allow the device to be autosuspended */
  usb_autopm_put_interface(dev->interface);
  
  return 0;
}

static ssize_t chatbird_write(struct file *file, const char *user_buffer,
			      size_t count, loff_t *ppos)
{
  struct chatbird_dev *dev;
  int retval = 0;
  struct urb *urb = NULL;
  char *buf = NULL;
  
  dev = file->private_data;

  
}

static const struct file_operations chatbird_fops = {
  .owner =	THIS_MODULE,
  //.read =	chatbird_read,
  .write =	chatbird_write,
  .open =	chatbird_open,
  .release =	chatbird_release,
  //.flush =	chatbird_flush,
  //.llseek =	noop_llseek,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver chatbird_class = {
  .name =		"chatbird%d",
  .fops =		&chatbird_fops,
  .minor_base =	0,
};


int chatbird_init(struct chatbird_dev *chatbird,struct usb_interface *interface)
{
  int retval = usb_register_dev(interface, &chatbird_class);
  if (retval) {
    /* something prevented us from registering this driver */
    dev_err(&interface->dev,
	    "Not able to get a minor for this device.\n");
    usb_set_intfdata(interface, NULL);
    return retval;
  }
  
  /* let the user know what node this device is now attached to */
  dev_info(&interface->dev,
	   "USB Chatbirdeton device now attached to USBChatbird-%d",
	   interface->minor);
  return 0;
}

int chatbird_deinit(struct chatbird_dev *chatbird)
{
}
