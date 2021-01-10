  
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
  
int chatbird_control_40(struct chatbird_dev *chatbird, __u16 value, __u16 index);
  
