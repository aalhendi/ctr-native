#include <common.h>

_Static_assert(sizeof(struct QuadBlock) == 0x5c);

static void CAM_FindClosestQuadblock_SetScratchpadWord(s16 *scratchpad, int halfwordIndex, int value)
{
	*(int *)(scratchpad + halfwordIndex) = value;
}

static int CAM_FindClosestQuadblock_GetScratchpadWord(s16 *scratchpad, int halfwordIndex)
{
	return *(int *)(scratchpad + halfwordIndex);
}

static void CAM_FindClosestQuadblock_SetCameraHitFlag(struct CameraDC *cDC, int value)
{
	*(int *)((u8 *)cDC + 0x3c) = value;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800188a8-0x80018b18
void CAM_FindClosestQuadblock(s16 *scratchpad, struct CameraDC *cDC, struct Driver *d, s16 *pos)
{
	struct GameTracker *gGT;
	struct mesh_info *meshInfo;
	struct QuadBlock *quad;

	(void)d;

	scratchpad[8] = pos[0];
	scratchpad[9] = pos[2];
	scratchpad[10] = pos[4];

	scratchpad[0] = pos[0];
	scratchpad[1] = (s16)((u16)pos[2] - 0x800);
	scratchpad[14] = scratchpad[0];
	scratchpad[2] = pos[4];
	scratchpad[9] = (s16)((u16)scratchpad[9] + 0x100);
	scratchpad[15] = scratchpad[1];
	scratchpad[16] = pos[4];

	scratchpad[0x18] = scratchpad[0] < scratchpad[8] ? scratchpad[0] : scratchpad[8];
	scratchpad[0x19] = scratchpad[1] < scratchpad[9] ? scratchpad[1] : scratchpad[9];
	scratchpad[0x1a] = scratchpad[2] < scratchpad[10] ? scratchpad[2] : scratchpad[10];
	scratchpad[0x1b] = scratchpad[8] < scratchpad[0] ? scratchpad[0] : scratchpad[8];
	scratchpad[0x1c] = scratchpad[9] < scratchpad[1] ? scratchpad[1] : scratchpad[9];
	scratchpad[0x1d] = scratchpad[10] < scratchpad[2] ? scratchpad[2] : scratchpad[10];

	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x12, 0x800);
	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x14, 0);
	scratchpad[0x1f] = 0;
	scratchpad[0x1e] = 0;
	scratchpad[0x11] = 0;
	CAM_FindClosestQuadblock_SetCameraHitFlag(cDC, 0);

	gGT = sdata->gGT;

	if ((gGT->level1 == NULL) || (gGT->level1->ptr_mesh_info == NULL) || (gGT->level1->ptr_mesh_info->bspRoot == NULL))
	{
		CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x16, 0);
		return;
	}

	meshInfo = gGT->level1->ptr_mesh_info;
	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x16, (int)meshInfo);

	if (cDC->ptrQuadBlock != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(cDC->ptrQuadBlock, (struct ScratchpadStruct *)scratchpad);
	}

	if (scratchpad[0x1f] == 0)
	{
		DECOMP_COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, (struct BoundingBox *)(scratchpad + 0x18), COLL_FIXED_BSPLEAF_TestQuadblocks,
		                                    (struct ScratchpadStruct *)scratchpad);
	}

	if (scratchpad[0x1f] == 0)
	{
		gGT->unk1cac[0] = -1;
		return;
	}

	CAM_FindClosestQuadblock_SetCameraHitFlag(cDC, 1);

	quad = (struct QuadBlock *)CAM_FindClosestQuadblock_GetScratchpadWord(scratchpad, 0x40);
	cDC->ptrQuadBlock = quad;
	gGT->unk1cac[0] = quad - meshInfo->ptrQuadBlockArray;
}
