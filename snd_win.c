/* 
  GZX - George's ZX Spectrum Emulator
  pcm playback through windows waveOut

  Momentalne se zda byt ok: rozumna latence, nezasekava se.
  Sla by latence jeste snizit? Je to rozumne?
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <windows.h>
#include "mgfx.h"
#include "sndw.h"

/*
  Latence <= N_BUF * 40 ms
  Aktualne: 120 ms
*/

//#define N_BUF 3
#define N_BUF 5

static int sbufn=0;   /* index prvniho bloku, ktery je submitnuty */
static int ebufn=0;   /* index prvniho bloku, ktery je prazdny */
static int ns,ne;

static int running=0;
static int server_on=0;

static int buf_size;

static HWAVEOUT hwaveout;
static u8 *sndbuf[N_BUF];
static WAVEHDR wavehdr[N_BUF];

/*
  kruhova fronta

  playing   empty     playing
  ........|.........|.......
          e         s
*/

int sndw_init(int bufs) {
  int play_rate;
  WAVEFORMATEX wfx;
  MMRESULT errcode;
  int i;

  buf_size=bufs;

  play_rate=28000;

  wfx.wFormatTag = 1; /* PCM */
  wfx.nChannels = 1;  /* mono */
  wfx.nSamplesPerSec = play_rate;
  wfx.nAvgBytesPerSec = play_rate;
  wfx.nBlockAlign = 1;
  wfx.wBitsPerSample = 8;
  wfx.cbSize = 0;

  errcode = waveOutOpen( &hwaveout,WAVE_MAPPER,&wfx,
    (DWORD_PTR)NULL,(DWORD_PTR)NULL, // User data.
    (DWORD)CALLBACK_NULL);

  if (errcode != MMSYSERR_NOERROR) return -1;

  waveOutPause(hwaveout);

  /* alokuj buffery */

  for(i=0;i<N_BUF;i++) {
    sndbuf[i]=malloc(buf_size);
    memset(&wavehdr[i],0,sizeof(WAVEHDR));
    wavehdr[i].lpData=(LPSTR)sndbuf[i];
    wavehdr[i].dwBufferLength=buf_size;
    waveOutPrepareHeader(hwaveout,&wavehdr[i],sizeof(WAVEHDR));
  }

  /* ... */

  server_on=1;

  sbufn=ebufn=0;
  ns=0;
  ne=N_BUF;
  running=0;

  return 0;
}

void sndw_done(void) {
  int i;

  printf("wout_close...\n");
  if(server_on) {
    waveOutReset(hwaveout);

    for(i=0;i<N_BUF;i++) {
      if(wavehdr[i].dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(hwaveout,&wavehdr[i],sizeof(WAVEHDR));
        free(sndbuf[i]); sndbuf[i]=NULL;
      }
    }

    waveOutClose(hwaveout);
    server_on=0;
  }
}

/* check whether there are finished sound buffers */
static void wout_checkfinished(void) {
  if(!server_on) {
    printf("called wout_checkfinished with server_on=0!!\n");
    exit(1);
  }
  while(ns>0 && (wavehdr[sbufn].dwFlags & WHDR_DONE)) {
    if(++sbufn>=N_BUF) sbufn=0;
    ne++; ns--;
//    fprintf(stdout,"playing buf finished (%d,%d,%d) (ns=%d,ne=%d)\n",
//      fbufn,ebufn,sbufn,ns,ne);

    /* stop playback on underrun */
    if(ns==0) {
      fprintf(stdout,"sound buffer underrun - stopping playback\n");
      waveOutPause(hwaveout);
      running=0;
    }
  }
}

/**********************************************/

void sndw_write(u8 *buf) {
  int c;
  
  c=0;
  /* possibly wait for a sound buffer to finish playing */
  while(ne==0) { 
//    printf("sound: wait for queue to empty\n");
//    printf("running=%d, ne=%d, ns=%d\n",running,ne,ns);
    mgfx_input_update();
    wout_checkfinished();
    usleep(1000);
    if(++c>300) {
      printf("sound lockup?!\n");
      printf("diagnostic: ns=%d ne=%d\n"
        "sbufn=%d ebufn=%d\n"
	"running=%d\n",ns,ne,sbufn,ebufn,running);
      printf("current buf: dwFlags=%lu\n",wavehdr[sbufn].dwFlags);
      c=0;
    }
  }

  /* insert buffer to our queue */
  if(ne!=0) {
    MMRESULT res;
    
    memcpy(sndbuf[ebufn],buf,buf_size);
    res = waveOutWrite(hwaveout,&wavehdr[ebufn],sizeof(WAVEHDR));
    if(res != MMSYSERR_NOERROR) {
      printf("waveOutWrite returns error %d\n",res);
    }
    
    if(++ebufn>=N_BUF) ebufn=0;
    ne--; ns++;
  } else {
    fprintf(stdout,"sound buffer overrun (dropped)\n");
  }

  /* start/restart playback */
  if(!running) {
    /* jen pokud je fronta plna */
    if(ne==0) {
      fprintf(stdout,"starting playback\n");
      running=1;
      waveOutRestart(hwaveout);
    }
  }
}

