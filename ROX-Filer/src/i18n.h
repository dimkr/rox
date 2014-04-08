/*
 * $Id: i18n.h,v 1.2 2000/07/28 21:11:07 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#include <gtk/gtk.h>

void i18n_init(void);
GtkItemFactoryEntry *translate_entries(GtkItemFactoryEntry *entries, gint n);
void free_translated_entries(GtkItemFactoryEntry *entries, gint n);
