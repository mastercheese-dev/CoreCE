/* The spawn pad and respawn meshes are text in port/assets/hardcore, and the
 * timer recordings are wav files, so a checkout has them without the hardcore
 * map files. tools/embed_assets.py compiles the text and stores each file. */
#ifndef HARDCORE_ASSETS_H
#define HARDCORE_ASSETS_H

enum
{
	HARDCORE_ASSET_MARKER = 1,
	HARDCORE_ASSET_AREA = 2,
	HARDCORE_ASSET_SOUND = 3
};

/* one file under port/assets/hardcore, as the build embedded it */
struct hardcore_asset_embedded
{
	const char *path;
	const unsigned int *data;
	unsigned int size;
};

extern const struct hardcore_asset_embedded hardcore_assets_embedded[];
extern const unsigned int hardcore_assets_embedded_count;

/* bytes stay valid for the rest of the run. 0 when this copy has no match. */
int hardcore_assets_find(unsigned kind, char const *name,
	unsigned char const **bytes, unsigned long *size);

#endif
