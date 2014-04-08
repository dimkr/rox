/*
 * $Id: display.h,v 1.25 2001/12/26 20:33:21 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _DISPLAY_H
#define _DISPLAY_H

#define ROW_HEIGHT_LARGE 64

#include <gtk/gtk.h>
#include <sys/types.h>
#include <dirent.h>

typedef struct _ViewData ViewData;

struct _ViewData
{
#ifdef GTK2
	PangoLayout *layout;
	PangoLayout *details;
#endif
	int	name_width;
	int	name_height;
	int	details_width;
	int	details_height;
#ifndef GTK2
	int	split_pos;		/* 0 => No split */
	int	split_width, split_height;
	char	*details;
#endif

	MaskedPixmap *image;		/* Image; possibly thumbnail */
};

/* Prototypes */
void display_init();
void display_set_layout(FilerWindow  *filer_window,
			DisplayStyle style,
			DetailsType  details);
void display_set_hidden(FilerWindow *filer_window, gboolean hidden);
void display_set_thumbs(FilerWindow *filer_window, gboolean thumbs);
int sort_by_name(const void *item1, const void *item2);
int sort_by_type(const void *item1, const void *item2);
int sort_by_date(const void *item1, const void *item2);
int sort_by_size(const void *item1, const void *item2);
void display_set_sort_fn(FilerWindow *filer_window,
			int (*fn)(const void *a, const void *b));
void display_set_autoselect(FilerWindow *filer_window, guchar *leaf);
void shrink_grid(FilerWindow *filer_window);
void calc_size(FilerWindow *filer_window, CollectionItem *colitem,
		int *width, int *height);

void draw_large_icon(GtkWidget *widget,
		     GdkRectangle *area,
		     DirItem  *item,
		     MaskedPixmap *image,
		     gboolean selected);
gboolean display_is_truncated(FilerWindow *filer_window, int i);
void display_change_size(FilerWindow *filer_window, gboolean bigger);

ViewData *display_create_viewdata(FilerWindow *filer_window, DirItem *item);
void display_free_colitem(Collection *collection, CollectionItem *colitem);
void display_update_view(FilerWindow *filer_window,
			 DirItem *item,
			 ViewData *view);

#endif /* _DISPLAY_H */
