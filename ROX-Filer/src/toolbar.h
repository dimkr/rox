/*
 * $Id: toolbar.h,v 1.4 2001/05/23 11:50:05 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _TOOLBAR_H
#define _TOOLBAR_H

#include <gtk/gtk.h>

/* The values correspond to the menu indexes in the option widget */
typedef enum {
	TOOLBAR_NONE 	= 0,
	TOOLBAR_NORMAL 	= 1,
	TOOLBAR_LARGE 	= 2,
} ToolbarType;

extern ToolbarType o_toolbar;
extern gint o_toolbar_info;

/* Prototypes */
void toolbar_init(void);
GtkWidget *toolbar_new(FilerWindow *filer_window);
GtkWidget *toolbar_tool_option(int i);
void toolbar_update_info(FilerWindow *filer_window);


#endif /* _TOOLBAR_H */
