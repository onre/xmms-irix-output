/******************************************************************************
 *
 * IRIX audio plugin for XMMS
 * 
 * This file contains most of the audio routines for talking to the sound
 * device. 
 *
 * Manuel Panea <mpd@rzg.mpg.de>, 1999
 *
 *****************************************************************************/


#include "IRIX.h"
#include "config.h"
#include <pthread.h>
#include <stropts.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>


/*****************************************************************************/

/*
  This is the maximum data we're willing to send to the audio device and
  when we should refill it.
*/
#define IRIX_AUDIO_CACHE 4096

/* This is our own buffer to get the audio data from xmms into */
/* it must be an integer multiple of IRIX_AUDIO_CACHE !! */
#define IRIX_BUFFER_CACHE 32768

/* A few support macros */
#define min(x, y) (x) < (y) ? (x) : (y)
#define max(x, y) (x) > (y) ? (x) : (y)
#define mod(x, y) ((x) - (int) ((x) / (y)) * (y))

/*****************************************************************************/

static char         *buffer_ptr = NULL;
static char         *buffer_end = NULL;
static char         *buffer_read = NULL;
static char         *buffer_write = NULL;
static char         *buffer_fillend = NULL;
static int           buffer_len = 0;
static struct timespec min_wait;

static gint          time_offset = 0;
static gint          byte_offset = 0;

static gboolean      paused = FALSE;
static gboolean      playing = FALSE;
static gboolean      filling = TRUE;
static gboolean      aopen = FALSE;
static gint          flush = -1;

static gint          audiofd = -2;
//static audio_info_t  adinfo;
static pthread_t      buffer_thread;

static uint_t        audio_scount = 0;

static struct {
  gboolean         is_signed;
  gboolean         is_bigendian;
  uint_t           sample_rate;
  uint_t           channels;
  uint_t           precision;
  gint             bps;
  gint             output_precision;
}                    stream_attrs;

/*****************************************************************************/

/*
 * xmms expects 0 <= volume <= 100
 */

void
irix_get_volume(int *left, int *right)
{
  double gainleft, gainright,
    *min = &irix_cfg.min_volume;
  ALpv pv[1];
  ALfixed gain[8];

  if (IRIX_DEBUG & IRIX_DEBUG_GET_VOLUME)
  {
    fprintf(stderr, "irix_get_volume: begin\n");
  }

  pv[0].param = AL_GAIN;
  pv[0].value.ptr = gain;
  pv[0].sizeIn = 8;
  if( alGetParams(AL_DEFAULT_OUTPUT, pv, 1) < 0)
  {
    fprintf(stderr, "irix_get_volume: alGetParams failed: %s\n", alGetErrorString(oserror()));
    return;
  }

  if ( (gainleft  = alFixedToDouble(gain[0])) < *min )  gainleft  = *min;
  if ( (gainright = alFixedToDouble(gain[1])) < *min )  gainright = *min;

  *left  = (gainleft  - *min) / irix_cfg.volume_ratio;
  *right = (gainright - *min) / irix_cfg.volume_ratio;

  if (IRIX_DEBUG & IRIX_DEBUG_GET_VOLUME)
  {
    fprintf(stderr, "*left='%d', *right='%d'\n", *left, *right);
    fprintf(stderr, "irix_get_volume: returning\n");
  }
}

/*****************************************************************************/
/*
 * Set volume (gain)
 */
