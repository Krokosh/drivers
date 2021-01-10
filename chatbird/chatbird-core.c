#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/memalloc.h>

#define USB_CHATBIRD_VENDOR_ID 0x03ee
#define USB_CHATBIRD_PRODUCT_ID 0xff01

#define PERIODS 65536
#define SAMPLERATE		11025

struct chatbird_dev {
  struct usb_interface	*interface;  struct usb_device *device;
  struct urb *irq;
  unsigned char *data;
  dma_addr_t data_dma;
  struct usb_endpoint_descriptor	*int_in, *int_out;
  char usb_path[32];
  struct snd_card *card;
  struct snd_pcm *pcm;
  unsigned long hwptr;
};

static void chatbird_irq(struct urb *urb)
{
  struct chatbird_dev *chatbird = urb->context;
  printk("Interrupt %x",chatbird->data[0]);
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

static struct snd_device_ops ops = { };

static const struct snd_pcm_hardware snd_chatbird_pcm_hw = {
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER ),
	.formats		= SNDRV_PCM_FMTBIT_S16,
	//.rates			= SNDRV_PCM_RATE_11025,
	.rate_min		= 8000,
	.rate_max		= 48000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 44 * PERIODS,
	.period_bytes_min	= 44,
	.period_bytes_max	= 44,
	.periods_min		= PERIODS,
	.periods_max		= PERIODS,
};

static int snd_chatbird_pcm_open(struct snd_pcm_substream *ss)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  struct snd_pcm_runtime *runtime = ss->runtime;
  printk("snd_chatbird_pcm_open++\n");
  runtime->hw = snd_chatbird_pcm_hw;  
  chatbird_control_40(chatbird_dev, 0xbc00, 0x1388);
  printk("snd_chatbird_pcm_open--\n");
}

static int snd_chatbird_pcm_close(struct snd_pcm_substream *ss)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  printk("snd_chatbird_pcm_close++\n");

  printk("snd_chatbird_pcm_close--\n");
}


static int snd_chatbird_pcm_trigger(struct snd_pcm_substream *ss, int cmd)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  printk("snd_chatbird_pcm_trigger++\n");
 
  int ret = 0;
  
  //  spin_lock(&chatbird_pcm->lock);
  
  switch (cmd) {
  case SNDRV_PCM_TRIGGER_START:
  case SNDRV_PCM_TRIGGER_RESUME:
    printk("Start\n");
    chatbird_control_40(chatbird_dev, 0xbc05, 0x1388);
    
    break;
  case SNDRV_PCM_TRIGGER_STOP:
  case SNDRV_PCM_TRIGGER_SUSPEND:
    printk("Stop\n");
    chatbird_control_40(chatbird_dev, 0xbc03, 0x1388);
    break;
  default:
    ret = -EINVAL;
  }
  
  //  spin_unlock(&chatbird_pcm->lock);
  
  return ret;
}

static int snd_chatbird_pcm_copy_kernel(struct snd_pcm_substream *ss, int channel,
				    unsigned long pos, void *buf,
				    unsigned long count)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  int err, i,j;
  int nSent;
  printk("snd_chatbird_pcm_copy_kernel++\n");

  printk("Sending %x bytes\n",count);
  for(j=0;j<count;j+=44)
    {
      usb_interrupt_msg(chatbird_dev->device,
			usb_sndintpipe(chatbird_dev->device,2),
			(unsigned char *)buf+i*44,
			44,
			&nSent,
			2 * HZ);
      chatbird_dev->hwptr+=44;
    }
  return 0;
}


static int snd_chatbird_pcm_prepare(struct snd_pcm_substream *ss)
{
  printk("snd_chatbird_pcm_prepare++\n");
  return 0;
}


static snd_pcm_uframes_t snd_chatbird_pcm_pointer(struct snd_pcm_substream *ss)
{
  size_t ptr;
  printk("snd_chatbird_pcm_pointer++\n");
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  ptr=bytes_to_frames(ss->runtime, chatbird_dev->hwptr);
  printk("hw %d, pointer %d\n",chatbird_dev->hwptr, ptr);
  return ptr;
}

static int snd_chatbird_pcm_copy_user(struct snd_pcm_substream *ss, int channel,
				  unsigned long pos, void __user *dst,
				  unsigned long count)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  int err, i,j;
  char buf[44];	

  int nSent;
  //printk("snd_chatbird_pcm_copy_user++\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
  count*=2;
#endif

  //  printk("Sending %d bytes\n",count);
  for(i=0;i<count;i+=44)
    {
      copy_from_user(buf,(unsigned char *)dst+i,44);

      usb_interrupt_msg(chatbird_dev->device,
			usb_sndintpipe(chatbird_dev->device,2),
			buf,
			44,
			&nSent,
			2 * HZ);
      //mdelay(1);
      chatbird_dev->hwptr+=44;
    }
  return 0;
}

static int snd_chatbird_pcm_ioctl(struct snd_pcm_substream *ss,
		     unsigned int cmd, void *arg)
{
  return snd_pcm_lib_ioctl(ss,cmd,arg);
}

/* hw_params callback */
static int snd_chatbird_pcm_hw_params(struct snd_pcm_substream *substream,
                             struct snd_pcm_hw_params *hw_params)
{
  return snd_pcm_lib_malloc_pages(substream,
				  params_buffer_bytes(hw_params));
}

