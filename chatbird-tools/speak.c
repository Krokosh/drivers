//speech synth 
//using espeak 
//Written by Bill Heaster
/*
 * This is the most minimalistic approach to getting espeak running with the C api
 *
 * 
 * compile with gcc -W -o myEspeak myEspeak.c -lespeak
 * 
 * This was tested in ubuntu 15.10 I had to download the following packages. 
 * espeak-data
 * libespeak-dev
 * 
 * */

#include <stdio.h>
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

int SetMotor(int val)
{
  printf("Sending IOCTL %x/%x\n",CHATBIRD_IOCSETMOTOR,val);
  ioctl(fp,CHATBIRD_IOCSETMOTOR,&val);
}

int SynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
  int i;
  printf("received %d samples\n",numsamples);
  if(fp==NULL)
    printf("Failed to open: %d\n",errno);
  SetMotor(3);
  for(i=0;i<numsamples;i+=44)
    {
      short buf[44];
      int j;
      for(j=0;j<22;j++)
	buf[j]=wav[i+j*2];
      int nRet=write(fp,buf,44);
      //printf("Write=%d/%d\n",nRet,errno);
      //fflush(fp);
    }
  SetMotor(1);
  return 0;
}


int main(int argc, char *argv[])
{
    espeak_ERROR speakErr;

    fp=open("/dev/chatbird2",O_RDWR);
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
    SetMotor(5);

    espeak_SetSynthCallback(SynthCallback);
    
    //make some text to spit out
    char textBuff[255]={0};

    if(argc)
      strcpy(textBuff, argv[1]);
    else
      strcpy(textBuff, "Hello, this is a test!");
    usleep(2000000);
    if((speakErr=espeak_Synth(textBuff, strlen(textBuff), 0,0,0,espeakCHARS_AUTO,NULL,NULL))!= EE_OK)
    {
        puts("error on synth creation\n");
              
    }
    SetMotor(5);
    usleep(2000000);
    SetMotor(1);
    close(fp);
    

    return 0;
}