void
irix_set_volume(int left, int right)
{
  double gainleft, gainright,
    *min = &irix_cfg.min_volume;
  ALpv pv[1];
  ALfixed gain[8];

  if (IRIX_DEBUG & IRIX_DEBUG_SET_VOLUME)
  {
    fprintf(stderr, "irix_set_volume: begin\n");
    fprintf(stderr, "left='%d', right='%d'\n", left, right);
  }

  gain[0] = alDoubleToFixed(*min + left  * irix_cfg.volume_ratio);
  gain[1] = alDoubleToFixed(*min + right * irix_cfg.volume_ratio);

  pv[0].param = AL_GAIN;
  pv[0].value.ptr = gain;
  pv[0].sizeIn = 8;
  if( alSetParams(AL_DEFAULT_OUTPUT, pv, 1) < 0)
  {
    fprintf(stderr, "irix_set_volume: alSetParams failed: %s\n", alGetErrorString(oserror()));
    return;
  }

  if (IRIX_DEBUG & IRIX_DEBUG_SET_VOLUME)
  {
    fprintf(stderr, "irix_set_volume: returning\n");
  }

}

/*****************************************************************************/

gint
irix_open_audio(AFormat fmt, gint rate, gint nch)
{
  ALpv	params;
  int frameSize, channelCount;

  if (IRIX_DEBUG & IRIX_DEBUG_OPEN_AUDIO)
  {
    fprintf(stderr, "irix_open_audio: begin. fmt='%d', rate='%d', nch='%d'\n",
          fmt, rate, nch);
  }

  /* Get a ALconfig handle */
  if ( (irix_cfg.myALconfig = alNewConfig()) < 0 )
  {
    fprintf(stderr, "irix_open_audio: alNewConfig failed: %s\n",
            alGetErrorString(oserror()));
    return 0;
  }

  /* Set the number of channels */
  alSetChannels(irix_cfg.myALconfig, nch);
  irix_cfg.channels = nch;

  if (IRIX_DEBUG & IRIX_DEBUG_OPEN_AUDIO)
  {
    fprintf(stderr, "irix_open_audio: opening port\n");
  }

  /* Open the port with the given ALconfig */
  if ( (irix_cfg.port = alOpenPort("xmms", "w", irix_cfg.myALconfig)) == NULL )
  {
    fprintf(stderr, "irix_init: alOpenPort failed: %s\n",
            alGetErrorString(oserror()));
    return;
  }
  irix_cfg.resource = alGetResource(irix_cfg.port);
  irix_cfg.maxFillable = alGetFillable(irix_cfg.port);

  /* Find out the combinedFrameSize */
  channelCount = alGetChannels(irix_cfg.myALconfig);
  frameSize = alGetWidth(irix_cfg.myALconfig);

  if (frameSize == 0 || channelCount == 0)
  {
    fprintf(stderr, "irix_open_audio: bad frameSize or channelCount\n");
    return;
  }
  irix_cfg.combinedFrameSize = frameSize * channelCount;

  /* Set the audio rate */
  irix_cfg.rate = rate;
  params.param = AL_RATE;
  params.value.ll = (long long) rate << 32;

  if (alSetParams(irix_cfg.resource, &params, 1) < 0)
  {
    fprintf(stderr, "irix_open_audio: alSetParams failed: %s\n", alGetErrorString(oserror()));
    return 0;
  }
  if( params.sizeOut < 0 )
  {
    fprintf(stderr, "irix_open_audio: invalid rate %d\n", rate);
    return 0;
  }

  /* Set up the output buffer */
  if ( (buffer_ptr = (char *)malloc(IRIX_BUFFER_CACHE)) == NULL )
  {
    perror("irix_open_audio: could not allocate cache buffer");
    return 0;
  }
  buffer_fillend = buffer_ptr;
  buffer_end = buffer_ptr + IRIX_BUFFER_CACHE;
  buffer_read = buffer_ptr;
  buffer_write = buffer_ptr;
  buffer_len = IRIX_BUFFER_CACHE;

  /* Ready to play! */
  irix_cfg.currentFrameNumber = 0;
  playing = 1;

  if (IRIX_DEBUG & IRIX_DEBUG_OPEN_AUDIO)
  {
    fprintf(stderr, "irix_open_audio: returning\n");
  }
  return 1;
}

/*****************************************************************************/

