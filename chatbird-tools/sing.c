#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>
#include "../chatbird/chatbird_ioctl.h"

int fp;

int fillsine(short *buf, float pitch)
{
  for (unsigned i = 0; i < 22; i++)
    {
      buf[i] = (short)(256*sin((i * pitch * M_PI ) / 160.0));
    }

}

int main(int argc, char *argv[])
{
  int i;
  short buf[22];
  char *szDev="/dev/chatbird0";
  
  for(i=0;i<argc;i++)
    if(argv[i][0]=='-')
      switch(argv[i][1])
        {
	case 'd':
	  szDev=argv[++i];
	  break;
	  
        }

  fp=open(szDev,O_RDWR);
  int tmp;
  float pitch=1;
  int cmd=0;
  fillsine(buf, pitch);
  do
    {
      ioctl(fp,CHATBIRD_IOCGETBUTTONS,&tmp);
      if(tmp&2)
	{
	  pitch-=0.1;
	  fillsine(buf, pitch);
	}
      if(tmp&4)
	{
	  pitch+=0.1;
	  fillsine(buf, pitch);
	}
      if(tmp&1)
	{
	  cmd++;
	  cmd%=4;
	  int val=0x01008000+(cmd+2);
	  ioctl(fp,CHATBIRD_IOCSETMOTOR,&val);
	}
      write(fp,buf,44);
      //usleep(5000);
      //pitch+=0.1;
      //fillsine(buf, pitch);
      usleep(1000);
    } while((tmp&8)==0);
  close(fp);
}
