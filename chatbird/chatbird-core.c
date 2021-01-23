#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <sound/core.h>
#include "chatbird.h"

#define USB_CHATBIRD_VENDOR_ID 0x03ee
#define USB_CHATBIRD_PRODUCT_ID 0xff01

#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME "chatbird"
#endif

int chatbird_send_44bytes(struct chatbird_dev *chatbird, char *buf)
{
  int nRet, nSent;
  nRet=usb_interrupt_msg(chatbird->device,
		    usb_sndintpipe(chatbird->device,2),
		    buf,
		    44,
		    &nSent,
		    2 * HZ);
  if(nRet)
    {
      printk("interrupt returned %d, sent %d\n",nRet,nSent);
      return nRet;
    }
  return nSent;
}

static void chatbird_irq(struct urb *urb)
{
  int i;
  struct chatbird_dev *chatbird = urb->context;
  printk("Interrupt %x",chatbird->data[0]);
  for(i=0;i<4;i++)
    if(chatbird->data[0]&(1<<i))
      chatbird->buttonstate|=(1<<i);
  switch (urb->status)
    {
    case 0:			/* success */
      break;
    case -ECONNRESET:	/* unlink */
    case -ENOENT:
    case -ESHUTDOWN:
      return;
      /* -EPIPE:  should clear the halt */
    default:		/* error */
      break;
    }

  usb_submit_urb (urb, GFP_ATOMIC);
}
    
int chatbird_control_40(struct chatbird_dev *chatbird, __u16 value, __u16 index)
{
  printk("chatbird type 40 value %x index %x\n",value, index);
  int nRet=usb_control_msg(chatbird->device,
		  usb_sndctrlpipe(chatbird->device, 0),
		  0x01, // request 1
		  0x40, // type 0x40
		  value, // Value
		  index, // Index
		  NULL,
		  0x00,
		  2 * HZ);
  if(nRet)
    printk("control returned %d\n",nRet);
  return nRet;
}

static int chatbird_probe(struct usb_interface *interface,
    const struct usb_device_id *id)
{
  struct chatbird_dev *chatbird=NULL;
  struct usb_device *device = interface_to_usbdev(interface);
  int ret;
  int i;
  int nSent;
  printk("chatbird_probe: %x:%x, %x.%x, %x.%x.%x/%x.%x.%x %x\n",
	 id->idVendor,id->idProduct, id->bcdDevice_lo, id->bcdDevice_hi,
	 id->bDeviceClass,id->bDeviceSubClass,id->bDeviceProtocol,
	 id->bInterfaceClass,id->bInterfaceSubClass,id->bInterfaceProtocol,
	 id->bInterfaceNumber);
  __u16 *buffer=devm_kzalloc(&device->dev,44,GFP_KERNEL);
  chatbird=devm_kzalloc(&device->dev,sizeof (struct chatbird_dev), GFP_KERNEL);
  if (!chatbird)
    return -ENOMEM;
  printk("Allocated structure\n");
  chatbird->device = device;
  chatbird->interface=interface;
  ret = usb_set_interface(chatbird->device, 0, 0);
  if (ret < 0)
    {
      printk("cannot set interface.\n");
      return ret;
    }
  usb_set_intfdata(interface, chatbird);
  usb_make_path(device, chatbird->usb_path, sizeof(chatbird->usb_path));
  chatbird->hwptr=0;
  
  chatbird->data = usb_alloc_coherent(device, 44, GFP_ATOMIC, &chatbird->data_dma);
  if (!chatbird->data)
    return -ENOMEM;
  printk("Allocated DMA\n");
  #if 1
  usb_control_msg(chatbird->device,
		  usb_sndctrlpipe(chatbird->device, 0),
		  0x09,
		  0x00,
		  0x01,
		  0x00,
		  NULL,
		  0x00,
		  2 * HZ);
  #else
  usb_set_configuration(chatbird->device,1);
  #endif
  printk("Set configuration\n");
  chatbird_control_40(chatbird, 0xaf05, 0x0037);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc00, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc05, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc03, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  msleep_interruptible(500);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  msleep_interruptible(500);
  
 #if 0
  usb_interrupt_msg(chatbird->device,
		    usb_rcvintpipe(chatbird->device,0x81),
		    buffer,
		    44,
		    &nSent,
		    2 * HZ);
#else
  chatbird->irq = usb_alloc_urb(0, GFP_KERNEL);
  if (!chatbird->irq)
    return -ENOMEM;
  usb_fill_int_urb(chatbird->irq,
		   chatbird->device,
		   usb_rcvintpipe(chatbird->device,0x81),
		   chatbird->data,
		   1,
		   chatbird_irq, chatbird, 8);
  chatbird->irq->transfer_dma = chatbird->data_dma;
  chatbird->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
  
  ret = usb_submit_urb(chatbird->irq , GFP_KERNEL);
  if (ret)
    {
      return ret;
    }
  
  #endif
  for(i=0;i<256;i++)
    {
      int j;
      for(j=0;j<22;j++)
	buffer[j]=j*i-255;
      usb_interrupt_msg(chatbird->device,
			usb_sndintpipe(chatbird->device,2),
			buffer,
			44,
			&nSent,
			2 * HZ);
    }
  return chatbird_init(chatbird,interface);
}

static void chatbird_disconnect(struct usb_interface *interface)
{
  struct chatbird_dev *chatbird = (struct chatbird_dev *)usb_get_intfdata (interface);
  usb_set_intfdata(interface, NULL);
  if(chatbird)
    {
      chatbird_deinit(chatbird,interface);
      usb_kill_urb(chatbird->irq);
      usb_free_urb(chatbird->irq);
      usb_free_coherent(interface_to_usbdev(interface), 44, chatbird->data, chatbird->data_dma);
    }
}			

/* table of devices that work with this driver */
static struct usb_device_id chatbird_table [] = {
  { USB_DEVICE(USB_CHATBIRD_VENDOR_ID, USB_CHATBIRD_PRODUCT_ID) },
  { }                      /* Terminating entry */
};

struct usb_driver chatbird_driver = {
  .name        = "chatbird",
  .probe       = chatbird_probe,
  .disconnect  = chatbird_disconnect,
  //.fops        = &chatbird_fops,
  //.minor       = USB_CHATBIRD_MINOR_BASE,
  .id_table    = chatbird_table,
};

static int __init usb_chatbird_init(void)
{
  int result;
  
  /* register this driver with the USB subsystem */
  result = usb_register(&chatbird_driver);
  if (result < 0) {
    printk("usb_register failed for the "__FILE__ "driver."
	   "Error number %d", result);
    return -1;
  }
  
  return 0;
}
module_init(usb_chatbird_init);

static void __exit usb_chatbird_exit(void)
{
  /* deregister this driver with the USB subsystem */
  usb_deregister(&chatbird_driver);
}
module_exit(usb_chatbird_exit);

MODULE_DEVICE_TABLE (usb, chatbird_table);

MODULE_DESCRIPTION("Chatbird driver");
MODULE_AUTHOR("Stephen Crocker");
MODULE_LICENSE("GPL");