void
irix_write_audio(void *ptr, gint length)
{
  int length1, length2;

  if (IRIX_DEBUG & IRIX_DEBUG_WRITE_AUDIO)
  {
    fprintf(stderr, "irix_write_audio: begin\n");
  }

  if ( (buffer_read + length) > buffer_end )
  {
    length1 = (int) (buffer_end - buffer_read);
    length2 = length - length1;

    if (IRIX_DEBUG & IRIX_DEBUG_WRITE_AUDIO)
    {
      fprintf(stderr, "irix_write_audio: '%d' bytes from '%d' to '%d'\n",
              length1, buffer_read - buffer_ptr,
              buffer_read - buffer_ptr + length1 );
      fprintf(stderr, "              and '%d' bytes from '%d' to '%d'\n",
              length2, 0, length2);
      fprintf(stderr, "              (write='%d', fillend='%d')\n",
              buffer_write - buffer_ptr, buffer_fillend - buffer_ptr);
    }
    memcpy(buffer_read, ptr, length1);
    buffer_fillend = buffer_end;
    buffer_read = buffer_ptr;
    memcpy(buffer_read, ptr + length1, length2);
    buffer_read += length2;
  }
  else
  {
    if (IRIX_DEBUG & IRIX_DEBUG_WRITE_AUDIO)
    {
      fprintf(stderr, "irix_write_audio: '%d' bytes from '%d' to '%d'\n",
              length, buffer_read - buffer_ptr,
              buffer_read - buffer_ptr + length );
      fprintf(stderr, "              (write='%d', fillend='%d')\n",
              buffer_write - buffer_ptr, buffer_fillend - buffer_ptr);
    }
    memcpy(buffer_read, ptr, length);
    if ( buffer_read >= buffer_write )
    {
      buffer_fillend += length;
    }
    buffer_read += length;
  }

  if (IRIX_DEBUG & IRIX_DEBUG_WRITE_AUDIO)
  {
    
    fprintf(stderr, "irix_write_audio: returning\n");
  }
  return;
}

/*****************************************************************************/

void
irix_close_audio(void)
{
  if (IRIX_DEBUG & IRIX_DEBUG_CLOSE_AUDIO)
  {
    fprintf(stderr, "irix_close_audio: begin\n");
  }

  free(buffer_ptr);

  if ( alClosePort(irix_cfg.port) < 0 )
  {
    fprintf(stderr, "irix_close_audio: alClosePort failed: %s\n", alGetErrorString(oserror()));
  }

  irix_cfg.port = NULL;

  if ( alFreeConfig(irix_cfg.myALconfig) < 0 )
  {
    fprintf(stderr, "irix_close_audio: alFreeConfig failed: %s\n", alGetErrorString(oserror()));
  }

  if (IRIX_DEBUG & IRIX_DEBUG_CLOSE_AUDIO)
  {
    fprintf(stderr, "irix_close_audio: returning\n");
  }
  return;
}

/*****************************************************************************/

void
irix_flush(gint ftime)
{
  int df;

  if (IRIX_DEBUG & IRIX_DEBUG_FLUSH)
  {
    fprintf(stderr, "irix_flush: begin. ftime = '%d'\n", ftime);
  }

  /* Discard all queued frames */
  buffer_read    = buffer_ptr;
  buffer_write   = buffer_ptr;
  buffer_fillend = buffer_ptr;

  df = alDiscardFrames(irix_cfg.port, IRIX_AUDIO_CACHE);
  if (IRIX_DEBUG & IRIX_DEBUG_PAUSE)
  {
    fprintf(stderr, "irix_flush: discarded %d frames\n", df);
  }
  
  /* Seek to the new playing time */
  if (IRIX_DEBUG & IRIX_DEBUG_FLUSH)
  {
    fprintf(stderr, "irix_flush: ftime = %d, rate = %d, rate/1000 = %lf\n",
            ftime, irix_cfg.rate, (double)irix_cfg.rate / 1000);
    fprintf(stderr, "irix_flush: ftime*rate/1000 = %lf\n",
            (ftime * ((double)irix_cfg.rate / 1000)));
  }
  irix_cfg.currentFrameNumber = (gint)
    (ftime * ((double)irix_cfg.rate / 1000));
  
  if (IRIX_DEBUG & IRIX_DEBUG_FLUSH)
  {
    fprintf(stderr, "irix_flush: currentFrameNumber: %d\n", irix_cfg.currentFrameNumber);
  }


  if (IRIX_DEBUG & IRIX_DEBUG_FLUSH)
  {
    fprintf(stderr, "irix_flush: returning\n");
  }
}   

