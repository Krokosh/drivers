//speech synth 
//using espeak and chatbird
//Based on example written by Bill Heaster
//Ported to Mitsumi "Chatbird" PC Mascot by Stephen Crocker.
/*
 * This is the most minimalistic approach to getting espeak running with the C api
 *
 * This was tested in Debian 10 I had to download the following packages. 
 * espeak-data
 * libespeak-dev
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <espeak/speak_lib.h>
#include <sys/ioctl.h>
#include "../chatbird/chatbird_ioctl.h"

int fp;

/* Must be called before any synthesis functions are called.
   This specifies a function in the calling program which is called when a buffer of
   speech sound data has been produced.

   The callback function is of the form:

int SynthCallback(short *wav, int numsamples, espeak_EVENT *events);

   wav:  is the speech sound data which has been produced.
      NULL indicates that the synthesis has been completed.

   numsamples: is the number of entries in wav.  This number may vary, may be less than
      the value implied by the buflength parameter given in espeak_Initialize, and may
      sometimes be zero (which does NOT indicate end of synthesis).

   events: an array of espeak_EVENT items which indicate word and sentence events, and
      also the occurance if <mark> and <audio> elements within the text.  The list of
      events is terminated by an event of type = 0.


   Callback returns: 0=continue synthesis,  1=abort synthesis.
*/
//don't delete this callback function.

int flags=0x01008000;

int SetMotor(int val,int length)
{
  flags&=0x000000c0;
  flags|=length<<16;
  flags|=0x8000|val;
  //printf("Sending IOCTL %x/%x\n",CHATBIRD_IOCSETMOTOR,flags);
  ioctl(fp,CHATBIRD_IOCSETMOTOR,&flags);
}

int SetLEDs(int val)
{
  flags&=0xffffff3f;
  flags=val<<6;
  ioctl(fp,CHATBIRD_IOCSETMOTOR,&flags);
}

int ledstate=0;

int SynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
  int i;
  printf("received %d samples\n",numsamples);
  if(fp==0)
    printf("Failed to open: %d\n",errno);
  SetMotor(3,numsamples);
  ledstate=0;
  int average=0;

  for(i=0;i<numsamples;i+=44)
    {
      short buf[44];
      int j;
      average=0;
      for(j=0;j<22;j++)
	{
	  buf[j]=wav[i+j*2];
	  average+=abs(buf[j]);
	}
      average/=22;
      int nRet=write(fp,buf,44);
      if(average>1024)
	ledstate|=1;
      if(average>2048)
	ledstate|=2;
      //printf("Average %x\n",average);
      if(((i/44)&3)==0)
	SetLEDs(ledstate);
      //printf("Write=%d/%d\n",nRet,errno);
      //fflush(fp);
    }
  return 0;
}


int main(int argc, char *argv[])
{
    int i;
    espeak_ERROR speakErr;
    char *szDev="/dev/chatbird0";
    char *szText="Hello, this is a test!";
    char *szVoice="english-rp";


    for(i=0;i<argc;i++)
      if(argv[i][0]=='-')
	switch(argv[i][1])
        {
          case 'd':
            szDev=argv[++i];
            break;

          case 't':
            szText=argv[++i];
            break;

          case 'v':
            szVoice=argv[++i];
            break;
        }

    fp=open(szDev,O_RDWR);
    SetMotor(4,128);
    usleep(500000);
    //must be called before any other functions
    //espeak initialize
    int nRet=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,0,NULL,espeakINITIALIZE_PHONEME_EVENTS);
    if(nRet <0)
    {
      printf("could not initialize espeak: %d\n",nRet);
        return -1;
    }
    printf("sample rate=%d\n",nRet);
    espeak_SetParameter(espeakVOLUME,25,0);
    espeak_SetParameter(espeakPITCH,60,0);
    espeak_SetParameter(espeakRANGE,100,0);
    SetMotor(5,128);
    espeak_SetVoiceByName(szVoice);
    espeak_SetSynthCallback(SynthCallback);
    
    //make some text to spit out
    char textBuff[255]={0};

    usleep(2000000);
    if((speakErr=espeak_Synth(szText, strlen(szText), 0,0,0,espeakCHARS_AUTO,NULL,NULL))!= EE_OK)
    {
        puts("error on synth creation\n");
              
    }
    SetLEDs(0);
    SetMotor(5,256);
    close(fp);
    

    return 0;
}
