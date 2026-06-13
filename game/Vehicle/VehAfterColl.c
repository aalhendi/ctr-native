#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80057c44-0x80057c68.
struct Scrub *VehAfterColl_GetSurface(u32 scrubId)
{
	struct Scrub *sc = &data.MetaDataScrub[0];

	if (scrubId < 7)
		return &sc[scrubId];

	return sc;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80057c68-0x80057c8c.
struct Terrain *VehAfterColl_GetTerrain(u8 terrainType)
{
	struct Terrain *ter = &data.MetaDataTerrain[0];

	// if terrain is valid
	if (terrainType <= TERRAIN_SLOWDIRT) // 20
		return &ter[terrainType];

	return ter;
}
