/*
 * $Id: remote.h,v 1.4 2001/10/14 12:12:24 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _REMOTE_H
#define _REMOTE_H

gboolean remote_init(xmlDocPtr rpc, gboolean new_copy);
xmlDocPtr run_soap(xmlDocPtr soap);

#endif /* _REMOTE_H */
