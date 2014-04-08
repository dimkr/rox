/*
 * $Id: appmenu.h,v 1.5 2001/12/10 22:42:02 tal197 Exp $
 *
 * Diego Zamboni <zamboni@cerias.purdue.edu>
 */

#ifndef _APPMENU_H
#define _APPMENU_H

#include <gtk/gtk.h>

/* External interface */
void appmenu_add(guchar *app_dir, DirItem *item, GtkWidget *menu);
void appmenu_remove(void);

#endif   /* _APPMENU_H */
