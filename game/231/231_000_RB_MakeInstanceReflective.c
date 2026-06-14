#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abab0-0x800abbb4.
void RB_MakeInstanceReflective(struct ScratchpadStruct *sps, struct Instance *inst)
{
	u16 quadFlags;
	struct GameTracker *gGT = sdata->gGT;

	if ((sps->boolDidTouchQuadblock == 0) || (sps->boolDidTouchHitbox != 0))
	{
		inst->bitCompressed_NormalVector_AndDriverIndex = 0x4000;
	}
	else
	{
		inst->bitCompressed_NormalVector_AndDriverIndex = ((u32)(sps->hit.plane.normal.x >> 6) & 0xff) | (((u32)sps->hit.plane.normal.y & 0x3fc0) << 2) |
		                                                  (((u32)(sps->hit.plane.normal.z >> 6) & 0xff) << 0x10);

		if (1 < gGT->numPlyrCurrGame)
			return;

		quadFlags = sps->hit.ptrQuadblock->quadFlags;

		if ((quadFlags & 0x2000) == 0)
		{
			if ((quadFlags & 1) != 0)
			{
				inst->flags |= REFLECTIVE;
				inst->vertSplit = gGT->level1->splitLines[1];
				return;
			}

			if ((quadFlags & 4) != 0)
			{
				inst->flags |= REFLECTIVE;
				inst->vertSplit = gGT->level1->splitLines[0];
				return;
			}
		}
	}

	inst->flags &= ~REFLECTIVE;
}
