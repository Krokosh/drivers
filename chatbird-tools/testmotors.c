#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../chatbird/chatbird_ioctl.h"

int main(int argc, char *argv[])
{
  int val=0x8203;
  int fp=open("/dev/chatbird2",O_RDWR);
  ioctl(fp,CHATBIRD_IOCSETMOTOR,&val);
  close(fp);
}
