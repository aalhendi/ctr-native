#include <common.h>

void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	u32 numQuads;
	struct QuadBlock *ptrQuad;

	// if bsp flag is water
	if ((node->flag & 2) != 0)
	{
		*(int *)&sps->dataOutput[0] |= 0x8000;
	}

	numQuads = node->data.leaf.numQuads;
	ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	while (numQuads-- != 0)
	{
		COLL_FIXED_QUADBLK_TestTriangles(ptrQuad++, sps);
	}

	if ((sps->Union.QuadBlockColl.searchFlags & 1) != 0)
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): CTR_NATIVE bridge: CAM_FindClosestQuadblock clears
		// this flag. Instance hitbox search stays disabled here until the
		// fixed COLL instance path is wired for native.
		return;
#else
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
#endif
	}
}
