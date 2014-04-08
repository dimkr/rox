/*
 * $Id: pinboard.h,v 1.15 2001/12/11 22:21:29 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _PINBOARD_H
#define _PINBOARD_H

extern Pinboard	*current_pinboard;

extern Icon *pinboard_drag_in_progress;

struct _Pinboard {
	guchar		*name;		/* Leaf name */
	GList		*icons;
};

void pinboard_init(void);
void pinboard_activate(guchar *name);
void pinboard_pin(guchar *path, guchar *name, int x, int y);
void pinboard_unpin(Icon *icon);
void pinboard_wink_item(Icon *icon, gboolean timeout);
void pinboard_clear(void);
void pinboard_icon_may_update(Icon *icon);
void pinboard_reshape_icon(Icon *icon);
void pinboard_save(void);
void pinboard_move_icons(void);

#endif /* _PINBOARD_H */
