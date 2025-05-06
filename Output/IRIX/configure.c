#include "IRIX.h"
#include <strings.h>
#include <math.h>

static GtkWidget *configure_win, *vbox, *notebook;

/* Audio device tab widgets */
static GtkWidget *audio_vbox;

static GtkWidget *audio_device_frame;
static GtkWidget *audio_device_box;
static GtkWidget *audio_device_option;

static GtkWidget *port_frame, *port_vbox, *port_table;
static GtkWidget *port_chk_speaker, *port_chk_headphones, *port_chk_lineout;

/* buffer size tab widgets */
static GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
static GtkWidget *buffer_size_box, *buffer_size_label, *buffer_size_spin;
static GtkObject *buffer_size_adj;
static GtkWidget *buffer_pre_box, *buffer_pre_label, *buffer_pre_spin;
static GtkObject *buffer_pre_adj;

static GtkWidget *bbox, *ok, *cancel, *apply;

static gchar *audio_device;

/* configure_write(); Write configuration to file */
static void
configure_write ()
{
  ConfigFile *cfgfile;
  gchar *filename;

  filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
  cfgfile = xmms_cfg_open_file(filename);
  if(!cfgfile)
    cfgfile = xmms_cfg_new();
	
  xmms_cfg_write_string(cfgfile, "IRIX", "audio_device",
                        irix_cfg.audio_device);
  xmms_cfg_write_int(cfgfile, "IRIX", "buffer_size", irix_cfg.buffer_size);
  xmms_cfg_write_int(cfgfile, "IRIX", "prebuffer", irix_cfg.prebuffer);
  /*
  xmms_cfg_write_int(cfgfile, "IRIX", "speaker",
                     ( irix_cfg.channel_flags & AUDIO_SPEAKER ) != 0 );
  xmms_cfg_write_int(cfgfile, "IRIX", "line_out",
                     ( irix_cfg.channel_flags & AUDIO_LINE_OUT ) != 0 );
  xmms_cfg_write_int(cfgfile, "IRIX", "headphone",
                     ( irix_cfg.channel_flags & AUDIO_HEADPHONE ) != 0 );
  */
  xmms_cfg_write_file(cfgfile, filename);
  xmms_cfg_free(cfgfile);

  g_free(filename);
}

/* Handler for "APPLY" button */
static void
configure_win_apply_cb(GtkWidget *w, gpointer data)
{
  irix_cfg.audio_device = audio_device;
  irix_cfg.buffer_size = (gint)GTK_ADJUSTMENT(buffer_size_adj)->value;
  irix_cfg.prebuffer = (gint)GTK_ADJUSTMENT(buffer_pre_adj)->value;
  /*
  irix_cfg.channel_flags =
    (((GTK_TOGGLE_BUTTON (port_chk_lineout   )->active)?AUDIO_LINE_OUT :0) |
     ((GTK_TOGGLE_BUTTON (port_chk_speaker   )->active)?AUDIO_SPEAKER  :0) |
     ((GTK_TOGGLE_BUTTON (port_chk_headphones)->active)?AUDIO_HEADPHONE:0));
  */
  configure_write();

  /*
  irix_set_dev();
  */
}

/* Handler for "OK" button */
static void
configure_win_ok_cb(GtkWidget *w, gpointer data)
{
  irix_cfg.buffer_size = (gint)GTK_ADJUSTMENT(buffer_size_adj)->value;
  irix_cfg.prebuffer = (gint)GTK_ADJUSTMENT(buffer_pre_adj)->value;
  /*
  irix_cfg.channel_flags =
    (((GTK_TOGGLE_BUTTON (port_chk_lineout   )->active)?AUDIO_LINE_OUT :0) |
     ((GTK_TOGGLE_BUTTON (port_chk_speaker   )->active)?AUDIO_SPEAKER  :0) |
     ((GTK_TOGGLE_BUTTON (port_chk_headphones)->active)?AUDIO_HEADPHONE:0));
  */
  configure_write();

  /*
  irix_set_dev();
  */

  gtk_widget_destroy(configure_win);
}

static void
configure_win_audio_dev_cb (GtkWidget *widget, gchar *device)
{
  audio_device = device;
}

#ifdef 0
static gchar *
device_name (gchar *file_name)
{
  int fd;
  gchar *name = 0;
  gchar *ctl_name;

  /* Use the ctl device to prevent a lock on any device being
     used by another process; also for CD output */
  ctl_name = malloc(strlen(file_name) + 5);
  strcpy (ctl_name, file_name);
  strcat (ctl_name, "ctl");
  
  fd = open (ctl_name, O_RDONLY);
  free (ctl_name);
  if (fd != -1)
  {
    audio_device_t device;
    if (ioctl (fd, AUDIO_GETDEV, &device) >= 0)
    {
      name = g_strdup (device.name);
    }

    close (fd);
  }

  return name;
}
#endif

static gint
scan_devices (gchar *type, GtkWidget *option_menu, 
	      GtkSignalFunc sigfunc, gchar *default_name)
{
}


