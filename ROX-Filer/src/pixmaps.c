/*
 * $Id: pixmaps.c,v 1.70 2002/02/02 14:31:32 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project
 * Copyright (C) 2002, the ROX-Filer team.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* pixmaps.c - code for handling pixmaps */

#include "config.h"
#define PIXMAPS_C

/* Remove pixmaps from the cache when they haven't been accessed for
 * this period of time (seconds).
 */

#define PIXMAP_PURGE_TIME 1200

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#ifdef THUMBS_USE_LIBPNG
# include <stdarg.h>
# include <png.h>
# ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
# endif
#endif

#include "global.h"

#include "fscache.h"
#include "support.h"
#include "gui_support.h"
#include "pixmaps.h"
#include "main.h"
#include "filer.h"
#include "dir.h"

GFSCache *pixmap_cache = NULL;

static const char * bad_xpm[] = {
"12 12 3 1",
" 	c #000000000000",
".	c #FFFF00000000",
"x	c #FFFFFFFFFFFF",
"            ",
" ..xxxxxx.. ",
" ...xxxx... ",
" x...xx...x ",
" xx......xx ",
" xxx....xxx ",
" xxx....xxx ",
" xx......xx ",
" x...xx...x ",
" ...xxxx... ",
" ..xxxxxx.. ",
"            "};

MaskedPixmap *im_error;
MaskedPixmap *im_unknown;
MaskedPixmap *im_symlink;
MaskedPixmap *im_dir;

MaskedPixmap *im_unmounted;
MaskedPixmap *im_mounted;
MaskedPixmap *im_multiple;
MaskedPixmap *im_appdir;

MaskedPixmap *im_help;
MaskedPixmap *im_dirs;

/* Static prototypes */

static void load_default_pixmaps(void);
static MaskedPixmap *load(char *pathname, gpointer data);
static void ref(MaskedPixmap *mp, gpointer data);
static void unref(MaskedPixmap *mp, gpointer data);
static int getref(MaskedPixmap *mp);
static gint purge(gpointer data);
static MaskedPixmap *image_from_file(char *path);
static MaskedPixmap *get_bad_image(void);
static GdkPixbuf *scale_pixbuf(GdkPixbuf *src, int max_w, int max_h);
static GdkPixbuf *scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h);
static void got_thumb_data(GdkPixbufLoader *loader,
				gint fd, GdkInputCondition cond);
static GdkPixbuf *get_thumbnail_for(char *path);


/****************************************************************
 *			EXTERNAL INTERFACE			*
 ****************************************************************/

void pixmaps_init(void)
{
#ifndef GTK2
	gdk_rgb_init();
	gtk_widget_push_visual(gdk_rgb_get_visual());
#endif
	gtk_widget_push_colormap(gdk_rgb_get_cmap());

	pixmap_cache = g_fscache_new((GFSLoadFunc) load,
					(GFSRefFunc) ref,
					(GFSRefFunc) unref,
					(GFSGetRefFunc) getref,
					NULL,	/* Update func */
					NULL);

	gtk_timeout_add(10000, purge, NULL);

	load_default_pixmaps();
}

/* 'name' is relative to app_dir. Always returns with a valid image. */
MaskedPixmap *load_pixmap(char *name)
{
	MaskedPixmap *retval;

	retval = image_from_file(make_path(app_dir, name)->str);
	if (!retval)
		retval = get_bad_image();
	return retval;
}

/* Load all the standard pixmaps */
static void load_default_pixmaps(void)
{
	im_error = load_pixmap("pixmaps/error.xpm");
	im_unknown = load_pixmap("pixmaps/unknown.xpm");
	im_symlink = load_pixmap("pixmaps/symlink.xpm");
	im_dir = load_pixmap("pixmaps/dir.xpm");

	im_unmounted = load_pixmap("pixmaps/mount.xpm");
	im_mounted = load_pixmap("pixmaps/mounted.xpm");
	im_multiple = load_pixmap("pixmaps/multiple.xpm");
	im_appdir = load_pixmap("pixmaps/application.xpm");

	im_help = load_pixmap("pixmaps/help.xpm");
	im_dirs = load_pixmap("pixmaps/dirs.xpm");
}

void pixmap_ref(MaskedPixmap *mp)
{
	ref(mp, NULL);
}

