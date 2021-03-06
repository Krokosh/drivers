#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../chatbird/chatbird_ioctl.h"

int main(int argc, char *argv[])
{
  int i;
  char *szDev="/dev/chatbird0";
  
  for(i=0;i<argc;i++)
    if(argv[i][0]=='-')
      switch(argv[i][1])
        {
	case 'd':
	  szDev=argv[++i];
	  break;
	}
  int val=0x01008000;
  i=0;
  int fp=open(szDev,O_RDWR);
  int tmp;
  do
    {
      ioctl(fp,CHATBIRD_IOCGETBUTTONS,&tmp);
      if(tmp&2)
	{
	  i++;
	  printf("+%x\n",i);
	}
      if(tmp&4)
	{
	  i--;
	  printf("-%x\n",i);
	}
      if(tmp&1)
	{
	  int cmd=val+(i&0x07);
	  printf("!%x\n",i);
	  ioctl(fp,CHATBIRD_IOCSETMOTOR,&cmd);
	}
      usleep(1000);
    } while((tmp&8)==0);
  close(fp);
}
