#include <common.h>

static u32 VehLap_UpdateProgress_PackS16Pair(s32 lo, s32 hi)
{
	return (u16)lo | ((u32)(u16)hi << 16);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005ca24-0x8005cd1c
void VehLap_UpdateProgress(struct Driver *driver)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 checkpointIndex = -1;

	if (driver == NULL)
		return;

	if ((driver->actionsFlagSet & 0x100000) == 0)
	{
		struct QuadBlock *quad = driver->lastValid;

		if ((quad != NULL) && (quad->checkpointIndex != 0xff))
		{
			checkpointIndex = quad->checkpointIndex;
		}
	}
	else
	{
		// Retail reads only byte 0x60a even though the local struct field is s16.
		checkpointIndex = *(u8 *)&driver->botData.ai_quadblock_checkpointIndex;
	}

	struct Level *level = gGT->level1;
	if (((u32)(level->cnt_restart_points - 1) >= 0xff) || (checkpointIndex < 0))
		return;

	struct CheckpointNode *nodes = level->ptr_restart_points;
	struct CheckpointNode *checkpointNode = &nodes[checkpointIndex];
	struct CheckpointNode *progressNode = &nodes[checkpointNode->nextIndex_forward];
	struct CheckpointNode *nextNode = &nodes[progressNode->nextIndex_forward];

	SVec4 nodeDelta;
	nodeDelta.x = progressNode->pos[0] - nextNode->pos[0];
	nodeDelta.y = progressNode->pos[1] - nextNode->pos[1];
	nodeDelta.z = progressNode->pos[2] - nextNode->pos[2];
	nodeDelta.w = 0;

	MATH_VectorNormalize((SVec3 *)&nodeDelta);

	s16 deltaX = (driver->posCurr.x >> 8) - progressNode->pos[0];
	s16 deltaY = (driver->posCurr.y >> 8) - progressNode->pos[1];
	s16 deltaZ = (driver->posCurr.z >> 8) - progressNode->pos[2];

	CTC2(VehLap_UpdateProgress_PackS16Pair(deltaX, deltaY), 0);
	CTC2(VehLap_UpdateProgress_PackS16Pair(deltaZ, driver->matrixMovingDir.m[0][2] >> 5), 1);
	CTC2(VehLap_UpdateProgress_PackS16Pair(driver->matrixMovingDir.m[1][2] >> 5, driver->matrixMovingDir.m[2][2] >> 5), 2);

	gte_ldv0(&nodeDelta);
	gte_mvmva(0, 0, 0, 3, 0);

	s32 projection = MFC2_S(25);
	s32 wrongWayTest = MFC2_S(26);
	s32 progress = ((u32)progressNode->distToFinish << 3) + (projection >> 0xc);
	s32 trackLength = (s32)nodes[0].distToFinish << 3;

	driver->distanceToFinish_curr = progress;
	// NOTE(aalhendi): Retail uses signed div/mfhi for this remainder.
	driver->distanceToFinish_curr = progress % trackLength;

	if (wrongWayTest < 0x5a801)
	{
		driver->actionsFlagSet &= ~0x100;
	}
	else
	{
		driver->actionsFlagSet |= 0x100;
	}

	if (((driver->actionsFlagSet & 0x8000000) != 0) && (driver->unknown_lap_related[1] != (u8)checkpointIndex))
	{
		driver->unknown_lap_related[0] = checkpointIndex;
		driver->actionsFlagSet &= ~0x8000000;
	}

	if (checkpointNode->nextIndex_left != 0xff)
	{
		driver->actionsFlagSet |= 0x8000000;
	}

	driver->unknown_lap_related[1] = checkpointIndex;
}
