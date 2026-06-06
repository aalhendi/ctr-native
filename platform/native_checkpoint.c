#include "platform/native_checkpoint.h"

#include <macros.h>

#include "../platform.h"
#include "ctr_scratchpad.h"
#include "platform/native_state.h"

#include <string.h>

#define NATIVE_CHECKPOINT_FOURCC(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

// NOTE(aalhendi): Whole-machine checkpoints are included after native memory
// and retail globals are defined, so they can snapshot the same process-local
// regions the game mutates.
#define NATIVE_CHECKPOINT_MAGIC              NATIVE_CHECKPOINT_FOURCC('C', 'T', 'R', 'C')
#define NATIVE_CHECKPOINT_VERSION            1u

enum NativeCheckpointRegionKind
{
	NATIVE_CHECKPOINT_REGION_RDATA = NATIVE_CHECKPOINT_FOURCC('R', 'D', 'A', 'T'), // resident rdata globals
	NATIVE_CHECKPOINT_REGION_DATA = NATIVE_CHECKPOINT_FOURCC('D', 'A', 'T', 'A'),  // resident data globals
	NATIVE_CHECKPOINT_REGION_SDATA = NATIVE_CHECKPOINT_FOURCC('S', 'D', 'A', 'T'), // resident sdata globals
	NATIVE_CHECKPOINT_REGION_D230 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '0'),  // main-menu overlay data
	NATIVE_CHECKPOINT_REGION_V230 = NATIVE_CHECKPOINT_FOURCC('V', '2', '3', '0'),  // main-menu video BSS
	NATIVE_CHECKPOINT_REGION_D231 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '1'),  // race/battle overlay data
	NATIVE_CHECKPOINT_REGION_D232 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '2'),  // adventure overlay data
	NATIVE_CHECKPOINT_REGION_D233 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '3'),  // cutscene overlay mutable data
	NATIVE_CHECKPOINT_REGION_GAR3 = NATIVE_CHECKPOINT_FOURCC('G', 'A', 'R', '3'),  // garage runtime state
	NATIVE_CHECKPOINT_REGION_CRD3 = NATIVE_CHECKPOINT_FOURCC('C', 'R', 'D', '3'),  // credits runtime state
	NATIVE_CHECKPOINT_REGION_MPAK = NATIVE_CHECKPOINT_FOURCC('M', 'P', 'A', 'K'),  // mempack backing store
	NATIVE_CHECKPOINT_REGION_SCRP = NATIVE_CHECKPOINT_FOURCC('S', 'C', 'R', 'P'),  // PS1 scratchpad RAM
	NATIVE_CHECKPOINT_REGION_NATS = NATIVE_CHECKPOINT_FOURCC('N', 'A', 'T', 'S'),  // native subsystem state bundle
};

struct NativeCheckpointRegion
{
	u32 kind;
	u32 offset;
	u32 size;
};

struct NativeCheckpointHeader
{
	u32 magic;
	u32 version;
	u32 size;
	u32 regionCount;
	struct PlatformMempackArena mempackArena;
	u32 psxRandSeed;
	s32 activeMempackIndex;
	u32 reserved[2];
	struct NativeCheckpointRegion regions[13];
};

internal u32 NativeCheckpoint_Align4(u32 value)
{
	return (value + 3u) & ~3u;
}

internal int NativeCheckpoint_GetActiveMempackIndex(void)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (sdata_static.PtrMempack == &sdata_static.mempack[i])
			return i;
	}

	if ((sdata_static.gameTracker.activeMempackIndex >= 0) && (sdata_static.gameTracker.activeMempackIndex < 4))
		return sdata_static.gameTracker.activeMempackIndex;

	return 0;
}

internal int NativeCheckpoint_GetRegionSize(u32 kind)
{
	switch (kind)
	{
	case NATIVE_CHECKPOINT_REGION_RDATA:
		return (int)sizeof(rdata);
	case NATIVE_CHECKPOINT_REGION_DATA:
		return (int)sizeof(data);
	case NATIVE_CHECKPOINT_REGION_SDATA:
		return (int)sizeof(sdata_static);
	case NATIVE_CHECKPOINT_REGION_D230:
		return (int)sizeof(D230);
	case NATIVE_CHECKPOINT_REGION_V230:
		return (int)sizeof(V230);
	case NATIVE_CHECKPOINT_REGION_D231:
		return (int)sizeof(D231);
	case NATIVE_CHECKPOINT_REGION_D232:
		return (int)sizeof(D232);
	case NATIVE_CHECKPOINT_REGION_D233:
		return (int)sizeof(D233);
	case NATIVE_CHECKPOINT_REGION_GAR3:
		return (int)sizeof(gGarage);
	case NATIVE_CHECKPOINT_REGION_CRD3:
		return (int)sizeof(creditsBSS) - OFFSETOF(struct Ovr233_Credits_BSS, CreditThread);
	case NATIVE_CHECKPOINT_REGION_MPAK:
		return (int)sizeof(s_mempackMemory);
	case NATIVE_CHECKPOINT_REGION_SCRP:
		return (int)CTR_SCRATCHPAD_SIZE;
	case NATIVE_CHECKPOINT_REGION_NATS:
		return NativeState_GetSize();
	}

	return 0;
}