void pixmap_unref(MaskedPixmap *mp)
{
	unref(mp, NULL);
}

void pixmap_make_huge(MaskedPixmap *mp)
{
	GdkPixbuf	*hg;

	if (mp->huge_pixmap)
		return;

	g_return_if_fail(mp->huge_pixbuf != NULL);

	hg = scale_pixbuf_up(mp->huge_pixbuf,
				HUGE_WIDTH * 0.7,
				HUGE_HEIGHT * 0.7);

	if (hg)
	{
		gdk_pixbuf_render_pixmap_and_mask(hg,
				&mp->huge_pixmap,
				&mp->huge_mask,
				128);
		mp->huge_width = gdk_pixbuf_get_width(hg);
		mp->huge_height = gdk_pixbuf_get_height(hg);
		gdk_pixbuf_unref(hg);
	}

	if (!mp->huge_pixmap)
	{
		gdk_pixmap_ref(mp->pixmap);
		if (mp->mask)
			gdk_bitmap_ref(mp->mask);
		mp->huge_pixmap = mp->pixmap;
		mp->huge_mask = mp->mask;
		mp->huge_width = mp->width;
		mp->huge_height = mp->height;
	}
}

void pixmap_make_small(MaskedPixmap *mp)
{
	GdkPixbuf	*sm;

	if (mp->sm_pixmap)
		return;

	g_return_if_fail(mp->huge_pixbuf != NULL);
			
	sm = scale_pixbuf(mp->huge_pixbuf, SMALL_WIDTH, SMALL_HEIGHT);

	if (sm)
	{
		gdk_pixbuf_render_pixmap_and_mask(sm,
				&mp->sm_pixmap,
				&mp->sm_mask,
				128);
		mp->sm_width = gdk_pixbuf_get_width(sm);
		mp->sm_height = gdk_pixbuf_get_height(sm);
		gdk_pixbuf_unref(sm);
	}

	if (mp->sm_pixmap)
		return;

	gdk_pixmap_ref(mp->pixmap);
	if (mp->mask)
		gdk_bitmap_ref(mp->mask);
	mp->sm_pixmap = mp->pixmap;
	mp->sm_mask = mp->mask;
	mp->sm_width = mp->width;
	mp->sm_height = mp->height;
}

#ifdef GTK2
# define GET_LOADER(key) (g_object_get_data(G_OBJECT(loader), key))
# define SET_LOADER(key, data) (g_object_set_data(G_OBJECT(loader), key, data))
# define SET_LOADER_FULL(key, data, free) \
		(g_object_set_data_full(G_OBJECT(loader), key, data, free))
#else
# define GET_LOADER(key) (gtk_object_get_data(GTK_OBJECT(loader), key))
# define SET_LOADER(key, data) \
		(gtk_object_set_data(GTK_OBJECT(loader), key, data))
# define SET_LOADER_FULL(key, data, free) \
		(gtk_object_set_data_full(GTK_OBJECT(loader), key, data, free))
#endif

/* Load image 'path' in the background and insert into pixmap_cache.
 * Call callback(data, path) when done (path is NULL => error).
 * If the image is already uptodate, or being created already, calls the
 * callback right away.
 */
void pixmap_background_thumb(gchar *path, GFunc callback, gpointer data)
{
	GdkPixbufLoader *loader;
	int		fd, tag;
	gboolean	found;
	MaskedPixmap	*image;
	GdkPixbuf	*pixbuf;

	image = g_fscache_lookup_full(pixmap_cache, path,
					FSCACHE_LOOKUP_ONLY_NEW, &found);

	if (found)
	{
		/* Thumbnail is known, or being created */
		callback(data, NULL);
		return;
	}

	pixbuf = get_thumbnail_for(path);
	if (pixbuf)
	{
		MaskedPixmap *image;

		image = image_from_pixbuf(pixbuf);
		gdk_pixbuf_unref(pixbuf);
		g_fscache_insert(pixmap_cache, path, image, TRUE);
		callback(data, path);
		return;
	}

	/* Add an entry, set to NULL, so no-one else tries to load this
	 * image.
	 */
	g_fscache_insert(pixmap_cache, path, NULL, TRUE);

	fd = mc_open(path, O_RDONLY | O_NONBLOCK);
	if (fd == -1)
	{
		callback(data, NULL);
		return;
	}

	loader = gdk_pixbuf_loader_new();

	SET_LOADER_FULL("thumb-path", g_strdup(path), g_free);
	SET_LOADER("thumb-callback", callback);
	SET_LOADER("thumb-callback-data", data);
	
	tag = gdk_input_add(fd, GDK_INPUT_READ,
			    (GdkInputFunction) got_thumb_data, loader);

	SET_LOADER("thumb-input-tag", GINT_TO_POINTER(tag));
}

