#include "IRIX.h"
#include "libxmms/configfile.h"

IrixConfig irix_cfg;

void
irix_init(void)
{
  ConfigFile *cfgfile;
  gchar *filename;
  ALparamInfo pinfo;

  if (IRIX_DEBUG & IRIX_DEBUG_INIT)
  {
    fprintf(stderr, "irix_init: begin\n");
  }

  memset(&irix_cfg, 0, sizeof(IrixConfig));

  irix_cfg.port = NULL;

/*   if ( alGetParamInfo(irix_cfg.resource, AL_GAIN, &pinfo) < 0 ) */
  if ( alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &pinfo) < 0 )
  {
    fprintf(stderr, "irix_init: alGetParamInfo failed: %s\n", alGetErrorString(oserror()));
  }
  irix_cfg.min_volume = alFixedToDouble(pinfo.min.ll);
  irix_cfg.max_volume = alFixedToDouble(pinfo.max.ll);
  irix_cfg.volume_ratio = (irix_cfg.max_volume - irix_cfg.min_volume) / 100;

  
  filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
  if((cfgfile = xmms_cfg_open_file(filename)))
  {
    xmms_cfg_read_string(cfgfile, "IRIX", "audio_device", 
                         &irix_cfg.audio_device);
    xmms_cfg_read_int(cfgfile, "IRIX", "buffer_size", &irix_cfg.buffer_size);
    xmms_cfg_read_int(cfgfile, "IRIX", "prebuffer", &irix_cfg.prebuffer);
    /*
    set_chan_mode(cfgfile, AUDIO_SPEAKER,   "speaker",   0);
    set_chan_mode(cfgfile, AUDIO_LINE_OUT,  "line_out",  0);
    set_chan_mode(cfgfile, AUDIO_HEADPHONE, "headphone", 0);
    */
    xmms_cfg_free(cfgfile);
  }

  if (IRIX_DEBUG & IRIX_DEBUG_INIT)
  {
    fprintf(stderr, "irix_init: returning\n");
  }
}