internal void *NativeCheckpoint_GetRegionPtr(u32 kind)
{
	switch (kind)
	{
	case NATIVE_CHECKPOINT_REGION_RDATA:
		return &rdata;
	case NATIVE_CHECKPOINT_REGION_DATA:
		return &data;
	case NATIVE_CHECKPOINT_REGION_SDATA:
		return &sdata_static;
	case NATIVE_CHECKPOINT_REGION_D230:
		return &D230;
	case NATIVE_CHECKPOINT_REGION_V230:
		return &V230;
	case NATIVE_CHECKPOINT_REGION_D231:
		return &D231;
	case NATIVE_CHECKPOINT_REGION_D232:
		return &D232;
	case NATIVE_CHECKPOINT_REGION_GAR3:
		return &gGarage;
	case NATIVE_CHECKPOINT_REGION_CRD3:
		return &creditsBSS.CreditThread;
	case NATIVE_CHECKPOINT_REGION_MPAK:
		return &s_mempackMemory[0];
	case NATIVE_CHECKPOINT_REGION_SCRP:
		return CTR_SCRATCHPAD_BASE;
	}

	return NULL;
}

internal int NativeCheckpoint_CaptureD233(void *dst, int dstSize)
{
	struct OverlayDATA_233 *state = (struct OverlayDATA_233 *)dst;

	if ((dst == NULL) || (dstSize != (int)sizeof(*state)))
		return 0;

	*state = D233;
	memset(state->cs_initMatrixTable, 0, sizeof(state->cs_initMatrixTable));

	return 1;
}

internal int NativeCheckpoint_RestoreD233(const void *src, int srcSize)
{
	const struct OverlayDATA_233 *state = (const struct OverlayDATA_233 *)src;

	if ((src == NULL) || (srcSize != (int)sizeof(*state)))
		return 0;

	D233 = *state;
	OVR233_RebuildInitMatrixTable();

	return 1;
}

internal int NativeCheckpoint_CaptureRegion(u32 kind, void *dst, int dstSize)
{
	void *src;

	if (kind == NATIVE_CHECKPOINT_REGION_D233)
		return NativeCheckpoint_CaptureD233(dst, dstSize);
	if (kind == NATIVE_CHECKPOINT_REGION_NATS)
		return NativeState_Capture(dst, dstSize);

	src = NativeCheckpoint_GetRegionPtr(kind);
	if (src == NULL)
		return 0;

	memcpy(dst, src, (size_t)dstSize);
	return 1;
}

internal int NativeCheckpoint_RestoreRegion(u32 kind, const void *src, int srcSize)
{
	void *dst;

	if (kind == NATIVE_CHECKPOINT_REGION_D233)
		return NativeCheckpoint_RestoreD233(src, srcSize);

	dst = NativeCheckpoint_GetRegionPtr(kind);
	if (dst == NULL)
		return 0;

	memcpy(dst, src, (size_t)srcSize);
	return 1;
}

internal int NativeCheckpoint_InitHeader(struct NativeCheckpointHeader *header)
{
	u32 offset = NativeCheckpoint_Align4((u32)sizeof(*header));
	u32 i;
	local_persist const u32 regionKinds[] = {
	    NATIVE_CHECKPOINT_REGION_RDATA, NATIVE_CHECKPOINT_REGION_DATA, NATIVE_CHECKPOINT_REGION_SDATA, NATIVE_CHECKPOINT_REGION_D230,
	    NATIVE_CHECKPOINT_REGION_V230,  NATIVE_CHECKPOINT_REGION_D231, NATIVE_CHECKPOINT_REGION_D232,  NATIVE_CHECKPOINT_REGION_D233,
	    NATIVE_CHECKPOINT_REGION_GAR3,  NATIVE_CHECKPOINT_REGION_CRD3, NATIVE_CHECKPOINT_REGION_MPAK,  NATIVE_CHECKPOINT_REGION_SCRP,
	    NATIVE_CHECKPOINT_REGION_NATS,
	};

	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_CHECKPOINT_MAGIC;
	header->version = NATIVE_CHECKPOINT_VERSION;
	header->regionCount = (u32)len(regionKinds);

	if (header->regionCount > len(header->regions))
		return 0;

	for (i = 0; i < header->regionCount; i++)
	{
		const int regionSize = NativeCheckpoint_GetRegionSize(regionKinds[i]);

		if (regionSize <= 0)
			return 0;

		header->regions[i].kind = regionKinds[i];
		header->regions[i].offset = offset;
		header->regions[i].size = (u32)regionSize;
		offset = NativeCheckpoint_Align4(offset + (u32)regionSize);
	}

	header->size = offset;
	return 1;
}