/****************************************************************
 *			INTERNAL FUNCTIONS			*
 ****************************************************************/

#ifdef THUMBS_USE_LIBPNG
/* Do this in a separate function to avoid problems with longjmp and
 * non-volatile variables.
 */
static gboolean png_save(png_structp png_ptr, png_infop info_ptr,
			 FILE *file, GdkPixbuf *pixbuf, GPtrArray *textarr)
{
	gboolean	has_alpha;
	int		width;
	int		height;
	int		bit_depth;
	int		rowstride;
	guchar		*pixels;

	/* An error during saving will cause a jump back to here */
	if (setjmp(png_jmpbuf(png_ptr)))
		return FALSE;

	png_init_io(png_ptr, file);

	pixels = gdk_pixbuf_get_pixels(pixbuf);
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
	bit_depth = gdk_pixbuf_get_bits_per_sample(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
			PNG_COLOR_TYPE_RGB_ALPHA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	
	
	/* Write image info */
	png_write_info(png_ptr, info_ptr);

	/* Write the image data */
	if (has_alpha)
	{
		int	row;
		
		for (row = 0; row < height; row++, pixels += rowstride)
			png_write_row(png_ptr, (png_bytep) pixels);
	}
	else	/* convert RGB to RGBA */
	{
		guchar	*rowbuf;
		int	col;
		int	row;

		rowbuf = g_malloc(width * 4);
		for (row = 0; row < height; row++, pixels += rowstride)
		{
			guchar	*src = pixels;
			guchar	*dst = rowbuf;
			for (col = 0; col < width; col++)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;
			}
			png_write_row(png_ptr, (png_bytep) rowbuf);
		}
		g_free(rowbuf);
	}
	
	png_write_end(png_ptr, info_ptr);

	return TRUE;
}

/* Saves pixbuf to a PNG file (non-interlaced RGBA).
 * The variable arguments are key-text pairs and must be NULL-terminated.
 * NOTE: unlike with gdk_pixbuf_save, keys do NOT have to be prefixed
 *       with "tEXt::".
 * Returns TRUE if the image was saved, FALSE otherwise.
 */
static gboolean pixbuf_save_png(GdkPixbuf *pixbuf, char *filename, ...)
{
	FILE		*file = NULL;
	png_structp	png_ptr = NULL;
	png_infop	info_ptr = NULL;
	GPtrArray	*textarr = NULL;
	gboolean	retval = FALSE;
	va_list		ap;
	char		*string;

	g_return_val_if_fail(pixbuf != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(*filename != '\0', FALSE);

	file = fopen(filename, "wb");
	if (!file)
		goto err;
	
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					  NULL, NULL, NULL);
	if (!png_ptr)
		goto err;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto err;

	/* Get optional key-text pairs. */
	textarr = g_ptr_array_new();
	va_start(ap, filename);
	string = va_arg(ap, char *);
	while (string != NULL)
	{
		png_text	*pngtext;

		pngtext = g_new(png_text, 1);
		pngtext->key = string;
		pngtext->text = va_arg(ap, char *);
		pngtext->compression = PNG_TEXT_COMPRESSION_NONE;
		/* (lang and translated_keyword not needed for COMP_NONE) */
		png_set_text(png_ptr, info_ptr, pngtext, 1);
		/* png_set_text() does NOT copy data, so we can't free
		 * it now -- store the pointers, they'll be freed later.
		 */
		g_ptr_array_add(textarr, pngtext);
			
		string = va_arg(ap, char *);
	}
	va_end(ap);

	retval = png_save(png_ptr, info_ptr, file, pixbuf, textarr);
err:
	if (file)
		fclose(file);
	if (png_ptr)
	{
		if (info_ptr)
			png_destroy_write_struct(&png_ptr, &info_ptr);
		else
			png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	}
	if (textarr)
		g_ptr_array_free(textarr, TRUE);

	if (!retval)
		g_warning("Error saving thumbnail '%s'", filename);

	return retval;
}