void
irix_configure(void)
{
  int default_index = -1;

  fprintf(stderr, "irix_configure: begin\n");

  configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(configure_win), "Configure IRIX driver");
  gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
  gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(configure_win), vbox);

  notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  /********************/
  /* Audio device tab */
  /********************/
  audio_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (audio_vbox), 5);

  audio_device_frame = gtk_frame_new ("Audio device:");
  gtk_box_pack_start (GTK_BOX (audio_vbox), audio_device_frame, 
                      FALSE, FALSE, 0);

  audio_device_box = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (audio_device_box), 5);
  gtk_container_add (GTK_CONTAINER (audio_device_frame), 
                     audio_device_box);

  audio_device_option = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (audio_device_box), audio_device_option, 
                      TRUE, TRUE, 0);
  default_index = scan_devices ("Audio devices:", audio_device_option, 
                                configure_win_audio_dev_cb, 
                                irix_cfg.audio_device);
  audio_device = irix_cfg.audio_device;
  if (default_index >= 0)
    gtk_option_menu_set_history (GTK_OPTION_MENU (audio_device_option), 
                                 default_index);

  gtk_widget_show (audio_device_option);
  gtk_widget_show (audio_device_box);
  gtk_widget_show (audio_device_frame);

  port_frame = gtk_frame_new("Output ports:");
  gtk_container_set_border_width(GTK_CONTAINER(port_frame), 5);
	
  gtk_box_pack_start (GTK_BOX (audio_vbox), port_frame, 
                      FALSE, FALSE, 0);

  port_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(port_frame), port_vbox);

  port_table = gtk_table_new(2, 1, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(port_table), 5);
  gtk_box_pack_start(GTK_BOX(port_vbox), port_table, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER(port_vbox), 5);

  port_chk_lineout    = gtk_check_button_new_with_label ( "Line out" );
  port_chk_headphones = gtk_check_button_new_with_label ( "Headphones" );
  port_chk_speaker    = gtk_check_button_new_with_label ( "Internal speaker" );
  gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_lineout, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_headphones, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(port_vbox), port_chk_speaker, FALSE, FALSE, 0);

  /*
   * Set buttons to correct state
   * First, get current state of buttons in case they've been changed by
   * eg audiocontrol
   */
  /*
  if ( irix_isopen() )
    irix_update_dev();
  */
  /*
  gtk_toggle_button_set_state ((GtkToggleButton*)port_chk_lineout,
                               (gboolean)(irix_cfg.channel_flags &
                                          AUDIO_LINE_OUT));
  gtk_toggle_button_set_state ((GtkToggleButton*)port_chk_headphones,
                               (gboolean)(irix_cfg.channel_flags &
                                          AUDIO_HEADPHONE));
  gtk_toggle_button_set_state ((GtkToggleButton*)port_chk_speaker,
                               (gboolean)(irix_cfg.channel_flags &
                                          AUDIO_SPEAKER));
  */
  gtk_widget_show(port_vbox);
  gtk_widget_show(port_table);
  gtk_widget_show(port_frame);

  gtk_widget_show(port_chk_lineout);
  gtk_widget_show(port_chk_headphones);
  gtk_widget_show(port_chk_speaker);

  gtk_widget_show (audio_vbox);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
                           audio_vbox, gtk_label_new("Audio"));

  /*****************/
  /* Buffering tab */
  /*****************/
  buffer_frame = gtk_frame_new("Buffering:");
  gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);
    
  buffer_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);
    
  buffer_table = gtk_table_new(2, 1, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
  gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);
    
  buffer_size_box = gtk_hbox_new(FALSE, 5);
  gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_size_box,
                            0, 1, 0, 1);
  buffer_size_label = gtk_label_new("Buffer size (msec):");
  gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_label,
                     FALSE, FALSE, 0);
  gtk_widget_show(buffer_size_label);
  buffer_size_adj = gtk_adjustment_new(irix_cfg.buffer_size,
                                       100, 10000, 100, 100, 100);
  buffer_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj),
                                         8, 0);
  gtk_widget_set_usize(buffer_size_spin, 60, -1);
  gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_spin,
                     FALSE, FALSE, 0);
  gtk_widget_show(buffer_size_spin);
  gtk_widget_show(buffer_size_box);
    
  buffer_pre_box = gtk_hbox_new(FALSE, 5);
  gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_pre_box,
                            1, 2, 0, 1);
  buffer_pre_label = gtk_label_new("Pre-buffer (percent):");
  gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_label,
                     FALSE, FALSE, 0);
  gtk_widget_show(buffer_pre_label);
  buffer_pre_adj = gtk_adjustment_new(irix_cfg.prebuffer, 0, 90, 1, 1, 1);
  buffer_pre_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_pre_adj), 1, 0);
  gtk_widget_set_usize(buffer_pre_spin, 60, -1);
  gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_spin,
                     FALSE, FALSE, 0);
  gtk_widget_show(buffer_pre_spin);
  gtk_widget_show(buffer_pre_box);

    
  gtk_widget_show(buffer_table);
  gtk_widget_show(buffer_vbox);
  gtk_widget_show(buffer_frame);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buffer_frame,
                           gtk_label_new("Buffering"));
	
  /* Display notebook widget */
  gtk_widget_show(notebook);
    
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
    
  /* display main buttons (ok/cancel) */
  ok = gtk_button_new_with_label("Ok");
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
                     GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);
  GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
  gtk_widget_show(ok);
  gtk_widget_grab_default(ok);

  apply = gtk_button_new_with_label("Apply");
  gtk_signal_connect_object(GTK_OBJECT(apply), "clicked",
                            GTK_SIGNAL_FUNC(configure_win_apply_cb), NULL);
  GTK_WIDGET_SET_FLAGS(apply, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), apply, TRUE, TRUE, 0);
  gtk_widget_show(apply);
	
  cancel = gtk_button_new_with_label("Cancel");
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(configure_win));
  GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
  gtk_widget_show(cancel);
	
  gtk_widget_show(bbox);
  gtk_widget_show(vbox);
  gtk_widget_show(configure_win);

  fprintf(stderr, "irix_configure: returning\n");
}