/* hw_free callback */
static int snd_chatbird_pcm_hw_free(struct snd_pcm_substream *substream)
{
  return snd_pcm_lib_free_pages(substream);
}

static const struct snd_pcm_ops snd_chatbird_pcm_ops = {
  .open = snd_chatbird_pcm_open,
  .close = snd_chatbird_pcm_close,
  .prepare = snd_chatbird_pcm_prepare,
  .trigger = snd_chatbird_pcm_trigger,
  .pointer = snd_chatbird_pcm_pointer,
  .ioctl = snd_chatbird_pcm_ioctl,
  .hw_params =	snd_chatbird_pcm_hw_params,
  .hw_free =	snd_chatbird_pcm_hw_free,
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,15,0)
  .copy_user = snd_chatbird_pcm_copy_user,
  .copy_kernel = snd_chatbird_pcm_copy_kernel,
#else
  .copy = snd_chatbird_pcm_copy_user,  
#endif
};


static int chatbird_snd_pcm_init(struct chatbird_dev *chatbird_dev)
{
  struct snd_card *card = chatbird_dev->card;
  struct snd_pcm *pcm;
  struct snd_pcm_substream *ss;
  int ret;
  int i;
  printk("chatbird_snd_pcm_init++\n");
  ret = snd_pcm_new(card, card->driver, 0, 1, 0,
		    &pcm);
  if (ret < 0)
    return ret;
  
  snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
		  &snd_chatbird_pcm_ops);
  
  snd_pcm_chip(pcm) = chatbird_dev;
  pcm->info_flags = 0;
  strscpy(pcm->name, card->shortname, sizeof(pcm->name));
  
  for (i = 0, ss = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
       ss; ss = ss->next, i++)
    {
      sprintf(ss->name, "#%d Audio", i);
      printk("Added %s\n",ss->name);
    }
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,9,0)
  snd_pcm_set_managed_buffer_all(pcm,
				 SNDRV_DMA_TYPE_CONTINUOUS,
				 NULL,
				 44 * PERIODS,
				 44 * PERIODS);
#else  
  snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					NULL,
					44*PERIODS, 44*PERIODS);
  #endif
  chatbird_dev->pcm = pcm;
  
  printk("chatbird_snd_pcm_init--\n");
  return 0;
}

static int snd_chatbird_volume_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *info)
{
	info->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	info->count = 1;
	info->value.integer.min = 0;
	info->value.integer.max = 15;
	info->value.integer.step = 1;

	return 0;
}

static int snd_chatbird_volume_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *value)
{
	struct chatbird_dev *chatbird_dev = snd_kcontrol_chip(kcontrol);
	u8 ch = value->id.numid - 1;

	value->value.integer.value[0] = 100;

	return 0;
}

static int snd_chatbird_volume_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *value)
{
	struct chatbird_dev *chatbird_dev = snd_kcontrol_chip(kcontrol);
	u8 ch = value->id.numid - 1;
	u8 old_val;

	old_val = 100;
	if (old_val == value->value.integer.value[0])
		return 0;


	return 1;
}

static const struct snd_kcontrol_new snd_chatbird_volume = {
	.iface = SNDRV_CTL_ELEM_IFACE_HWDEP,
	.name = "Master Playback Switch",
	.index=0,
	.info = snd_chatbird_volume_info,
	.get = snd_chatbird_volume_get,
	.put = snd_chatbird_volume_put,
};

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
  usb_fill_int_urb(chatbird->irq, chatbird->device, 0x81, chatbird->data,
		   1,
		   chatbird_irq, chatbird, 8);
  chatbird->irq->transfer_dma = chatbird->data_dma;
  chatbird->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
  
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

  
  ret = snd_card_new(&chatbird->device->dev,
		     SNDRV_DEFAULT_IDX1, "chatbird", THIS_MODULE, 0,
		     &chatbird->card);
  if (ret < 0)
    return ret;

  strscpy(chatbird->card->driver, "chatbird", sizeof(chatbird->card->driver));
  strscpy(chatbird->card->shortname, "PC Mascot Audio", sizeof(chatbird->card->shortname));
  sprintf(chatbird->card->longname, "%s on %s", chatbird->card->shortname,chatbird->usb_path);
	  
  ret = snd_device_new(chatbird->card, SNDRV_DEV_LOWLEVEL, chatbird, &ops);
  if (ret < 0)
    return ret;
#if 0
  struct snd_kcontrol_new kctl;

  strscpy(chatbird->card->mixername, "CHATBIRD", sizeof(chatbird->card->mixername));
  kctl = snd_chatbird_volume;
  kctl.count = 1;
  
  ret = snd_ctl_add(chatbird->card, snd_ctl_new1(&kctl, chatbird));
  if (ret < 0)
    {
      printk("snd_ctl_add returned %d\n",ret);
      return ret;
    }
  #endif
  ret = chatbird_snd_pcm_init(chatbird);
  if (ret < 0)
    return ret;
  
  ret = snd_card_register(chatbird->card);
  if (ret < 0)
    {
      printk("snd_card_register returned %d\n",ret);
      return ret;
    }

  return ret;
}

static void chatbird_disconnect(struct usb_interface *interface)
{
  struct chatbird_dev *chatbird = usb_get_intfdata (interface);
  usb_set_intfdata(interface, NULL);
  if(chatbird)
    {      
      snd_card_free(chatbird->card);
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
