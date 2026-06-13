#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c508-0x8003c518.
void MainLoadVLC_Callback(struct LoadQueueSlot *param_1)
{
	// VLC is now loaded
	sdata->bool_IsLoaded_VlcTable = 1;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native keeps the loaded VLC pointer in host-visible state.
	sdata->ptrVlcTable = param_1->ptrDestination;
#endif
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c518-0x8003c58c.
void MainLoadVLC(void)
{
	struct BigHeader *bigfile;
	struct BigEntry *entry;
	int size;

	// VLC is not loaded
	sdata->bool_IsLoaded_VlcTable = 0;

	bigfile = sdata->ptrBigfileCdPos_2;
	entry = BIG_GETENTRY(bigfile);
	size = (entry[BI_VLCTABLE].size + 0x7ff) & 0xfffff800;
	sdata->ptrVlcTable = MEMPACK_AllocMem(size);

	// This table is passed as parameter to DecDCTvlc2
	LOAD_AppendQueue(bigfile, LT_SETADDR, BI_VLCTABLE, sdata->ptrVlcTable, MainLoadVLC_Callback);
}
