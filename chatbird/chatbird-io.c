#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <asm/ioctl.h>
#include "chatbird.h"
#include "chatbird_ioctl.h"

extern struct usb_driver chatbird_driver;

static int chatbird_open(struct inode *inode, struct file *file)
{
  struct chatbird_dev *dev;
  struct usb_interface *interface;
  int subminor;
  int retval = 0;
  
  subminor = iminor(inode);
  dev_dbg(&dev->device->dev,"chatbird_open++(%d)\n",subminor);
  
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
  dev_dbg(&dev->device->dev,"Found %x\n",dev);
#ifdef USE_AUTOPM
  retval = usb_autopm_get_interface(interface);
  if (retval)
    goto exit;
#endif 
  /* save our object in the file's private structure */
  file->private_data = dev;
  
exit:
  dev_dbg(&dev->device->dev,"chatbird_open--: %d\n",retval);
  return retval;
}

static int chatbird_release(struct inode *inode, struct file *file)
{
  struct chatbird_dev *dev;
  
  dev = file->private_data;
  if (dev == NULL)
    return -ENODEV;
  
#ifdef USE_AUTOPM
  /* allow the device to be autosuspended */
  usb_autopm_put_interface(dev->interface);
#endif
  return 0;
}

static ssize_t chatbird_write(struct file *file, const char __user *user_buffer,
			      size_t count, loff_t *ppos)
{
  struct chatbird_dev *dev;
  int retval = 0;
  int todo=count;
  dev = file->private_data;
  int i;

  //rintk("Sending %d bytes\n",count);
  for(i=0;i<count;i+=44)
    {
      int ret;
      copy_from_user(dev->data,(unsigned char *)user_buffer+i,min(44,todo));
      ret=chatbird_send_44bytes(dev,dev->data);
      if(ret<0)
	{
	  dev_err(&dev->device->dev,"Send returned %d\n",ret);
	}
      retval+=ret;
      todo-=ret;
    }
  return retval;
}

static long chatbird_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  struct chatbird_dev *dev;
  int val;
  dev = file->private_data;
  void __user *argp = (void __user *)arg;
  copy_from_user(&val,argp,sizeof(int));
  
  switch(cmd)
    {
    case CHATBIRD_IOCSETMOTOR:
      {
	int val;
	copy_from_user(&val,argp,sizeof(int));
	dev_dbg(&dev->device->dev,"Set motor %x\n",val);
	chatbird_control_40(dev, val&0xffff, val>>16);
	return 0;
      }
    case CHATBIRD_IOCGETBUTTONS:
      {
	int val=dev->buttonstate;
	dev->buttonstate=0;
	dev_dbg(&dev->device->dev,"Get buttons %x\n",val);
	copy_to_user(argp,&val,sizeof(int));
	return 0;
      }
    default:
      dev_err(&dev->device->dev,"Unknown IOCTL %x:%x/%x\n",cmd,arg,val);
      return -EINVAL;
    }
}

static const struct file_operations chatbird_fops = {
  .owner =	THIS_MODULE,
  //.read =	chatbird_read,
  .write =	chatbird_write,
  .open =	chatbird_open,
  .unlocked_ioctl =	chatbird_ioctl,
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
  dev_dbg(&chatbird->device->dev,"chatbird_init(%x,%x)\n",chatbird,interface);
  int retval = usb_register_dev(interface, &chatbird_class);
  if (retval) {
    /* something prevented us from registering this driver */
    dev_err(&interface->dev,
	    "Not able to get a minor for this device.\n");
    return retval;
  }
  
  /* let the user know what node this device is now attached to */
  dev_info(&interface->dev,
	   "USB Chatbird device now attached to USBChatbird-%d",
	   interface->minor);
  return 0;
}

int chatbird_deinit(struct chatbird_dev *chatbird,struct usb_interface *interface)
{
  dev_dbg(&chatbird->device->dev,"chatbird_deinit(%x,%x)\n",chatbird,interface);
  usb_deregister_dev(interface, &chatbird_class);
  return 0;
}