/*****************************************************************************/

void
irix_pause(short p)
{
  int df;

  if (IRIX_DEBUG & IRIX_DEBUG_PAUSE)
  {
    fprintf(stderr, "irix_pause: begin\n");
  }
  if (p)
  {
    paused = TRUE;
    df = alDiscardFrames(irix_cfg.port, IRIX_AUDIO_CACHE);
    if (IRIX_DEBUG & IRIX_DEBUG_PAUSE)
    {
      fprintf(stderr, "irix_pause: discarded %d frames\n", df);
    }
  }
  else
    paused = FALSE;
  if (IRIX_DEBUG & IRIX_DEBUG_PAUSE)
  {
    fprintf(stderr, "irix_pause: returning\n");
  }
}


/*****************************************************************************/
/*
 * This is supposed to allow reclaim of unplayed music;
 * Requires mucking about with the buffer
 */
gint
irix_buffer_free(void)
{
  gint bfree;
  int  gf;

  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
  {
    fprintf(stderr, "irix_buffer_free: begin\n");
  }

  if(paused)
  {
    if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
    {
      fprintf(stderr, "irix_buffer_free: returning 0\n");
    }
    return 0;
  }


  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
  {
    fprintf(stderr, "irix_buffer_free: read='%d', write='%d', fillend='%d'\n",
            buffer_read - buffer_ptr, buffer_write - buffer_ptr,
            buffer_fillend - buffer_ptr);
  }
  
  bfree = IRIX_BUFFER_CACHE - (buffer_fillend - buffer_write);
  if ( buffer_read <= buffer_write )
  {
    if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
    {
      fprintf(stderr, "irix_buffer_free: bfree was '%d' but will set it to '%d'\n", bfree, bfree-(buffer_read-buffer_ptr));
    }
    bfree -= (buffer_read - buffer_ptr);
  }

  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
  {
    fprintf(stderr, "irix_buffer_free: bfree = '%d'\n", bfree);
  }

  if ( buffer_fillend > buffer_write )
  {
    gf = alGetFillable(irix_cfg.port);
    if ( (irix_cfg.maxFillable - gf) > IRIX_AUDIO_CACHE )
    {
      if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
      {
        fprintf(stderr, "irix_buffer_free: no output_audio because\n");
        fprintf(stderr, "     maxFillable (%d) - gf (%d) = %d > MAX_PLAY (%d)\n",
                irix_cfg.maxFillable, gf, irix_cfg.maxFillable - gf, IRIX_AUDIO_CACHE);
      }
    }
    else
    {
      irix_output_audio();
    }
  }
  else
  {
    if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
    {
      fprintf(stderr, "irix_buffer_free: no output_audio because fillend = '%d'\n", buffer_fillend - buffer_ptr);
    }
  }
  
  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_FREE)
  {
    fprintf(stderr, "irix_buffer_free: returning bfree='%d'\n", bfree);
  }
  return bfree;
}

/*****************************************************************************/

