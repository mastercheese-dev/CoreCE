/* The pad and the respawn meshes are text in port/assets/hardcore. The build
compiles that text into the bytes this lookup returns. The timer recordings
are the wav files. A map file is still used when this copy has no matching piece. */

#include "cseries.h"
#include "hardcore_assets.h"

#include <stdio.h>
#include <string.h>

int hardcore_assets_find(unsigned kind, char const *name,
	unsigned char const **bytes, unsigned long *size)
{
	char path[64];
	unsigned int index;

	if (!name || !name[0] || !bytes || !size || strlen(name) > 32)
		return 0;
	if (kind == HARDCORE_ASSET_MARKER)
		snprintf(path, sizeof(path), "marker.txt");
	else if (kind == HARDCORE_ASSET_AREA)
		snprintf(path, sizeof(path), "areas/%s.txt", name);
	else if (kind == HARDCORE_ASSET_SOUND)
		snprintf(path, sizeof(path), "sounds/%s.wav", name);
	else
		return 0;
	for (index = 0; index < hardcore_assets_embedded_count; index++)
	{
		struct hardcore_asset_embedded const *embedded = &hardcore_assets_embedded[index];

		if (embedded->path && !strcmp(embedded->path, path))
		{
			*bytes = (unsigned char const *)embedded->data;
			*size = embedded->size;
			return 1;
		}
	}
	return 0;
}
