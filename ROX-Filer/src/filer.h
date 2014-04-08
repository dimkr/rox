/*
 * $Id: filer.h,v 1.73 2002/01/28 18:03:21 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _FILER_H
#define _FILER_H

#include <gtk/gtk.h>

enum {
	RESIZE_STYLE = 0,
	RESIZE_ALWAYS = 1,
	RESIZE_NEVER = 2,
};

typedef enum
{
	OPEN_SHIFT		= 0x01,	/* Do ShiftOpen */
	OPEN_SAME_WINDOW	= 0x02, /* Directories open in same window */
	OPEN_CLOSE_WINDOW	= 0x04, /* Opening files closes the window */
	OPEN_FROM_MINI		= 0x08,	/* Non-dir => close minibuffer */
} OpenFlags;

typedef enum
{
	FILER_NEEDS_RESCAN	= 0x01, /* Call may_rescan after scanning */
	FILER_UPDATING		= 0x02, /* (scanning) items may already exist */
	FILER_CREATE_THUMBS	= 0x04, /* Create thumbs when scan ends */
} FilerFlags;

typedef void (*TargetFunc)(FilerWindow *filer_window, int item, gpointer data);

struct _FilerWindow
{
	GtkWidget	*window;
	gboolean	scanning;	/* State of the 'scanning' indicator */
	guchar		*path;		/* pathname */
	Collection	*collection;
	gboolean	temp_item_selected;
	gboolean	show_hidden;
	FilerFlags	flags;
	int 		(*sort_fn)(const void *a, const void *b);

	DetailsType	details_type;
	DisplayStyle	display_style;

	Directory	*directory;

	gboolean	had_cursor;	/* (before changing directory) */
	char		*auto_select;	/* If it we find while scanning */

	GtkWidget	*message;	/* The 'Running as ...' message */

	GtkWidget	*minibuffer_area;	/* The hbox to show/hide */
	GtkWidget	*minibuffer_label;	/* The operation name */
	GtkWidget	*minibuffer;		/* The text entry */
	int		mini_cursor_base;
	MiniType	mini_type;

	/* TRUE if hidden files are shown because the minibuffer leafname
	 * starts with a dot.
	 */
	gboolean 	temp_show_hidden;

	TargetFunc	target_cb;
	gpointer	target_data;

	GtkWidget	*toolbar_frame;
	GtkWidget	*toolbar_text;
	GtkWidget	*scrollbar;

	gint		open_timeout;	/* Will resize and show window... */

	GtkStateType	selection_state;	/* for drawing selection */
	
	gboolean	show_thumbs;
	GList		*thumb_queue;		/* paths to thumbnail */
	GtkWidget	*thumb_bar, *thumb_progress;
	int		max_thumbs;		/* total for this batch */
};

extern FilerWindow 	*window_with_focus;
extern GList		*all_filer_windows;
extern GHashTable	*child_to_filer;
extern gboolean 	o_unique_filer_windows;

/* Prototypes */
void filer_init(void);
FilerWindow *filer_opendir(char *path, FilerWindow *src_win);
void filer_update_dir(FilerWindow *filer_window, gboolean warning);
void filer_update_all(void);
int selected_item_number(Collection *collection);
DirItem *selected_item(Collection *collection);
void change_to_parent(FilerWindow *filer_window);
void full_refresh(void);
void filer_openitem(FilerWindow *filer_window, int item_number,
		OpenFlags flags);
void filer_check_mounted(char *path);
void filer_close_recursive(char *path);
void filer_change_to(FilerWindow *filer_window, char *path, char *from);
gboolean filer_exists(FilerWindow *filer_window);
void filer_open_parent(FilerWindow *filer_window);
void filer_detach_rescan(FilerWindow *filer_window);
void filer_target_mode(FilerWindow	*filer_window,
			TargetFunc	fn,
			gpointer	data,
			char		*reason);
void filer_window_autosize(FilerWindow *filer_window, gboolean allow_shrink);
GList *filer_selected_items(FilerWindow *filer_window);
void filer_create_thumb(FilerWindow *filer_window, gchar *pathname);
void filer_cancel_thumbnails(FilerWindow *filer_window);
void filer_set_title(FilerWindow *filer_window);
void filer_create_thumbs(FilerWindow *filer_window);

#endif /* _FILER_H */