internal int NativeCheckpoint_ValidateHeader(const struct NativeCheckpointHeader *header, int srcSize)
{
	struct NativeCheckpointHeader liveHeader;
	u32 i;

	if ((header == NULL) || (srcSize < (int)sizeof(*header)))
		return 0;
	if ((header->magic != NATIVE_CHECKPOINT_MAGIC) || (header->version != NATIVE_CHECKPOINT_VERSION))
		return 0;
	if ((header->size < sizeof(*header)) || (header->size > (u32)srcSize))
		return 0;
	if (!NativeCheckpoint_InitHeader(&liveHeader))
		return 0;
	if ((header->size != liveHeader.size) || (header->regionCount != liveHeader.regionCount))
		return 0;

	for (i = 0; i < header->regionCount; i++)
	{
		const struct NativeCheckpointRegion *region = &header->regions[i];
		const struct NativeCheckpointRegion *liveRegion = &liveHeader.regions[i];
		const u32 end = region->offset + region->size;

		if ((region->kind != liveRegion->kind) || (region->offset != liveRegion->offset) || (region->size != liveRegion->size))
			return 0;
		if ((region->size == 0) || (region->offset < sizeof(*header)) || (end < region->offset) || (end > header->size))
			return 0;
	}

	return 1;
}

int NativeCheckpoint_GetSize(void)
{
	struct NativeCheckpointHeader header;

	if (!NativeCheckpoint_InitHeader(&header))
		return 0;

	return (int)header.size;
}

int NativeCheckpoint_Capture(void *dst, int dstSize)
{
	struct NativeCheckpointHeader header;
	u8 *bytes = (u8 *)dst;
	u32 i;

	if (!NativeCheckpoint_InitHeader(&header))
		return 0;
	if ((dst == NULL) || (dstSize < (int)header.size))
		return 0;

	header.mempackArena = s_mempackArena;
	header.psxRandSeed = psxRandSeed;
	header.activeMempackIndex = NativeCheckpoint_GetActiveMempackIndex();

	memset(dst, 0, header.size);
	memcpy(dst, &header, sizeof(header));

	for (i = 0; i < header.regionCount; i++)
	{
		struct NativeCheckpointRegion *region = &header.regions[i];

		if (!NativeCheckpoint_CaptureRegion(region->kind, &bytes[region->offset], (int)region->size))
			return 0;
	}

	return 1;
}

int NativeCheckpoint_Restore(const void *src, int srcSize)
{
	const struct NativeCheckpointHeader *header = (const struct NativeCheckpointHeader *)src;
	const u8 *bytes = (const u8 *)src;
	const struct NativeCheckpointRegion *nativeStateRegion = NULL;
	u32 i;

	if (!NativeCheckpoint_ValidateHeader(header, srcSize))
		return 0;

	// NOTE(aalhendi): 233 checkpoints store only mutable overlay state. Restore
	// the source-owned static image first, then overlay the captured runtime
	// fields below.
	OVR233_ResetRuntimeState();

	for (i = 0; i < header->regionCount; i++)
	{
		const struct NativeCheckpointRegion *region = &header->regions[i];

		if (region->kind == NATIVE_CHECKPOINT_REGION_NATS)
		{
			nativeStateRegion = region;
		}
		else
		{
			if (!NativeCheckpoint_RestoreRegion(region->kind, &bytes[region->offset], (int)region->size))
				return 0;
		}
	}

	psxRandSeed = header->psxRandSeed;
	Platform_ConfigureMempackArena();
	Platform_RepairResidentPointers(header->activeMempackIndex);

	if (nativeStateRegion == NULL)
		return 0;
	if (!NativeState_Restore(&bytes[nativeStateRegion->offset], (int)nativeStateRegion->size))
		return 0;

	return 1;
}
