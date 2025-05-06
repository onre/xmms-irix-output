/*
 * about.c
 *
 * Dialog box for when user clicks on 'about' button
 *
 */

#include "IRIX.h"

static GtkWidget *dialog,*button,*label;

void
about_close_cb(GtkWidget *w,gpointer data)
{
    gtk_widget_destroy(dialog);
}

void
irix_about(void)
{
    dialog=gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog),"About IRIX Audio Driver 0.6");
    gtk_container_border_width(GTK_CONTAINER(dialog),5);
    label=gtk_label_new("XMMS IRIX Audio Driver 0.6\n\n \
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,\n\
USA.\n\n\
Written and maintained by Manuel Panea <mpd@rzg.mpg.de>.\n\
\n\
Latest information can be found at:\n\
http://www.rzg.mpg.de/~mpd/xmms");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,TRUE,TRUE,0);
    gtk_widget_show(label);

    button=gtk_button_new_with_label(" Close ");
    gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(about_close_cb),NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),button,FALSE,FALSE,0);
    gtk_widget_show (button);
    gtk_widget_show(dialog);
    gtk_widget_grab_focus(button);
}