static void destroy_key_value_fn(gpointer key, gpointer value, gpointer data)
{
	g_free(key);
	g_free(value);
}

/* Frees the pixel array of a pixbuf and the hash table associated with it. */
static void pixbuf_destroy_fn(guchar *pixels, gpointer data)
{
	if (data != NULL)
	{
		g_hash_table_foreach((GHashTable *) data,
				destroy_key_value_fn, NULL);
		g_hash_table_destroy((GHashTable *) data);
	}
	
	g_free(pixels);
}

static GdkPixbuf *png_load(png_structp png_ptr, png_infop info_ptr,
			   FILE *file, GHashTable *text_hash)
{
	png_uint_32	width, height, rowbytes, row;
	int		bit_depth, color_type, interlace_type;
	guchar		*pixels;
	guchar		*rowp;

	/* An error during loading will cause a jump back to here */
	if (setjmp(png_jmpbuf(png_ptr)))
		return FALSE;

	png_init_io(png_ptr, file);
	
	/* We've already read 8 bytes from the file */
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);

	if (interlace_type != PNG_INTERLACE_NONE || width > 256 || height > 256)
		return FALSE;
	
	/* Change paletted images to RGB */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	/* Transform grayscale images of less than 8 to 8 bits */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	/* Add alpha channel if there's transparency info in the file */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	/* 16-bit RGB to 8-bit RGB */
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	
	/* Convert RGB to RGBA with an opacity value of 255 */
	if (bit_depth == 8 && color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);

	/* Expand bit depths of 1, 2 and 4 to 8 */
	if (bit_depth < 8)
		png_set_packing(png_ptr);
	
	/* Grayscale to RGB */
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	
	png_read_update_info(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	/* Read rows */
	pixels = g_malloc(rowbytes * height);
	for (row = 0, rowp = pixels; row < height; row++, rowp += rowbytes)
		png_read_row(png_ptr, rowp, NULL);
	
	png_read_end(png_ptr, NULL);

	/* Get key-text pairs and store them for later use */	
	if (text_hash)
	{
		png_textp	text_ptr;
		int		num_text;

		png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
		while (--num_text >= 0)
		{
			g_hash_table_insert(text_hash,
					g_strdup(text_ptr[num_text].key),
					g_strdup(text_ptr[num_text].text));
		}
	}

	return gdk_pixbuf_new_from_data(pixels,
			GDK_COLORSPACE_RGB, TRUE, 8,
			(int) width, (int) height, (int) rowbytes,
			pixbuf_destroy_fn, text_hash);
}

/* Creates a pixbuf from a PNG file and returns it.  If 'text_hash' is not
 * NULL, it will be set to point to a hash table storing the image comments
 * (key-text pairs).
 * This is a support function for the thumbnail standard, so it will refuse
 * to load interlaced images or images larger than 256x256 pixels.
 * The hash table will be automatically destroyed when you destroy the pixbuf.
 */
static GdkPixbuf *pixbuf_new_from_png(char *filename, GHashTable **text_hash)
{
	FILE		*file = NULL;
	png_structp	png_ptr = NULL;
	png_infop	info_ptr = NULL;
	GdkPixbuf	*retval = NULL;
	char		header[8];

	g_return_val_if_fail(filename != NULL, NULL);
	g_return_val_if_fail(*filename != '\0', NULL);

	file = fopen(filename, "rb");
	if (!file)
		return NULL;

	/* Is it a PNG file? */
	if (fread(header, 1, 8, file) != 8 || png_sig_cmp(header, 0, 8))
		goto err;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 NULL, NULL, NULL);
	if (!png_ptr)
		goto err;
	
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto err;

	*text_hash = g_hash_table_new(g_str_hash, g_str_equal);

	retval = png_load(png_ptr, info_ptr, file, *text_hash);
err:
	if (file)
		fclose(file);

	if (png_ptr)
	{
		if (info_ptr)
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		else
			png_destroy_read_struct(&png_ptr, NULL, NULL);
	}

	if (!retval)
		g_warning("Error loading thumbnail '%s'", filename);

	return retval;
}
#endif /* THUMBS_USE_LIBPNG */

#if defined(GTK2) || defined(THUMBS_USE_LIBPNG)
/* Create a thumbnail file for this image.
 * XXX: Thumbnails should be deleted somewhere!
 */
