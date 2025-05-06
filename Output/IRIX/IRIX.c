/* IRIX.c; data to be exported for the plugin to be used by XMMS */

#include "IRIX.h"

static OutputPlugin op =
{
    NULL,
    NULL,
    "IRIX audio plugin v0.6",
    irix_init,
    irix_about,
    irix_configure,
    irix_get_volume,
    irix_set_volume,
    irix_open_audio,
    irix_write_audio,
    irix_close_audio,
    irix_flush,
    irix_pause,
    irix_buffer_free,
    irix_buffer_playing,
    irix_output_time,
    irix_written_time,
};


OutputPlugin *
get_oplugin_info(void)
{
    return &op;
}
