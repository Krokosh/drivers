#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/version.h>

#define USB_CHATBIRD_VENDOR_ID 0x03ee
#define USB_CHATBIRD_PRODUCT_ID 0xff01

struct chatbird_dev {
  struct usb_interface	*interface;
  struct usb_device *device;
  struct urb *irq;
  unsigned char data[44];
  struct usb_endpoint_descriptor	*int_in, *int_out;
};

static void chatbird_irq(struct urb *urb)
{
  struct chatbird_dev *chatbird = urb->context;
  printk("Interrupt %x",chatbird->data[0]);
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

  __u16 *buffer=devm_kzalloc(&device->dev,44,GFP_KERNEL);

  chatbird=devm_kzalloc(&device->dev,sizeof (struct chatbird_dev), GFP_KERNEL);
  chatbird->device = device;
  chatbird->interface=interface;
  ret = usb_set_interface(chatbird->device, 0, 0);
  if (ret < 0)
    {
      printk("cannot set interface.\n");
      return ret;
    }

  #if 0
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
  chatbird_control_40(chatbird, 0xaf05, 0x0037);
  chatbird_control_40(chatbird, 0xbc00, 0x1388);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  chatbird_control_40(chatbird, 0xbc05, 0x1388);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  chatbird_control_40(chatbird, 0xbc03, 0x1388);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  chatbird_control_40(chatbird, 0xbc01, 0x1388);
  
  chatbird->irq = usb_alloc_urb(0, GFP_KERNEL);
  if (!chatbird->irq)
    return -ENOMEM;
  
#if 0
  usb_interrupt_msg(chatbird->device,
		    usb_rcvintpipe(chatbird->device,0x81),
		    buffer,
		    44,
		    &nSent,
		    2 * HZ);
  #else
  usb_fill_int_urb(chatbird->irq, chatbird->device, 0x81, chatbird->data,
		   1,
		   chatbird_irq, chatbird, 8);
  
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
  return ret;
}

static void chatbird_disconnect(struct usb_interface *interface)
{
}			

/* table of devices that work with this driver */
static struct usb_device_id chatbird_table [] = {
  { USB_DEVICE(USB_CHATBIRD_VENDOR_ID, USB_CHATBIRD_PRODUCT_ID) },
  { }                      /* Terminating entry */
};

static struct usb_driver chatbird_driver = {
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