static void save_thumbnail(char *path, GdkPixbuf *full, MaskedPixmap *image)
{
	struct stat info;
	int original_width, original_height;
	GString *to;
	char *md5, *swidth, *sheight, *ssize, *smtime, *uri;
	mode_t old_mask;
	int name_len;

	/* If the source image was very small, don't bother saving */
	if (gdk_pixbuf_get_width(full) * gdk_pixbuf_get_height(full) <
			   (HUGE_WIDTH * HUGE_HEIGHT * 3))
		return;

	original_width = gdk_pixbuf_get_width(full);
	original_height = gdk_pixbuf_get_height(full);

	if (mc_stat(path, &info) != 0)
		return;

	swidth = g_strdup_printf("%d", original_width);
	sheight = g_strdup_printf("%d", original_height);
	ssize = g_strdup_printf("%" SIZE_FMT, info.st_size);
	smtime = g_strdup_printf("%ld", info.st_mtime);

	path = pathdup(path);
	uri = g_strconcat("file://", path, NULL);
	md5 = md5_hash(uri);
	g_free(path);
		
	to = g_string_new(home_dir);
	g_string_append(to, "/.thumbnails");
	mkdir(to->str, 0700);
	g_string_append(to, "/normal/");
	mkdir(to->str, 0700);
	g_string_append(to, md5);
	name_len = to->len + 4; /* Truncate to this length when renaming */
	g_string_sprintfa(to, ".png.ROX-Filer-%ld", (long) getpid());

	g_free(md5);

	old_mask = umask(0077);
#ifdef GTK2
	gdk_pixbuf_save(image->huge_pixbuf,
			to->str,
			"png",
			NULL,
			"tEXt::Thumb::Image::Width", swidth,
			"tEXt::Thumb::Image::Height", sheight,
			"tEXt::Thumb::Size", ssize,
			"tEXt::Thumb::MTime", smtime,
			"tEXt::Thumb::URI", uri,
			"tEXt::Software", PROJECT,
			NULL);
#else
	pixbuf_save_png(image->huge_pixbuf,
			to->str,
			"Thumb::Image::Width", swidth,
			"Thumb::Image::Height", sheight,
			"Thumb::Size", ssize,
			"Thumb::MTime", smtime,
			"Thumb::URI", uri,
			"Software", PROJECT,
			NULL);
#endif	
	umask(old_mask);

	/* We create the file ###.png.ROX-Filer-PID and rename it to avoid
	 * a race condition if two program create the same thumb at
	 * once.
	 */
	{
		gchar *final;

		final = g_strndup(to->str, name_len);
		if (rename(to->str, final))
			g_warning("Failed to rename '%s' to '%s': %s",
				  to->str, final, g_strerror(errno));
		g_free(final);
	}

	g_string_free(to, TRUE);
	g_free(swidth);
	g_free(sheight);
	g_free(ssize);
	g_free(smtime);
	g_free(uri);
}
#endif /* GTK2 or THUMBS_USE_LIBPNG */

/* Check if we have an up-to-date thumbnail for this image.
 * If so, return it. Otherwise, returns NULL.
 */
static GdkPixbuf *get_thumbnail_for(char *path)
{
	GdkPixbuf *thumb = NULL;
#if defined(GTK2) || defined(THUMBS_USE_LIBPNG)
	char *thumb_path, *md5, *uri;
	char *ssize, *smtime;
	struct stat info;
#ifndef GTK2
	GHashTable *text_hash;
#endif

	path = pathdup(path);
	uri = g_strconcat("file://", path, NULL);
	md5 = md5_hash(uri);
	g_free(uri);
	
	thumb_path = g_strdup_printf("%s/.thumbnails/normal/%s.png",
					home_dir, md5);
	g_free(md5);

#ifdef GTK2
	thumb = gdk_pixbuf_new_from_file(thumb_path, NULL);
#else
	thumb = pixbuf_new_from_png(thumb_path, &text_hash);
#endif
	if (!thumb)
		goto err;

	/* Note that these don't need freeing... */
#ifdef GTK2
	ssize = gdk_pixbuf_get_option(thumb, "tEXt::Thumb::Size");
#else
	ssize = g_hash_table_lookup(text_hash, "Thumb::Size");
#endif	
	if (!ssize)
		goto err;

#ifdef GTK2
	smtime = gdk_pixbuf_get_option(thumb, "tEXt::Thumb::MTime");
#else
	smtime = g_hash_table_lookup(text_hash, "Thumb::MTime");
#endif
	if (!smtime)
		goto err;
	
	if (mc_stat(path, &info) != 0)
		goto err;
	
	if (info.st_mtime != atol(smtime) || info.st_size != atol(ssize))
		goto err;

	goto out;
err:
	if (thumb)
		gdk_pixbuf_unref(thumb);
	thumb = NULL;
out:
	g_free(path);
	g_free(thumb_path);
#endif /* GTK2 or THUMBS_USE_LIBPNG */
	return thumb;
}