gint
irix_buffer_playing(void)
{
  int gf;

  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_PLAYING)
  {
    fprintf(stderr, "irix_buffer_playing: begin\n");
  }

  if (irix_cfg.port == NULL)
  {
    playing = FALSE;
  }
  else
  {
    gf = alGetFillable(irix_cfg.port);
    if ( irix_cfg.maxFillable > gf )
    {
      if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_PLAYING)
      {
        fprintf(stderr, "     maxFillable=%d, gf=%d\n",
                irix_cfg.maxFillable, gf);
      }
      playing = TRUE;
    }
    else
    {
      playing = FALSE;
    }
  }


  if (IRIX_DEBUG & IRIX_DEBUG_BUFFER_PLAYING)
  {
    fprintf(stderr, "irix_buffer_playing: returning %d\n", playing);
  }
  return playing;
}

/*****************************************************************************/

gint
irix_output_time(void)
{
  static gint otime = 0;

  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_TIME)
  {
    fprintf(stderr, "irix_output_time: begin\n");
  }

  if (!playing)
    return 0;

  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_TIME)
  {
    fprintf(stderr, "irix_output_time: currentFN: %d\n",
            irix_cfg.currentFrameNumber);
  }

  if (!paused)
  {
    otime = (gint) ((double)irix_cfg.currentFrameNumber
                    * 1000 / irix_cfg.rate);
  }
  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_TIME)
  {
    fprintf(stderr, "irix_output_time: returning %d\n", otime);
  }
  return otime;
}

/*****************************************************************************/

gint
irix_written_time(void)
{
  gint wtime;

  if (IRIX_DEBUG & IRIX_DEBUG_WRITTEN_TIME)
  {
    fprintf(stderr, "irix_written_time: begin\n");
  }

  if (!playing)
    return 0;

  wtime = (gint) ((double)(irix_cfg.currentFrameNumber + 2 * IRIX_AUDIO_CACHE)
                    * 1000 / irix_cfg.rate);
  if (IRIX_DEBUG & IRIX_DEBUG_WRITTEN_TIME)
  {
    fprintf(stderr, "irix_written_time: returning %d\n", wtime);
  }
  return wtime;
}

/*****************************************************************************/

void
irix_output_audio(void)
{
  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_AUDIO)
  {
    fprintf(stderr, "irix_output_audio: begin\n");
  }

  if (paused == TRUE)
  {
    if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_AUDIO)
    {
      fprintf(stderr, "irix_output_audio: paused\n");
    }
    return;
  }

  if ( (buffer_fillend - buffer_write) < IRIX_AUDIO_CACHE)
  {
    if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_AUDIO)
    {
      fprintf(stderr, "irix_output_audio: returning because\n");
      fprintf(stderr, "                   buffer_fillend - buffer_write < IRIX_AUDIO_CACHE\n");
      fprintf(stderr, "                   (%d - %d < %d)\n",
              buffer_fillend - buffer_ptr,
              buffer_write - buffer_ptr,
              IRIX_AUDIO_CACHE);
    }
    return;
  }

  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_AUDIO)
  {
    fprintf(stderr,
            "irix_output_audio: read='%d', write='%d' to '%d', fillend='%d'\n",
            buffer_read - buffer_ptr,
            buffer_write - buffer_ptr,
            buffer_write - buffer_ptr + IRIX_AUDIO_CACHE,
            buffer_fillend-buffer_ptr);
  }

  alWriteFrames(irix_cfg.port, buffer_write,
                IRIX_AUDIO_CACHE / irix_cfg.combinedFrameSize);

  irix_cfg.currentFrameNumber += IRIX_AUDIO_CACHE/irix_cfg.combinedFrameSize;

  buffer_write += IRIX_AUDIO_CACHE;
  if ( buffer_write >= buffer_end )
  {
    buffer_write = buffer_ptr;
    buffer_fillend = buffer_read;
  }

  if (IRIX_DEBUG & IRIX_DEBUG_OUTPUT_AUDIO)
  {
    fprintf(stderr, "irix_output_audio: returning\n");
  }
  return;
}

/*****************************************************************************/

