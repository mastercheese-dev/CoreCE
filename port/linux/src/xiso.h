/*
XISO.H

The maps folder out of an Xbox disc image (xiso.c).
*/

#ifndef __HALO_LINUX_XISO_H
#define __HALO_LINUX_XISO_H

/* called as the copy goes: the file being copied, and the bytes copied of
all the files' */
typedef void (*xiso_progress_proc)(void *context, const char *file, unsigned long long done,
	unsigned long long total);

/* copies the image's maps folder to <destination>/maps; returns nonzero on
success, or 0 with the reason (for the player) in error. Called from any
thread. When the maps folder has no loading.tga, the picture is read from
the disc's default.xbe and written as that file. */
int xiso_extract_maps(const char *image_path, const char *destination, xiso_progress_proc progress, void *context,
	char *error, int error_size);

/* writes the loading screen's picture from the image's default.xbe into an
existing maps folder, as maps_directory/loading.tga. Returns nonzero when
the file was written, or 0 with the reason in error. For a maps folder
copied before that picture was extracted. */
int xiso_extract_loading_picture(const char *image_path, const char *maps_directory,
	char *error, int error_size);

#endif
