/*
 * $Id: appinfo.h,v 1.5 2001/12/24 12:13:03 tal197 Exp $
 */

#ifndef _APPINFO_H
#define _APPINFO_H

/* Name of the XML file where the info is stored */
#define APPINFO_FILENAME		"AppInfo.xml"

/* External interface */
XMLwrapper *appinfo_get(guchar *app_dir, DirItem *item);
void appinfo_unref(XMLwrapper *info);
xmlNode *appinfo_get_section(XMLwrapper *ai, guchar *name);

#endif   /* _APPINFO_H */
