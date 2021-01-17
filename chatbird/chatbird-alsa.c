#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/memalloc.h>
#include "chatbird.h"

#define PERIODS 65536
#define SAMPLERATE		11025


static struct snd_device_ops ops = { };

static const struct snd_pcm_hardware snd_chatbird_pcm_hw = {
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER ),
	.formats		= SNDRV_PCM_FMTBIT_S16,
	//.rates			= SNDRV_PCM_RATE_11025,
	.rate_min		= 16000,
	.rate_max		= 16000,
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
  return 0;
}

static int snd_chatbird_pcm_close(struct snd_pcm_substream *ss)
{
  struct chatbird_dev *chatbird_dev = snd_pcm_substream_chip(ss);
  printk("snd_chatbird_pcm_close++\n");
  
  printk("snd_chatbird_pcm_close--\n");
  return 0;
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
  printk("snd_chatbird_pcm_copy_kernel++\n");
  struct snd_pcm_runtime *runtime = ss->runtime;

  printk("Sending %x bytes\n",count);
  for(i=0;i<count;i+=44)
    {
      chatbird_dev->hwptr+=chatbird_send_44bytes(chatbird_dev,buf+i);
    }
  chatbird_dev->hwptr%=runtime->buffer_size;
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

  struct snd_pcm_runtime *runtime = ss->runtime;
  int nSent;
  printk("snd_chatbird_pcm_copy_user++\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
  count*=2;
#endif

  printk("Sending %d bytes\n",count);
  for(i=0;i<count;i+=44)
    {
      copy_from_user(buf,(unsigned char *)dst+i,44);
      chatbird_dev->hwptr+=chatbird_send_44bytes(chatbird_dev,buf);
    }
  chatbird_dev->hwptr%=runtime->buffer_size;
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

int chatbird_init(struct chatbird_dev *chatbird,struct usb_interface *interface)
{
  int ret = snd_card_new(&chatbird->device->dev,
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

int chatbird_deinit(struct chatbird_dev *chatbird, struct usb_interface *interface)
{
  return snd_card_free(chatbird->card);
}