/* Load the image 'path' and return a pointer to the resulting
 * MaskedPixmap. NULL on failure.
 * Doesn't check for thumbnails (this is for small icons).
 */
static MaskedPixmap *image_from_file(char *path)
{
	GdkPixbuf	*pixbuf;
	MaskedPixmap	*image;
#ifdef GTK2
	GError		*error = NULL;
	
	pixbuf = gdk_pixbuf_new_from_file(path, &error);
	if (!pixbuf)
	{
		g_print("%s\n", error ? error->message : _("Unknown error"));
		g_error_free(error);
	}
#else
	pixbuf = gdk_pixbuf_new_from_file(path);
#endif
	if (!pixbuf)
		return NULL;

	image = image_from_pixbuf(pixbuf);

	gdk_pixbuf_unref(pixbuf);

	return image;
}

/* Scale src down to fit in max_w, max_h and return the new pixbuf.
 * If src is small enough, then ref it and return that.
 */
static GdkPixbuf *scale_pixbuf(GdkPixbuf *src, int max_w, int max_h)
{
	int	w, h;

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);

	if (w <= max_w && h <= max_h)
	{
		gdk_pixbuf_ref(src);
		return src;
	}
	else
	{
		float scale_x = ((float) w) / max_w;
		float scale_y = ((float) h) / max_h;
		float scale = MAX(scale_x, scale_y);
		int dest_w = w / scale;
		int dest_h = h / scale;
		
		return gdk_pixbuf_scale_simple(src,
						MAX(dest_w, 1),
						MAX(dest_h, 1),
						GDK_INTERP_BILINEAR);
	}
}

/* Scale src up to fit in max_w, max_h and return the new pixbuf.
 * If src is that size or bigger, then ref it and return that.
 */
static GdkPixbuf *scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h)
{
	int	w, h;

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);

	if (w == 0 || h == 0 || (w >= max_w && h >= max_h))
	{
		gdk_pixbuf_ref(src);
		return src;
	}
	else
	{
		float scale_x = max_w / ((float) w);
		float scale_y = max_h / ((float) h);
		float scale = MIN(scale_x, scale_y);
		
		return gdk_pixbuf_scale_simple(src,
						w * scale,
						h * scale,
						GDK_INTERP_BILINEAR);
	}
}

/* Turn a full-size pixbuf into a MaskedPixmap */
MaskedPixmap *image_from_pixbuf(GdkPixbuf *full_size)
{
	MaskedPixmap	*mp;
	GdkPixbuf	*huge_pixbuf, *normal_pixbuf;
	GdkPixmap	*pixmap;
	GdkBitmap	*mask;

	g_return_val_if_fail(full_size != NULL, NULL);

	huge_pixbuf = scale_pixbuf(full_size, HUGE_WIDTH, HUGE_HEIGHT);
	g_return_val_if_fail(huge_pixbuf != NULL, NULL);

	normal_pixbuf = scale_pixbuf(huge_pixbuf, ICON_WIDTH, ICON_HEIGHT);
	g_return_val_if_fail(normal_pixbuf != NULL, NULL);

	gdk_pixbuf_render_pixmap_and_mask(normal_pixbuf, &pixmap, &mask, 128);

	if (!pixmap)
	{
		gdk_pixbuf_unref(huge_pixbuf);
		gdk_pixbuf_unref(normal_pixbuf);
		return NULL;
	}

	mp = g_new(MaskedPixmap, 1);
	mp->huge_pixbuf = huge_pixbuf;
	mp->ref = 1;

	mp->pixmap = pixmap;
	mp->mask = mask;
	mp->width = gdk_pixbuf_get_width(normal_pixbuf);
	mp->height = gdk_pixbuf_get_height(normal_pixbuf);

	gdk_pixbuf_unref(normal_pixbuf);

	mp->huge_pixmap = NULL;
	mp->huge_mask = NULL;
	mp->huge_width = -1;
	mp->huge_height = -1;

	mp->sm_pixmap = NULL;
	mp->sm_mask = NULL;
	mp->sm_width = -1;
	mp->sm_height = -1;

	return mp;
}

