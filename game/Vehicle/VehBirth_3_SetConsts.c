#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058a60-0x80058ba4.
void VehBirth_SetConsts(struct Driver *driver)
{
	u32 metaPhysSize;
	u32 i;
	struct MetaPhys *metaPhys;
	u8 *d;

	d = (u8 *)driver;

	int engineID = data.MetaDataCharacters[data.characterIDs[driver->driverID]].engineID;

	for (i = 0; i < 65; i++)
	{
		metaPhys = &data.metaPhys[i];

		metaPhysSize = metaPhys->size;

		u32 rawValue = (u32)metaPhys->value[engineID];
		u8 *dst = &d[metaPhys->offset];

		if (metaPhysSize == 1)
		{
			dst[0] = (u8)rawValue;
			continue;
		}

		if (metaPhysSize == 2)
		{
			dst[0] = (u8)rawValue;
			dst[1] = (u8)(rawValue >> 8);
			continue;
		}

		if (metaPhysSize == 4)
		{
			dst[0] = (u8)rawValue;
			dst[1] = (u8)(rawValue >> 8);
			dst[2] = (u8)(rawValue >> 16);
			dst[3] = (u8)(rawValue >> 24);
		}
	}

	return;
}
