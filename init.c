#include "IRIX.h"

#include <errno.h>
#include <xmms/configfile.h>

IrixConfig irix_cfg;

void
irix_init(void)
{
  ALparamInfo pinfo;

  if (IRIX_DEBUG & IRIX_DEBUG_INIT)
  {
    fprintf(stderr, "irix_init: begin\n");
  }

  memset(&irix_cfg, 0, sizeof(IrixConfig));

  irix_cfg.port = NULL;

  if ( alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &pinfo) < 0 )
  {
    fprintf(stderr, "irix_init: alGetParamInfo failed: %s\n", alGetErrorString(oserror()));
  }
  irix_cfg.min_volume = alFixedToDouble(pinfo.min.ll);
  irix_cfg.max_volume = alFixedToDouble(pinfo.max.ll);
  irix_cfg.volume_ratio = (irix_cfg.max_volume - irix_cfg.min_volume) / 100;
 
  if (IRIX_DEBUG & IRIX_DEBUG_INIT)
  {
    fprintf(stderr, "irix_init: returning\n");
  }
}

