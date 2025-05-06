/*  xmms - graphically mp3 player..
 *  Copyright (C) 1998-1999  Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef IRIX_H
#define IRIX_H

#define IRIX_DEBUG_INIT           0x0001
#define IRIX_DEBUG_ABOUT          0x0002
#define IRIX_DEBUG_CONFIGURE      0x0004
#define IRIX_DEBUG_GET_VOLUME     0x0008
#define IRIX_DEBUG_SET_VOLUME     0x0010
#define IRIX_DEBUG_OPEN_AUDIO     0x0020
#define IRIX_DEBUG_WRITE_AUDIO    0x0040
#define IRIX_DEBUG_CLOSE_AUDIO    0x0080
#define IRIX_DEBUG_FLUSH          0x0100
#define IRIX_DEBUG_PAUSE          0x0200
#define IRIX_DEBUG_BUFFER_FREE    0x0400
#define IRIX_DEBUG_BUFFER_PLAYING 0x0800
#define IRIX_DEBUG_OUTPUT_TIME    0x1000
#define IRIX_DEBUG_WRITTEN_TIME   0x2000
#define IRIX_DEBUG_OUTPUT_AUDIO   0x4000
#define IRIX_DEBUG_DUMMY          0x0000

#define IRIX_DEBUG (IRIX_DEBUG_INIT |\
                    IRIX_DEBUG_CONFIGURE |\
                    IRIX_DEBUG_OPEN_AUDIO |\
                    IRIX_DEBUG_CLOSE_AUDIO |\
                    IRIX_DEBUG_FLUSH |\
                    IRIX_DEBUG_PAUSE |\
                    IRIX_DEBUG_BUFFER_PLAYING |\
                    IRIX_DEBUG_WRITTEN_TIME |\
                    IRIX_DEBUG_DUMMY)

#define IRIX_DEBUG 0

#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <dmedia/audio.h>

#include "xmms/plugin.h"
#include "libxmms/configfile.h"

extern OutputPlugin op;

typedef struct
{
  gchar    *audio_device;
  gint      channels;
  gint      buffer_size;
  gint      prebuffer;
  gint      rate;
  double    min_volume, max_volume, volume_ratio;
  gint      combinedFrameSize;
  stamp_t   currentFrameNumber;
  ALport    port;
  ALconfig  myALconfig;
  int       resource;
  int       maxFillable;
} IrixConfig;

extern IrixConfig irix_cfg;


void irix_init(void);
void irix_about(void);
void irix_configure(void);

void irix_get_volume(int *l,int *r);
void irix_set_volume(int l,int r);

int irix_open_audio(AFormat fmt,int rate,int nch);
void irix_write_audio(void *ptr,int length);
void irix_close_audio(void);

void irix_flush(int time);
void irix_pause(short p);
int irix_buffer_free(void);
int irix_buffer_playing(void);

int irix_output_time(void);
int irix_written_time(void);

void irix_output_audio(void);

#endif /* IRIX_H */