/* Return a pointer to the (static) bad image. The ref counter will ensure
 * that the image is never freed.
 */
static MaskedPixmap *get_bad_image(void)
{
	GdkPixbuf *bad;
	MaskedPixmap *mp;
	
	bad = gdk_pixbuf_new_from_xpm_data(bad_xpm);
	mp = image_from_pixbuf(bad);
	gdk_pixbuf_unref(bad);

	return mp;
}

static MaskedPixmap *load(char *pathname, gpointer user_data)
{
	return image_from_file(pathname);
}

static void ref(MaskedPixmap *mp, gpointer data)
{
	/* printf("[ ref %p %d->%d ]\n", mp, mp->ref, mp->ref + 1); */
	
	if (mp)
		mp->ref++;
}

static void unref(MaskedPixmap *mp, gpointer data)
{
	/* printf("[ unref %p %d->%d ]\n", mp, mp->ref, mp->ref - 1); */
	
	if (mp && --mp->ref == 0)
	{
		if (mp->huge_pixbuf)
		{
			gdk_pixbuf_unref(mp->huge_pixbuf);
		}

		if (mp->huge_pixmap)
			gdk_pixmap_unref(mp->huge_pixmap);
		if (mp->huge_mask)
			gdk_bitmap_unref(mp->huge_mask);

		if (mp->pixmap)
			gdk_pixmap_unref(mp->pixmap);
		if (mp->mask)
			gdk_bitmap_unref(mp->mask);

		if (mp->sm_pixmap)
			gdk_pixmap_unref(mp->sm_pixmap);
		if (mp->sm_mask)
			gdk_bitmap_unref(mp->sm_mask);

		g_free(mp);
	}	
}

static int getref(MaskedPixmap *mp)
{
	return mp ? mp->ref : 0;
}

/* Called now and then to clear out old pixmaps */
static gint purge(gpointer data)
{
	g_fscache_purge(pixmap_cache, PIXMAP_PURGE_TIME);

	return TRUE;
}

static void got_thumb_data(GdkPixbufLoader *loader,
				gint fd, GdkInputCondition cond)
{
	GFunc callback;
	gchar *path;
	gpointer cbdata;
	char data[4096];
	int  got, tag;

	got = read(fd, data, sizeof(data));

#ifdef GTK2
	if (got > 0 && gdk_pixbuf_loader_write(loader, data, got, NULL))
		return;
#else
	if (got > 0 && gdk_pixbuf_loader_write(loader, data, got))
		return;
#endif

	/* Some kind of error, or end-of-file */

	path = GET_LOADER("thumb-path");

	tag = GPOINTER_TO_INT(GET_LOADER("thumb-input-tag"));
	gdk_input_remove(tag);

#ifdef GTK2
	gdk_pixbuf_loader_close(loader, NULL);
#else
	gdk_pixbuf_loader_close(loader);
#endif

	if (got == 0)
	{
		MaskedPixmap *image;
		GdkPixbuf *pixbuf;

		pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
		if (pixbuf)
		{
			gdk_pixbuf_ref(pixbuf);
			image = image_from_pixbuf(pixbuf);

			g_fscache_insert(pixmap_cache, path, image, FALSE);
#if defined(GTK2) || defined(THUMBS_USE_LIBPNG)
			save_thumbnail(path, pixbuf, image);
#endif
			gdk_pixbuf_unref(pixbuf);
			pixmap_unref(image);
		}
		else
			got = -1;
	}

	mc_close(fd);
	
	callback = GET_LOADER("thumb-callback");
	cbdata = GET_LOADER("thumb-callback-data");

	callback(cbdata, got != 0 ? NULL : path);

#ifdef GTK2
	g_object_unref(G_OBJECT(loader));
#else
	gtk_object_unref(GTK_OBJECT(loader));
#endif
}
