RELEASE = $(shell uname -r)
RUNNING := /lib/modules/$(RELEASE)/build
KDIR := $(shell if test -d $(RUNNING); then echo $(RUNNING); else echo /usr/src/linux; fi)
TREE = $(shell echo $(RELEASE) | cut -d. -f1,2)
INSTALLDIR := /lib/modules/$(shell uname -r)/kernel/drivers/media/

ifeq ($(KERNELRELEASE),)

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) M=$(PWD) modules  
modules_install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) M=$(PWD) modules_install  
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers
else
chatbird-y := chatbird-core.o chatbird-io.o
snd-chatbird-y := chatbird-core.o chatbird-alsa.o

obj-m:= snd-chatbird.o chatbird.o
endif


install:
	cp chatbird.ko $(INSTALLDIR)
	depmod
