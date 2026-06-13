#include <common.h>

static u32 VehGroundSkids_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static void VehGroundSkids_WriteLo16(u32 *word, u16 value)
{
	*(u16 *)word = value;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c120-0x8005c278.
void VehGroundSkids_Subset1(u32 *currXY, u32 *prevXY, int depth, u8 *scratch)
{
	struct GameTracker *gGT = sdata->gGT;
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.curr;
	u32 *nextPrim = prim + 0xd;

	if ((u32 *)backBuffer->primMem.endMin100 < nextPrim)
		return;

	backBuffer->primMem.curr = nextPrim;

	prim[1] = *(u32 *)(scratch + 0x1c);
	prim[4] = *(u32 *)(scratch + 0x1c);
	prim[7] = *(u32 *)(scratch + 0x20);
	prim[10] = *(u32 *)(scratch + 0x20);

	prim[2] = currXY[0];
	prim[5] = currXY[1];
	prim[8] = prevXY[0];
	prim[11] = prevXY[1];

	struct Icon *icon = gGT->ptrIcons[0x2f];
	prim[3] = *(u32 *)&icon->texLayout.u0;

	u32 tpage = *(u32 *)&icon->texLayout.u1;
	if ((*(u32 *)(scratch + 0x24) & 1) != 0)
		tpage = (tpage & 0xff9fffff) | 0x00600000;
	else
		tpage = (tpage & 0xff9fffff) | 0x00400000;
	prim[6] = tpage;

	VehGroundSkids_WriteLo16(&prim[9], *(u16 *)&icon->texLayout.u2);
	VehGroundSkids_WriteLo16(&prim[12], *(u16 *)&icon->texLayout.u3);

	struct PushBuffer *pb = *(struct PushBuffer **)(scratch + 0x18);
	u_long *ot = pb->ptrOT + ((s32)depth >> 6);
	prim[0] = (u32)*ot | 0x0c000000;
	*ot = (u_long)VehGroundSkids_Ptr24(prim);
}

#define VEH_GROUND_SKIDS_SCRATCH_X 0xb8
#define VEH_GROUND_SKIDS_SCRATCH_Y 0xbc
#define VEH_GROUND_SKIDS_SCRATCH_Z 0xc0

static s16 VehGroundSkids_ScaleRelative(u16 value, u16 origin)
{
	// NOTE(aalhendi): Retail uses lhu/subu/sll/sh, so preserve unsigned halfword wraparound.
	return (s16)(u16)(((u32)value - (u32)origin) << 2);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c278-0x8005c354.
void VehGroundSkids_Subset2(SVECTOR *scratch, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3)
{
	u8 *scratchBytes = (u8 *)scratch;
	u16 originX = *(u16 *)(scratchBytes + VEH_GROUND_SKIDS_SCRATCH_X);
	u16 originY = *(u16 *)(scratchBytes + VEH_GROUND_SKIDS_SCRATCH_Y);
	u16 originZ = *(u16 *)(scratchBytes + VEH_GROUND_SKIDS_SCRATCH_Z);

	scratch[0].vx = VehGroundSkids_ScaleRelative((u16)v1->vx, originX);
	scratch[0].vy = VehGroundSkids_ScaleRelative((u16)v1->vy, originY);
	scratch[0].vz = VehGroundSkids_ScaleRelative((u16)v1->vz, originZ);

	scratch[1].vx = VehGroundSkids_ScaleRelative((u16)v2->vx, originX);
	scratch[1].vy = VehGroundSkids_ScaleRelative((u16)v2->vy, originY);
	scratch[1].vz = VehGroundSkids_ScaleRelative((u16)v2->vz, originZ);

	scratch[2].vx = VehGroundSkids_ScaleRelative((u16)v3->vx, originX);
	scratch[2].vy = VehGroundSkids_ScaleRelative((u16)v3->vy, originY);
	scratch[2].vz = VehGroundSkids_ScaleRelative((u16)v3->vz, originZ);
}

#define VEH_GROUND_SKIDS_FRAME_COUNT    8
#define VEH_GROUND_SKIDS_FRAME_SIZE     0x40
#define VEH_GROUND_SKIDS_SCRATCH_CURR   0x28
#define VEH_GROUND_SKIDS_SCRATCH_PREV   0x4c
#define VEH_GROUND_SKIDS_SCRATCH_DEPTH0 0x70
#define VEH_GROUND_SKIDS_SCRATCH_DEPTH1 0x94
#define VEH_GROUND_SKIDS_SCRATCH_ORIGIN 0xb8

static u32 VehGroundSkids_ColorWord(int value)
{
	return 0x3e000000 | ((u32)value << 16) | ((u32)value << 8) | (u32)value;
}

static s32 VehGroundSkids_Abs(s32 value)
{
	return value < 0 ? -value : value;
}

static int VehGroundSkids_InitPoint(SVECTOR *scratch, const SVECTOR *point, const s32 *origin)
{
	// NOTE(aalhendi): Retail uses lh/lw/subu/sll here; preserve unsigned 32-bit wraparound.
	s32 x = (s32)(((u32)(s32)point->vx - (u32)origin[0]) << 2);
	if (VehGroundSkids_Abs(x) >= 0x1771)
		return 0;

	s32 y = (s32)(((u32)(s32)point->vy - (u32)origin[1]) << 2);
	if (VehGroundSkids_Abs(y) >= 0x1771)
		return 0;

	s32 z = (s32)(((u32)(s32)point->vz - (u32)origin[2]) << 2);
	if (VehGroundSkids_Abs(z) >= 0x1771)
		return 0;

	scratch[0].vx = x;
	scratch[0].vy = y;
	scratch[0].vz = z;
	return 1;
}

static int VehGroundSkids_IntensityFromDepth(int depth)
{
	depth >>= 2;
	if (depth < 0x180)
		return 0x7f;

	MTC2(depth - 0x180, 30);
	int shift = 0x1a - MFC2(31);
	if (shift < 0)
		shift = 0;

	int intensity = 0x7f >> (shift & 0x1f);
	if (intensity < 0x10)
		return -1;

	return intensity;
}

static void VehGroundSkids_ProjectFrame(SVECTOR *scratch, SVECTOR *frame, u32 *sxy, s32 *depth)
{
	VehGroundSkids_Subset2(scratch, &frame[0], &frame[1], &frame[2]);
	gte_ldv0(&scratch[0]);
	gte_ldv1(&scratch[1]);
	gte_ldv2(&scratch[2]);
	gte_rtpt();
	gte_stsxy3(&sxy[0], &sxy[1], &sxy[2]);
	gte_stsz3(&depth[0], &depth[1], &depth[2]);

	VehGroundSkids_Subset2(scratch, &frame[3], &frame[4], &frame[5]);
	gte_ldv0(&scratch[0]);
	gte_ldv1(&scratch[1]);
	gte_ldv2(&scratch[2]);
	gte_rtpt();
	gte_stsxy3(&sxy[3], &sxy[4], &sxy[5]);
	gte_stsz3(&depth[3], &depth[4], &depth[5]);

	VehGroundSkids_Subset2(scratch, &frame[6], &frame[7], &frame[0]);
	gte_ldv0(&scratch[0]);
	gte_ldv1(&scratch[1]);
	gte_ldv2(&scratch[2]);
	gte_rtpt();
	gte_stsxy3(&sxy[6], &sxy[7], &sxy[8]);
	gte_stsz3(&depth[6], &depth[7], &depth[8]);
}

static void VehGroundSkids_TryEmitSegment(u32 *currXY, u32 *prevXY, s32 *currDepth, s32 *prevDepth, u32 flags, u32 prevFlags, int bit, u8 *frameBase,
                                          int pointIndex, u8 *scratchBytes)
{
	if ((flags & prevFlags & bit) == 0)
		return;
	if (currDepth[pointIndex] <= 0x20 || currDepth[pointIndex + 1] <= 0x20)
		return;
	if (prevDepth[pointIndex] <= 0x20 || prevDepth[pointIndex + 1] <= 0x20)
		return;

	scratchBytes[0x24] = frameBase[(pointIndex * sizeof(SVECTOR)) + 7];
	int depth = (currDepth[pointIndex] >> 2) + (frameBase[(pointIndex * sizeof(SVECTOR)) + 6] << 6);
	VehGroundSkids_Subset1(&currXY[pointIndex], &prevXY[pointIndex], depth, scratchBytes);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c354-0x8005ca24.
void VehGroundSkids_Main(struct Thread *thread, struct PushBuffer *pb)
{
	gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);

	u8 *scratchBytes = CTR_SCRATCHPAD_BASE;
	SVECTOR *scratch = CTR_SCRATCHPAD_PTR(SVECTOR, 0x0);
	s32 *origin = CTR_SCRATCHPAD_PTR(s32, VEH_GROUND_SKIDS_SCRATCH_ORIGIN);

	*(struct PushBuffer **)(scratchBytes + 0x18) = pb;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;

	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransVector((VECTOR *)origin);

	origin[0] = pb->matrix_Camera.t[0];
	origin[1] = pb->matrix_Camera.t[1];
	origin[2] = pb->matrix_Camera.t[2];

	while (thread != NULL)
	{
		struct Driver *d = thread->object;
		u32 flags = d->skidmarkEnableFlags;

		if (flags > 0xf)
		{
			int frameIndex = ((u8)d->skidmarkFrameIndex - 1) & 7;
			SVECTOR *frame = (SVECTOR *)(void *)&d->skidmarks[frameIndex * VEH_GROUND_SKIDS_FRAME_SIZE];

			if (VehGroundSkids_InitPoint(scratch, &frame[0], origin))
			{
				gte_ldv0(scratch);
				gte_rtv0();

				int intensity = VehGroundSkids_IntensityFromDepth(MFC2_S(27));
				if (intensity >= 0)
				{
					u32 *currXY = CTR_SCRATCHPAD_PTR(u32, VEH_GROUND_SKIDS_SCRATCH_CURR);
					u32 *prevXY = CTR_SCRATCHPAD_PTR(u32, VEH_GROUND_SKIDS_SCRATCH_PREV);
					s32 *currDepth = CTR_SCRATCHPAD_PTR(s32, VEH_GROUND_SKIDS_SCRATCH_DEPTH0);
					s32 *prevDepth = CTR_SCRATCHPAD_PTR(s32, VEH_GROUND_SKIDS_SCRATCH_DEPTH1);

					*(u32 *)(scratchBytes + 0x1c) = VehGroundSkids_ColorWord(intensity);
					*(u32 *)(scratchBytes + 0x20) = 0xffffffff;

					u32 prevFlags = 0;
					while (flags != 0)
					{
						u32 currFlags = flags;

						if ((currFlags & 0xf) != 0)
						{
							frame = (SVECTOR *)(void *)&d->skidmarks[frameIndex * VEH_GROUND_SKIDS_FRAME_SIZE];
							VehGroundSkids_ProjectFrame(scratch, frame, currXY, currDepth);

							VehGroundSkids_TryEmitSegment(currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 1, (u8 *)frame, 0, scratchBytes);
							VehGroundSkids_TryEmitSegment(currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 2, (u8 *)frame, 2, scratchBytes);
							VehGroundSkids_TryEmitSegment(currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 4, (u8 *)frame, 4, scratchBytes);
							VehGroundSkids_TryEmitSegment(currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 8, (u8 *)frame, 6, scratchBytes);
						}

						u32 *tmpXY = currXY;
						currXY = prevXY;
						prevXY = tmpXY;

						s32 *tmpDepth = currDepth;
						currDepth = prevDepth;
						prevDepth = tmpDepth;

						frameIndex = (frameIndex + 1) & (VEH_GROUND_SKIDS_FRAME_COUNT - 1);

						if (*(u32 *)(scratchBytes + 0x20) == 0xffffffff)
						{
							*(u32 *)(scratchBytes + 0x20) = *(u32 *)(scratchBytes + 0x1c);
							prevFlags = 0xf;
						}
						else
						{
							prevFlags = currFlags;
							flags = currFlags >> 4;

							int faded = (*(u32 *)(scratchBytes + 0x1c) & 0xff) >> 1;
							*(u32 *)(scratchBytes + 0x20) = *(u32 *)(scratchBytes + 0x1c);
							*(u32 *)(scratchBytes + 0x1c) = VehGroundSkids_ColorWord(faded);
						}

						if (*(u32 *)(scratchBytes + 0x20) == 0)
							break;
					}
				}
			}
		}

		thread = thread->siblingThread;
	}
}
