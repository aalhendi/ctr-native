#include <common.h>
#include <ctr_scratchpad.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d094-0x8001d0c4
struct MetaDataMODEL *COLL_LevModelMeta(u32 id)
{
	// use unsigned so -1 is positive
	if (id >= 0xe2)
		id = 0;

	return &data.MetaDataModels[id];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001eb0c-0x8001ebec
void COLL_SearchBSP_CallbackQUADBLK(u32 *posTop, u32 *posBottom, struct ScratchpadStruct *sps, s32 hitRadius)
{
	s32 hitRadiusSquared = hitRadius * hitRadius;

	sps->Input1.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->boolDidTouchHitbox = 0;
	sps->numTrianglesTested = 0;
	sps->boolDidTouchQuadblock = 0;
	sps->collision.stepFlags = 0;
	struct mesh_info *meshInfo = sps->ptr_mesh_info;
	sps->numBspHitboxesHit = 0;

	u32 topXY = posTop[0];
	s16 topZ = *(s16 *)(posTop + 1);

	sps->Input1.pos.x = (s16)topXY;
	sps->Input1.pos.y = (s16)(topXY >> 16);
	sps->Input1.pos.z = topZ;
	sps->Union.QuadBlockColl.hitPos.x = (s16)topXY;
	sps->Union.QuadBlockColl.hitPos.y = (s16)(topXY >> 16);
	sps->Union.QuadBlockColl.hitPos.z = topZ;

	u32 bottomXY = posBottom[0];
	s16 bottomZ = *(s16 *)(posBottom + 1);

	sps->Union.QuadBlockColl.pos.x = (s16)bottomXY;
	sps->Union.QuadBlockColl.pos.y = (s16)(bottomXY >> 16);
	sps->Union.QuadBlockColl.pos.z = bottomZ;

	sps->hitFraction = 0x1000;
	sps->Input1.hitRadiusSquared = hitRadiusSquared;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadiusSquared;

	s16 topX = sps->Input1.pos.x;
	s16 topY = sps->Input1.pos.y;
	topZ = sps->Input1.pos.z;
	s16 bottomX = sps->Union.QuadBlockColl.pos.x;
	s16 bottomY = sps->Union.QuadBlockColl.pos.y;
	bottomZ = sps->Union.QuadBlockColl.pos.z;

	s16 min = bottomX;
	s16 max = topX;
	if ((topX - bottomX) < 0)
	{
		min = topX;
		max = bottomX;
	}
	sps->bbox.min[0] = min;
	sps->bbox.max[0] = max;

	min = bottomY;
	max = topY;
	if ((topY - bottomY) < 0)
	{
		min = topY;
		max = bottomY;
	}
	sps->bbox.min[1] = min;
	sps->bbox.max[1] = max;

	min = bottomZ;
	max = topZ;
	if ((topZ - bottomZ) < 0)
	{
		min = topZ;
		max = bottomZ;
	}
	sps->bbox.min[2] = min;
	sps->bbox.max[2] = max;

	COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
}


internal b32 COLL_SearchBSP_CallbackPARAM_Overlaps(struct BSP *node, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ)
{
	return ((node->box.min[1] <= maxY) && (node->box.min[0] <= maxX) && (minX <= node->box.max[0]) && (node->box.min[2] <= maxZ) &&
	        (minZ <= node->box.max[2]) && (minY <= node->box.max[1]));
}

internal void COLL_SearchBSP_CallbackPARAM_PushChild(struct BSP *root, u16 childId, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ, u16 **stackTop)
{
	if (childId == 0xffff)
		return;

	struct BSP *child = &root[childId & 0x3fff];
	if (!COLL_SearchBSP_CallbackPARAM_Overlaps(child, minX, minY, minZ, maxX, maxY, maxZ))
		return;

	**stackTop = childId;
	(*stackTop)++;
}

internal void COLL_SearchBSP_CallbackPARAM_PushChildren(struct BSP *root, struct BSP *node, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ,
                                                        u16 **stackTop)
{
	// Retail pushes child 0 then child 1; the scratchpad stack pops child 1 first.
	COLL_SearchBSP_CallbackPARAM_PushChild(root, (u16)node->data.branch.childID[0], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
	COLL_SearchBSP_CallbackPARAM_PushChild(root, (u16)node->data.branch.childID[1], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ebec-0x8001ede4
void COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, void (*callback)(struct BSP *, struct ScratchpadStruct *),
                                  struct ScratchpadStruct *param)
{
	if (root == NULL)
		return;

	s16 minX = bbox->min[0];
	s16 minY = bbox->min[1];
	s16 minZ = bbox->min[2];
	s16 maxX = bbox->max[0];
	s16 maxY = bbox->max[1];
	s16 maxZ = bbox->max[2];

	// Retail stores pending child IDs at scratchpad 0x1f800070 and pops them
	// LIFO, preserving the original BSP traversal order without host recursion.
	u16 *stackBase = CTR_SCRATCHPAD_PTR(u16, 0x70);
	u16 *stackTop = stackBase;

	COLL_SearchBSP_CallbackPARAM_PushChildren(root, root, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);

	while (stackTop != stackBase)
	{
		stackTop--;
		u16 childId = *stackTop;
		struct BSP *child = &root[childId & 0x3fff];

		if ((childId & 0x4000) != 0)
		{
			callback(child, param);
			continue;
		}

		COLL_SearchBSP_CallbackPARAM_PushChildren(root, child, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);
	}
}


internal u32 CollFixed_PackS16Pair(s32 lo, s32 hi)
{
	return (u16)lo | ((u32)(u16)hi << 16);
}

internal s32 Coll_MipsAbsS32(s32 value)
{
	return (value < 0) ? CTR_MipsNegLo(value) : value;
}

internal void CollFixed_GteLoadR11R12(u32 value)
{
	CTC2(value, 0);
}

internal void CollFixed_GteLoadR13R21(u32 value)
{
	CTC2(value, 1);
}

internal void CollFixed_GteLoadR22R23(u32 value)
{
	CTC2(value, 2);
}

internal void CollFixed_GteLoadR33(u32 value)
{
	CTC2(value, 4);
}

internal void CollFixed_GteLoadVXY0(u32 value)
{
	MTC2(value, 0);
}

internal void CollFixed_GteLoadVZ0(s32 value)
{
	MTC2_S(value, 1);
}

internal void CollFixed_GteLoadIR0(s32 value)
{
	MTC2_S(value, 8);
}

internal void CollFixed_GteLoadIR(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

internal void CollFixed_GteLoadMAC(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 25);
	MTC2_S(y, 26);
	MTC2_S(z, 27);
}

internal s32 CollFixed_GteReadMAC1(void)
{
	return MFC2_S(25);
}

internal s32 CollFixed_GteReadMAC2(void)
{
	return MFC2_S(26);
}

internal s32 CollFixed_GteReadMAC3(void)
{
	return MFC2_S(27);
}

internal void CollFixed_GteLoadLZCS(s32 value)
{
	MTC2_S(value, 30);
}

internal s32 CollFixed_GteReadLZCR(void)
{
	return MFC2_S(31);
}

internal void CollFixed_GteMVMVA(void)
{
	doCOP2(0x0406012);
}

internal void CollFixed_GteGPL12(void)
{
	doCOP2(0x01a8003e);
}

internal void CollFixed_GteOP0(void)
{
	doCOP2(0x0170000c);
}

internal void CollFixed_GteRTIR(void)
{
	doCOP2(0x049e012);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ede4-0x8001ef1c
void COLL_FIXED_TRIANGL_Barycentrics(s16 *out, s16 *v1, s16 *v2, s16 *point)
{
	s32 v1x = v1[0];
	s32 v1y = v1[1];
	s32 v1z = v1[2];
	s32 edgeX = v2[0] - v1x;
	s32 edgeY = v2[1] - v1y;
	s32 edgeZ = v2[2] - v1z;
	s32 pointX = point[0] - v1x;
	s32 pointY = point[1] - v1y;
	s32 pointZ = point[2] - v1z;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(edgeZ, pointX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(pointY, pointZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadVZ0(edgeZ);

	CollFixed_GteMVMVA();
	s32 pointDot = CollFixed_GteReadMAC2();
	s32 edgeDot = CollFixed_GteReadMAC1();

	CollFixed_GteLoadLZCS(pointDot);
	s32 shift = CollFixed_GteReadLZCR() - 2;

	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	pointDot = CTR_MipsSll(pointDot, shift);

	if (shift < 12)
	{
		edgeDot >>= 12 - shift;
	}

	s32 factor = 0;

	if (edgeDot != 0)
	{
		factor = pointDot / edgeDot;

		if (factor < 0)
		{
			factor = 0;
		}
		else if (factor > 0x1000)
		{
			factor = 0x1000;
		}
	}

	CollFixed_GteLoadIR0(factor);
	CollFixed_GteLoadMAC(v1x, v1y, v1z);
	CollFixed_GteLoadIR(edgeX, edgeY, edgeZ);
	CollFixed_GteGPL12();

	out[0] = (s16)CollFixed_GteReadMAC1();
	out[1] = (s16)CollFixed_GteReadMAC2();
	out[2] = (s16)CollFixed_GteReadMAC3();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d0c4-0x8001d610
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node)
{
	struct CollInstanceHitboxScratch *scratch = &sps->collision.instanceHitbox;
	s32 diffX;
	s32 diffY;
	s32 diffZ;
	s32 centerDiffX;
	s32 centerDiffY;
	s32 centerDiffZ;
	s32 dotSegment;
	s32 dotCenter;
	s32 shift;
	s32 divisor;
	s32 factor;
	s32 projX;
	s32 projY;
	s32 projZ;
	s32 relX;
	s32 relY;
	s32 relZ;
	s32 radius;
	s32 radiusSquared;
	s32 distSquared;
	s32 remaining;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	s32 len;
	s32 invLen;
	s32 scaledX;
	s32 scaledY;
	s32 scaledZ;

	if ((node->flag >> 8) == BSP_HITBOX_CLASS_TOUCH)
	{
		sps->bspHitbox = node;
		sps->boolDidTouchHitbox = (s16)((u16)sps->boolDidTouchHitbox + 1);
	}

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_FORCE_INSTANCE_HIT) != 0)
	{
		CTR_SET_VEC3(sps->hit.plane.normal.v, 0, 0x1000, 0);
		sps->hit.reorderResult = 6;
		sps->hitFraction = 0;
		sps->bspHitbox = node;
		sps->Union.QuadBlockColl.hitPos = sps->Union.QuadBlockColl.pos;
		sps->boolDidTouchHitbox = (s16)((u16)sps->boolDidTouchHitbox + 1);
		return 6;
	}

	scratch->segmentDelta[1] = 0;

	diffX = sps->Input1.pos.x - sps->Union.QuadBlockColl.pos.x;
	diffY = 0;
	diffZ = sps->Input1.pos.z - sps->Union.QuadBlockColl.pos.z;
	scratch->segmentDelta[0] = diffX;
	scratch->segmentDelta[2] = diffZ;

	centerDiffX = node->data.hitbox.center.x - sps->Union.QuadBlockColl.pos.x;
	centerDiffY = 0;
	centerDiffZ = node->data.hitbox.center.z - sps->Union.QuadBlockColl.pos.z;
	scratch->centerDelta[1] = 0;
	scratch->centerDelta[0] = centerDiffX;
	scratch->centerDelta[2] = centerDiffZ;

	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		diffY = sps->Input1.pos.y - sps->Union.QuadBlockColl.pos.y;
		scratch->segmentDelta[1] = diffY;
		centerDiffY = node->data.hitbox.center.y - sps->Union.QuadBlockColl.pos.y;
		scratch->centerDelta[1] = centerDiffY;
	}

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(diffZ, centerDiffX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(centerDiffY, centerDiffZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadVZ0(diffZ);
	CollFixed_GteMVMVA();

	dotSegment = CollFixed_GteReadMAC1();
	dotCenter = CollFixed_GteReadMAC2();
	scratch->segmentDot = dotSegment;
	scratch->centerDot = dotCenter;

	if (dotCenter <= 0)
		return 0;

	CollFixed_GteLoadLZCS(dotCenter);
	shift = CollFixed_GteReadLZCR() - 2;
	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	divisor = dotSegment >> (12 - shift);
	dotCenter = CTR_MipsSll(dotCenter, shift);

	if (divisor < 0)
		return 0;

	factor = 0;
	if (divisor != 0)
	{
		factor = dotCenter / divisor;
	}
	scratch->lineFactor = factor;

	projX = CTR_MipsMulLo(factor, diffX) >> 12;
	projY = 0;
	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		projY = CTR_MipsMulLo(factor, diffY) >> 12;
	}
	projZ = CTR_MipsMulLo(factor, diffZ) >> 12;

	relX = projX - centerDiffX;
	relY = projY - centerDiffY;
	relZ = projZ - centerDiffZ;
	scratch->projectedDelta[0] = projX;
	scratch->projectedDelta[1] = projY;
	scratch->projectedDelta[2] = projZ;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadR13R21(relZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadVZ0(relZ);
	CollFixed_GteMVMVA();

	radius = node->data.hitbox.radius;
	radiusSquared = sps->Input1.hitRadius + radius;
	radiusSquared = CTR_MipsMulLo(radiusSquared, radiusSquared);
	distSquared = CollFixed_GteReadMAC1();
	scratch->radiusSquared = radiusSquared;
	scratch->distanceSquared = distSquared;

	remaining = radiusSquared - distSquared;
	if (remaining < 0)
		return 0;

	if (remaining != 0)
	{
		if (dotSegment != 0)
		{
			factor -= CTR_MipsSll(remaining, 12) / dotSegment;
		}
		scratch->adjustedFactor = factor;
	}

	if (sps->hitFraction < factor)
		return 0;

	hitX = 0;
	hitY = 0;
	hitZ = 0;
	if (factor > 0)
	{
		hitX = CTR_MipsMulLo(diffX, factor) >> 12;
		hitY = CTR_MipsMulLo(diffY, factor) >> 12;
		hitZ = CTR_MipsMulLo(diffZ, factor) >> 12;
	}

	if ((node->flag & BSP_HITBOX_CHECK_Y_RANGE) != 0)
	{
		s32 centerY = node->data.hitbox.center.y;
		if ((hitY < centerY) && ((centerY + node->id) < hitY))
			return 0;
	}

	sps->bspHitbox = node;
	sps->hitFraction = factor;
	sps->boolDidTouchHitbox = (s16)((u16)sps->boolDidTouchHitbox + 1);
	scratch->hitDelta[0] = hitX;
	scratch->hitDelta[1] = hitY;
	scratch->hitDelta[2] = hitZ;

	normalX = hitX - centerDiffX;
	normalY = 0;
	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		normalY = hitY - centerDiffY;
	}
	normalZ = hitZ - centerDiffZ;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadR13R21(normalZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadVZ0(normalZ);
	CollFixed_GteMVMVA();

	len = SquareRoot0(CollFixed_GteReadMAC1());
	invLen = 0x1000000 / len;

	normalX = CTR_MipsMulLo(normalX, invLen) >> 12;
	normalY = CTR_MipsMulLo(normalY, invLen) >> 12;
	normalZ = CTR_MipsMulLo(normalZ, invLen) >> 12;
	scratch->normal[0] = normalX;
	scratch->normal[1] = normalY;
	scratch->normal[2] = normalZ;

	sps->Union.QuadBlockColl.hitPos.x = (s16)((u16)sps->Union.QuadBlockColl.pos.x + hitX);
	CTR_SET_VEC3(sps->hit.plane.normal.v, (s16)normalX, (s16)normalY, (s16)normalZ);
	sps->Union.QuadBlockColl.hitPos.z = (s16)((u16)sps->Union.QuadBlockColl.pos.z + hitZ);
	sps->Union.QuadBlockColl.hitPos.y = (s16)((u16)sps->Union.QuadBlockColl.pos.y + hitY);
	sps->hit.reorderResult = 6;

	scaledX = CTR_MipsMulLo(normalX, radius) >> 12;
	scaledY = CTR_MipsMulLo(normalY, radius) >> 12;
	scaledZ = CTR_MipsMulLo(normalZ, radius) >> 12;
	scratch->scaledNormal[0] = scaledX;
	scratch->scaledNormal[1] = scaledY;
	scratch->scaledNormal[2] = scaledZ;

	sps->hit.pushOut.x = (s16)((u16)node->data.hitbox.center.x + scaledX);
	sps->hit.pushOut.y = (s16)((u16)node->data.hitbox.center.y + scaledY);
	sps->hit.pushOut.z = (s16)((u16)node->data.hitbox.center.z + scaledZ);
	sps->hit.hitPos = sps->hit.pushOut;

	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d610-0x8001d77c
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps)
{
	struct BSP *bspArray = node->data.leaf.bspHitboxArray;

	if (bspArray == NULL)
		return;

	// check every instance hitbox until
	// end of list (null flag) is found
	for (/**/; bspArray->flag != 0; bspArray++)
	{
		struct BoundingBox *bbox = &bspArray->box;

		// 1F8001CC
		s32 arraySize = sps->numBspHitboxesHit - 1;
		for (; arraySize >= 0; arraySize--)
		{
			if (bspArray == sps->bspHitboxesHit[arraySize])
				goto NextBSP;
		}

		if ((
		        // if hitbox data is not tied to an active visible instance
		        (
		            // if collision for instance is disabled
		            ((bspArray->flag & BSP_HITBOX_COLLIDABLE) == 0) ||
		            // if bspHitbox.InstDef doesn't exist
		            (bspArray->data.hitbox.instDef == NULL))

		        ||

		        // if data is valid

		        // allows drawing flag is enabled
		        ((bspArray->data.hitbox.instDef->ptrInstance->flags & 0xf) != 0)) &&

		    // compare bsp boundingbox to scratchpad boundingbox
		    ((sps->bbox.min[0] <= bbox->max[0]) &&

		     (bbox->min[0] <= sps->bbox.max[0]) &&

		     (sps->bbox.min[1] <= bbox->max[1]) &&

		     (bbox->min[1] <= sps->bbox.max[1]) &&

		     (sps->bbox.min[2] <= bbox->max[2]) &&

		     (bbox->min[2] <= sps->bbox.max[2])))
		{
			// check with collision for this instance
			COLL_FIXED_INSTANC_TestPoint(sps, bspArray);
		}

	NextBSP:
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d77c-0x8001d944
void COLL_FIXED_BotsSearch(s16 *posCurr, s16 *posPrev, struct ScratchpadStruct *sps)
{
	s16 radius = sps->Input1.hitRadius;
	s32 sqrRadius = CTR_MipsMulLo(radius, radius);

	sps->Input1.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadius = radius;

	for (s32 axis = 0; axis < 3; axis++)
	{
		sps->Input1.pos.v[axis] = posCurr[axis];
		sps->Union.QuadBlockColl.hitPos.v[axis] = posCurr[axis];
		sps->Union.QuadBlockColl.pos.v[axis] = posPrev[axis];

		s16 deltaCurr = posCurr[axis] - radius;
		s16 deltaPrev = posPrev[axis] - radius;
		sps->bbox.min[axis] = (deltaCurr < deltaPrev) ? deltaCurr : deltaPrev;

		deltaCurr = posCurr[axis] + radius;
		deltaPrev = posPrev[axis] + radius;
		sps->bbox.max[axis] = (deltaCurr > deltaPrev) ? deltaCurr : deltaPrev;
	}

	sps->numTrianglesTested = 0;
	sps->boolDidTouchHitbox = 0;

	sps->hitFraction = 0x1000;
	sps->numBspHitboxesHit = 0;
	sps->collision.stepFlags = 0;

	COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestInstance, sps);
}


internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef1c-0x8001ef50
void COLL_FIXED_TRIANGL_UNUSED(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	// NOTE(aalhendi): Retail skips the TestPoint setup and jumps into the
	// shared body with t2 preloaded from sps+0x58. Native makes a0-a3 and t2
	// explicit.
	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, (s32)CollFixed_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance));
}


internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW)
{
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;

	s32 startX = sps->Union.QuadBlockColl.pos.x;
	s32 startY = sps->Union.QuadBlockColl.pos.y;
	s32 startZ = sps->Union.QuadBlockColl.pos.z;
	s32 deltaX = sps->Union.QuadBlockColl.hitPos.x - startX;
	s32 deltaY = sps->Union.QuadBlockColl.hitPos.y - startY;
	s32 deltaZ = sps->Union.QuadBlockColl.hitPos.z - startZ;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(startX, startY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(startZ, deltaX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(deltaY, deltaZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);

	normalZW = CTR_MipsSll(normalZW >> 16, 13);

	CollFixed_GteMVMVA();
	s32 lineDot = CollFixed_GteReadMAC2();
	s32 planeDot = CollFixed_GteReadMAC1() - normalZW;

	if (lineDot >= 0)
		return;

	s32 factor = -planeDot / (lineDot >> 12);

	CollFixed_GteLoadMAC(startX, startY, startZ);
	CollFixed_GteLoadIR(deltaX, deltaY, deltaZ);
	CollFixed_GteLoadIR0(factor);

	if ((factor < 0) || (factor > 0x1000))
		return;

	CollFixed_GteGPL12();
	s32 hitX = CollFixed_GteReadMAC1();
	s32 hitY = CollFixed_GteReadMAC2();
	s32 hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(sps->candidate.hitPos.v, (s16)hitX, (s16)hitY, (s16)hitZ);

	struct BspSearchVertex *baryV2 = v2;
	struct BspSearchVertex *baryV3 = v3;
	CollNormalAxis normalAxis = sps->candidate.normalAxis;

	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		s32 origin = v1->pos.z;
		firstA = v2->pos.z - origin;
		firstB = v3->pos.z - origin;
		firstHit = hitZ - origin;

		if (Coll_MipsAbsS32(firstA) < Coll_MipsAbsS32(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = v1->pos.x;
		secondA = baryV2->pos.x - origin;
		secondB = baryV3->pos.x - origin;
		secondHit = hitX - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		s32 origin = v1->pos.x;
		firstA = v2->pos.x - origin;
		firstB = v3->pos.x - origin;
		firstHit = hitX - origin;

		if (Coll_MipsAbsS32(firstA) < Coll_MipsAbsS32(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = v1->pos.y;
		secondA = baryV2->pos.y - origin;
		secondB = baryV3->pos.y - origin;
		secondHit = hitY - origin;
	}
	else
	{
		s32 origin = v1->pos.y;
		firstA = v2->pos.y - origin;
		firstB = v3->pos.y - origin;
		firstHit = hitY - origin;

		if (Coll_MipsAbsS32(firstA) < Coll_MipsAbsS32(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = v1->pos.z;
		secondA = baryV2->pos.z - origin;
		secondB = baryV3->pos.z - origin;
		secondHit = hitZ - origin;
	}

	s32 baryA = -0x1000;
	s32 baryB = -0x1000;

	if (firstA != 0)
	{
		s32 denom = (CTR_MipsMulLo(secondB, firstA) - CTR_MipsMulLo(firstB, secondA)) >> 6;

		if (denom != 0)
		{
			baryB = CTR_MipsMulLo(CTR_MipsMulLo(secondHit, firstA) - CTR_MipsMulLo(firstHit, secondA), 0x40) / denom;

			if ((baryB >= 0) && (baryB <= 0x1000))
			{
				baryA = (CTR_MipsSll(firstHit, 12) - CTR_MipsMulLo(baryB, firstB)) / firstA;
			}
		}
	}
	else
	{
		if (firstB == 0)
			return;

		baryB = CTR_MipsSll(firstHit, 12) / firstB;

		if ((baryB >= 0) && (baryB <= 0x1000))
		{
			baryA = (CTR_MipsSll(secondHit, 12) - CTR_MipsMulLo(baryB, secondB)) / secondA;
		}
	}

	struct QuadBlock *quad = sps->candidate.ptrQuadblock;

	if ((baryA < 0) || ((baryA + baryB - 0x1000) > 0))
		return;

	if ((quad->quadFlags & 0x40) != 0)
	{
		sps->collision.stepFlags |= (u8)quad->terrain_type;
		return;
	}

	sps->hit.ptrQuadblock = quad;
	sps->barycentrics[0] = (s16)baryA;
	sps->barycentrics[1] = (s16)baryB;
	sps->levVertHit[0] = v1->pLevelVertex;
	sps->levVertHit[1] = baryV2->pLevelVertex;
	sps->levVertHit[2] = baryV3->pLevelVertex;
	sps->boolDidTouchQuadblock = (s16)((u16)sps->boolDidTouchQuadblock + 1);
	sps->hit.hitPos = sps->candidate.hitPos;
	sps->Union.QuadBlockColl.hitPos = sps->candidate.hitPos;
	sps->hit.plane = sps->candidate.plane;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef50-0x8001f2dc
void COLL_FIXED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 normalZW = (s32)CollFixed_PackS16Pair(v1->plane.normal.z, v1->plane.halfDistance);

	sps->numTrianglesTested = (s16)((u16)sps->numTrianglesTested + 1);
	sps->candidate.normalAxis = (CollNormalAxis)v1->flags;
	sps->candidate.plane = v1->plane;

	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, normalZW);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f2dc-0x8001f41c
void COLL_FIXED_TRIANGL_GetNormVec(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 v1x = v1->pos.x;
	s32 v1y = v1->pos.y;
	s32 v1z = v1->pos.z;
	s32 edgeAX = v3->pos.x - v1x;
	s32 edgeAY = v3->pos.y - v1y;
	s32 edgeAZ = v3->pos.z - v1z;
	s32 edgeBX = v2->pos.x - v1x;
	s32 edgeBY = v2->pos.y - v1y;
	s32 edgeBZ = v2->pos.z - v1z;

	CollFixed_GteLoadR11R12((u16)edgeAX);
	CollFixed_GteLoadR22R23((u16)edgeAY);
	CollFixed_GteLoadR33((u16)edgeAZ);
	CollFixed_GteLoadIR(edgeBX, edgeBY, edgeBZ);

	u32 lodShift = sps->collision.triNormalLodShift;
	u32 normalShift = sps->collision.triNormalVecBitShift;
	s32 scale = sps->collision.triNormalVecDividend;

	CollFixed_GteOP0();

	s32 nx = CollFixed_GteReadMAC1();
	s32 ny = CollFixed_GteReadMAC2();
	s32 nz = CollFixed_GteReadMAC3();

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(v1x, v1y));
	CollFixed_GteLoadR13R21((u16)v1z);

	nx = CTR_MipsMulLo(nx >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);
	ny = CTR_MipsMulLo(ny >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);
	nz = CTR_MipsMulLo(nz >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);

	CollFixed_GteLoadIR(nx, ny, nz);
	v1->plane.normal.x = (s16)nx;
	v1->plane.normal.y = (s16)ny;

	CollFixed_GteRTIR();
	s32 plane = CollFixed_GteReadMAC1();

	v1->plane.normal.z = (s16)nz;
	v1->plane.halfDistance = (s16)(plane >> 1);

	s32 absX = Coll_MipsAbsS32(nx);
	s32 absY = Coll_MipsAbsS32(ny);
	s32 absZ = Coll_MipsAbsS32(nz);
	CollNormalAxis dominantAxis = COLL_NORMAL_AXIS_Z;

	if ((absX - absY) < 0)
	{
		if ((absY - absZ) >= 0)
		{
			dominantAxis = COLL_NORMAL_AXIS_Y;
		}
	}
	else if ((absX - absZ) >= 0)
	{
		dominantAxis = COLL_NORMAL_AXIS_X;
	}

	v1->flags = (u16)dominantAxis;
}


global_variable struct LevVertex *sCollFixedLoadScratchpadVertsVertexArray;
global_variable struct QuadBlock *sCollFixedLoadScratchpadVertsQuad;

internal void COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	// NOTE(aalhendi): Retail passes these through implicit MIPS registers t8/t9.
	// Native records that register state explicitly before calling the loader.
	sCollFixedLoadScratchpadVertsVertexArray = sps->ptr_mesh_info->ptrVertexArray;
	sCollFixedLoadScratchpadVertsQuad = quad;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f7f0-0x8001f928
void COLL_FIXED_QUADBLK_LoadScratchpadVerts(struct ScratchpadStruct *sps)
{
	struct LevVertex *ptrVert = sCollFixedLoadScratchpadVertsVertexArray;
	struct QuadBlock *ptrQuad = sCollFixedLoadScratchpadVertsQuad;
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->quadIndex2 = ptrQuad->index[2];
	sps->quadIndex3 = ptrQuad->index[3];

	for (u16 *index = (u16 *)&ptrQuad->index[0]; index < (u16 *)&ptrQuad->index[9]; index++, bsv++)
	{
		struct LevVertex *vertCurr = &ptrVert[*index];
		bsv->pLevelVertex = vertCurr;
		*(s32 *)&bsv->pos.v[0] = *(s32 *)&vertCurr->pos[0];
		*(s32 *)&bsv->pos.v[2] = *(s32 *)&vertCurr->pos[2];
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f67c-0x8001f6f0
void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 2 for low poly (big block)
	sps->collision.triNormalLodShift = 2;

	sps->collision.triNormalVecBitShift = quad->triNormalVecBitShift;

	s16 *QBL_TNVD = &quad->triNormalVecDividend[0];

	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	if ((u16)sps->quadIndex2 != (u16)sps->quadIndex3)
	{
		sps->collision.triNormalVecDividend = QBL_TNVD[9];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
	}
	sps->collision.triNormalVecDividend = QBL_TNVD[8];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f6f0-0x8001f7f0
void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 0 for high poly (small block)
	sps->collision.triNormalLodShift = 0;

	sps->collision.triNormalVecBitShift = quad->triNormalVecBitShift;

	s16 *QBL_TNVD = &quad->triNormalVecDividend[0];

	// calculate normal vectors for eight triangles,
	// no collision detection here
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	if ((u16)sps->quadIndex2 != (u16)sps->quadIndex3)
	{
		sps->collision.triNormalVecDividend = QBL_TNVD[4];             // triangle 4
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7

		sps->collision.triNormalVecDividend = QBL_TNVD[5];             // triangle 5
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8

		sps->collision.triNormalVecDividend = QBL_TNVD[6];             // triangle 6
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6

		sps->collision.triNormalVecDividend = QBL_TNVD[7];             // triangle 7
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
	}

	sps->collision.triNormalVecDividend = QBL_TNVD[0];             // triangle 0
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5

	sps->collision.triNormalVecDividend = QBL_TNVD[1];             // triangle 1
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5

	sps->collision.triNormalVecDividend = QBL_TNVD[2];             // triangle 2
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1

	sps->collision.triNormalVecDividend = QBL_TNVD[3];             // triangle 3
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f41c-0x8001f5f0
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->candidate.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min[0] > sps->bbox.max[0]) || (quad->bbox.min[1] > sps->bbox.max[1]) || (quad->bbox.min[2] > sps->bbox.max[2]) ||
	    (sps->bbox.min[0] > quad->bbox.max[0]) || (sps->bbox.min[1] > quad->bbox.max[1]) || (sps->bbox.min[2] > quad->bbox.max[2]))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_HIGH_LOD) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0,1,2

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1,3,2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_REUSE_NORMALS) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f5f0-0x8001f67c
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	// if bsp flag is water
	if ((node->flag & 2) != 0)
	{
		sps->collision.stepFlags |= 0x8000;
	}

	s32 numQuads = node->data.leaf.numQuads;
	struct QuadBlock *ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_FIXED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_TEST_INSTANCES) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}


struct CollFixedPlayerTrig
{
	s32 x;
	s32 z;
};

internal s32 COLL_FIXED_PlayerSearch_ClampByte(s32 value)
{
	if (value < 0)
		return 0;

	if (value > 0xff)
		return 0xff;

	return value;
}

internal struct CollFixedPlayerTrig COLL_FIXED_PlayerSearch_Trig(s32 angle)
{
	struct TrigTable trig = data.trigApprox[angle & 0x3ff];
	struct CollFixedPlayerTrig out;

	if ((angle & 0x400) == 0)
	{
		out.x = trig.sin;
		out.z = trig.cos;

		if ((angle & 0x800) != 0)
		{
			out.x = -out.x;
			out.z = -out.z;
		}
	}
	else
	{
		out.x = trig.cos;
		out.z = -trig.sin;

		if ((angle & 0x800) != 0)
		{
			out.x = -out.x;
			out.z = -out.z;
		}
	}

	return out;
}

internal u32 COLL_FIXED_PlayerSearch_CompressNormal(s16 normalX, s16 normalY, s16 normalZ, u8 driverID)
{
	return (((u16)normalX >> 6) & 0xff) | ((((u16)normalY >> 6) & 0xff) << 8) | ((((u16)normalZ >> 6) & 0xff) << 16) | ((driverID + 1) << 24);
}

internal void COLL_FIXED_PlayerSearch_SetupSearch(struct ScratchpadStruct *sps, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 topX = d->posCurr.x >> 8;
	s16 topY = (d->posCurr.y >> 8) + 0x80;
	s16 topZ = d->posCurr.z >> 8;
	s16 bottomY = (d->posCurr.y >> 8) - 0x100;

	d->actionsFlagSet &= 0xfffeffff;

	sps->Union.QuadBlockColl.pos.x = topX;
	sps->Union.QuadBlockColl.pos.y = topY;
	sps->Union.QuadBlockColl.pos.z = topZ;

	sps->Input1.pos.x = topX;
	sps->Input1.pos.y = bottomY;
	sps->Input1.pos.z = topZ;

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0x10;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;

	sps->Union.QuadBlockColl.searchFlags = 0;
	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	}

	sps->boolDidTouchQuadblock = 0;
	sps->boolDidTouchHitbox = 0;
	sps->numTrianglesTested = 0;

	sps->bbox.min[0] = topX;
	sps->bbox.max[0] = topX;
	sps->bbox.min[1] = (bottomY < topY) ? bottomY : topY;
	sps->bbox.max[1] = (topY < bottomY) ? bottomY : topY;
	sps->bbox.min[2] = topZ;
	sps->bbox.max[2] = topZ;

	sps->Union.QuadBlockColl.hitPos = sps->Input1.pos;
}

internal void COLL_FIXED_PlayerSearch_UpdateLighting(struct ScratchpadStruct *sps, struct Driver *d, struct Instance *inst)
{
	struct LevVertex *v0 = sps->levVertHit[0];
	struct LevVertex *v1 = sps->levVertHit[1];
	struct LevVertex *v2 = sps->levVertHit[2];

	if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
		return;

	s32 baryA = sps->barycentrics[0];
	s32 baryB = sps->barycentrics[1];
	s32 r0 = v0->color_hi[0];
	s32 g0 = v0->color_hi[1];
	s32 b0 = v0->color_hi[2];
	s32 r = (CTR_MipsMulLo(baryA, v1->color_hi[0] - r0) >> 12) + (CTR_MipsMulLo(baryB, v2->color_hi[0] - r0) >> 12) + r0;
	s32 g = (CTR_MipsMulLo(baryA, v1->color_hi[1] - g0) >> 12) + (CTR_MipsMulLo(baryB, v2->color_hi[1] - g0) >> 12) + g0;
	s32 b = (CTR_MipsMulLo(baryA, v1->color_hi[2] - b0) >> 12) + (CTR_MipsMulLo(baryB, v2->color_hi[2] - b0) >> 12) + b0;

	r = COLL_FIXED_PlayerSearch_ClampByte(r);
	g = COLL_FIXED_PlayerSearch_ClampByte(g);
	b = COLL_FIXED_PlayerSearch_ClampByte(b);

	s32 light = CTR_MipsMulLo(((CTR_MipsMulLo(r, 0x4c) >> 8) + (CTR_MipsMulLo(g, 0x96) >> 8) + (CTR_MipsMulLo(b, 0x1e) >> 8)), -0x20) + 0xc00;

	if (light < 0)
	{
		light = 0;
	}

	s32 scaledLight = light << 3;

	if (light > 0x1000)
	{
		light = 0x1000;
		scaledLight = 0x8000;
	}

	light = CTR_MipsMulLo(scaledLight - light, 8);

	d->alphaScaleBackup = (CTR_MipsMulLo(d->alphaScaleBackup, 200) + light) >> 8;
	inst->alphaScale = (CTR_MipsMulLo(inst->alphaScale, 200) + light) >> 8;
}

internal void COLL_FIXED_PlayerSearch_NormalizeAxis3(struct ScratchpadStruct *sps, struct Driver *d)
{
	s32 x = CTR_MipsMulLo(d->AxisAngle3_normalVec[0], 5) + CTR_MipsMulLo(sps->hit.plane.normal.x, 3);
	s32 y = CTR_MipsMulLo(d->AxisAngle3_normalVec[1], 5) + CTR_MipsMulLo(sps->hit.plane.normal.y, 3);
	s32 z = CTR_MipsMulLo(d->AxisAngle3_normalVec[2], 5) + CTR_MipsMulLo(sps->hit.plane.normal.z, 3);
	u32 sum = (u32)CTR_MipsMulLo(x, x) + (u32)CTR_MipsMulLo(y, y) + (u32)CTR_MipsMulLo(z, z);
	u32 len = VehCalc_FastSqrt(sum, 0x18) >> 12;

	d->AxisAngle3_normalVec[0] = CTR_MipsSll(x, 12) / (s32)len;
	d->AxisAngle3_normalVec[1] = CTR_MipsSll(y, 12) / (s32)len;
	d->AxisAngle3_normalVec[2] = CTR_MipsSll(z, 12) / (s32)len;
}

internal void COLL_FIXED_PlayerSearch_NormalizeAxis2(struct Driver *d, s32 x, s32 y, s32 z)
{
	u32 sum = (u32)CTR_MipsMulLo(x, x) + (u32)CTR_MipsMulLo(y, y) + (u32)CTR_MipsMulLo(z, z);
	u32 len = VehCalc_FastSqrt(sum, 0x18) >> 12;

	d->AxisAngle2_normalVec[0] = CTR_MipsSll(x, 12) / (s32)len;
	d->AxisAngle2_normalVec[1] = CTR_MipsSll(y, 12) / (s32)len;
	d->AxisAngle2_normalVec[2] = CTR_MipsSll(z, 12) / (s32)len;
}

internal b32 COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(struct Driver *d, struct QuadBlock *quad)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;

	if ((quad->quadFlags & 0x200) != 0)
		return 1;

	if ((d->kartState == KS_MASK_GRABBED) || ((d->unkAA & 1) != 0) || ((quad->quadFlags & 0x1000) == 0))
		return 0;

	if (quad->checkpointIndex == 0xff)
	{
		if ((u32)(gGT->levelID - GEM_STONE_VALLEY) < 5)
		{
			d->lastValid = quad;
		}
		return 0;
	}

	struct CheckpointNode *node = &level->ptr_restart_points[quad->checkpointIndex];

	if (((d->actionsFlagSet & 0x1000000) == 0) && (node->nextIndex_forward > 1) &&
	    ((((level->ptr_restart_points[0].distToFinish >> 2) << 3) < (s32)(d->distanceToFinish_checkpoint - CTR_MipsMulLo(node->distToFinish, 8)))))
	{
		return 1;
	}

	u16 trackLength = level->ptr_restart_points[0].distToFinish;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail reaches directly through lastValid here. Native
	// can enter this early checkpoint path before spawn/collision has seeded
	// lastValid, and cannot mirror PS1 low-memory null-space reads.
	if (d->lastValid == NULL)
	{
		d->lastValid = d->currBlockTouching;
		return 0;
	}
#endif

	if ((node->distToFinish < (CTR_MipsMulLo(trackLength, 0xf) >> 4)) && (d->lastValid->checkpointIndex != 0xff) &&
	    ((level->ptr_restart_points[d->lastValid->checkpointIndex].distToFinish + (trackLength >> 2)) < node->distToFinish))
	{
		return 1;
	}

	d->lastValid = d->currBlockTouching;
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d944-0x8001eb0c
void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	s32 lerpFrames;

	COLL_FIXED_PlayerSearch_SetupSearch(sps, d);

	if (d->underDriver != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(d->underDriver, sps);
	}

	if ((sps->boolDidTouchQuadblock == 0) && (sps->ptr_mesh_info != NULL) && (sps->ptr_mesh_info->bspRoot != NULL))
	{
		COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
	}

	struct Instance *inst = t->inst;

	if (sps->boolDidTouchQuadblock == 0)
	{
		inst->bitCompressed_NormalVector_AndDriverIndex = ((d->driverID + 1) << 24) | 0x4000;
		inst->flags &= ~REFLECTIVE;
		d->quadBlockHeight = d->posCurr.y - 0x10000;
	}
	else
	{
		struct QuadBlock *quad = sps->hit.ptrQuadblock;
		inst->bitCompressed_NormalVector_AndDriverIndex =
		    COLL_FIXED_PlayerSearch_CompressNormal(sps->hit.plane.normal.x, sps->hit.plane.normal.y, sps->hit.plane.normal.z, d->driverID);
		d->quadBlockHeight = sps->Union.QuadBlockColl.hitPos.y << 8;
		d->unkAA |= 4;

		if ((quad->terrain_type == TERRAIN_MUD) || (quad->terrain_type == TERRAIN_WATER) || (quad->terrain_type == TERRAIN_FASTWATER))
		{
			inst->vertSplit = 0;
			inst->flags |= SPLIT_LINE;
		}

		if (gGT->numPlyrCurrGame < 2)
		{
			u16 quadFlags = quad->quadFlags;

			if ((quadFlags & 0x2000) == 0)
			{
				if ((quadFlags & 1) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = level->splitLines[1];
				}
				else if ((quadFlags & 4) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = level->splitLines[0];
				}
				else
				{
					inst->flags &= ~REFLECTIVE;
				}
			}
		}

		COLL_FIXED_PlayerSearch_NormalizeAxis3(sps, d);

		if ((quad->quadFlags & 0x80) != 0)
		{
			d->actionsFlagSet |= 0x10000;
		}

		d->underDriver = quad;

		if ((d->posCurr.y <= d->quadBlockHeight + 0x1000) || ((quad->terrain_type == TERRAIN_MUD) && (d->posCurr.y < 1)))
		{
			if ((quad->quadFlags & 0x1000) != 0)
			{
				d->normalVecUP = sps->hit.plane.normal;
				d->unkAA |= 8;
			}

			if (d->currBlockTouching == NULL)
			{
				d->currBlockTouching = quad;
				d->AxisAngle1_normalVec = sps->hit.plane.normal;
			}

			COLL_FIXED_PlayerSearch_UpdateLighting(sps, d, inst);
		}
	}

	if (d->quadBlockHeight + 0x8000 < d->posCurr.y)
	{
		d->terrainMeta2 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	}

	if (d->posCurr.y < ((s32)level->ptr_mesh_info->bspRoot->box.min[1] - 0x40) * 0x100)
	{
		d->unkAA |= 1;
	}

	s32 landingDelta = d->velocity.y - d->ySpeed;

	if ((d->currBlockTouching != NULL) && ((d->unkAA & 9) == 0) && (d->kartState != KS_MASK_GRABBED))
	{
		d->velocity.x += d->AxisAngle1_normalVec.x >> 1;
		d->velocity.y += d->AxisAngle1_normalVec.y >> 1;
		d->velocity.z += d->AxisAngle1_normalVec.z >> 1;
	}

	struct QuadBlock *quad = d->currBlockTouching;

	d->xSpeed = d->velocity.x;
	d->ySpeed = d->velocity.y;
	d->zSpeed = d->velocity.z;

	if (quad == NULL)
	{
		goto DriverAirborne;
	}

	if (COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(d, quad) != 0)
	{
		d->unkAA |= 1;
	}

	d->jump_LandingBoost = 0;
	d->actionsFlagSet &= 0xfff7ffbf;

	if ((d->unkAA & 8) == 0)
	{
		goto DriverAirborne;
	}

	if ((d->driverRankItemValue == 2) || ((gGT->gameMode2 & 0x80000) != 0))
	{
		d->currentTerrain = TERRAIN_ICE;
	}
	else
	{
		u8 terrainType = d->currBlockTouching->terrain_type;

		if ((terrainType != TERRAIN_ICE) && (d->currentTerrain == TERRAIN_ICE))
		{
			d->filler_short = 0xfec0;
		}

		d->currentTerrain = terrainType;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(d->currentTerrain);
	d->terrainMeta2 = d->terrainMeta1;
	d->jump_CoyoteTimerMS = 0xa0;

	{
		u32 actions = d->actionsFlagSet;

		d->actionsFlagSet = actions | 1;

		if ((d->actionsFlagSetPrevFrame & 1) == 0)
		{
			s32 absLanding = Coll_MipsAbsS32(landingDelta);

			d->actionsFlagSet = actions | 0x83;
			d->filler_short = 0x140;

			u32 volume = VehCalc_MapToRange(absLanding, 0x100, 0x3c00, 0x78, 0xfa);

			if (d->kartState != KS_MASK_GRABBED)
			{
				volume = (volume & 0xff) << 16;

				if ((d->actionsFlagSet & 0x10000) == 0)
				{
					volume |= 0x8080;
				}
				else
				{
					volume |= 0x1008080;
				}

				OtherFX_Play_LowLevel(7, 1, volume);
			}
		}
	}

	lerpFrames = 6;
	goto BlendNormal;

DriverAirborne:
	if (d->jump_CooldownMS != 0)
	{
		d->actionsFlagSet |= 0x80000;
	}

	if (d->jump_unknown != 0)
	{
		d->actionsFlagSet |= 0x40;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	d->currentTerrain = TERRAIN_NONE;
	d->actionsFlagSet &= ~1u;

	d->jump_LandingBoost += gGT->elapsedTimeMS;
	d->jump_CoyoteTimerMS -= gGT->elapsedTimeMS;
	if (d->jump_CoyoteTimerMS < 0)
	{
		d->jump_CoyoteTimerMS = 0;
	}

	lerpFrames = 7;
	if (d->jump_CoyoteTimerMS == 0)
	{
		d->jump_CooldownMS = 0;
		d->jump_unknown = 0;
	}

BlendNormal:
{
	s32 invFrames = 8 - lerpFrames;
	s32 normalX = (CTR_MipsMulLo(lerpFrames, d->AxisAngle2_normalVec[0]) + CTR_MipsMulLo(invFrames, d->normalVecUP.x)) >> 3;
	s32 normalY = (CTR_MipsMulLo(lerpFrames, d->AxisAngle2_normalVec[1]) + CTR_MipsMulLo(invFrames, d->normalVecUP.y)) >> 3;
	s32 normalZ = (CTR_MipsMulLo(lerpFrames, d->AxisAngle2_normalVec[2]) + CTR_MipsMulLo(invFrames, d->normalVecUP.z)) >> 3;

	if (d->hazardTimer > 0)
	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(CTR_MipsMulLo(d->hazardTimer, 0xc));
		s16 input[4] = {CTR_MipsMulLo(trig.x, 0x19) >> 10, 0, CTR_MipsMulLo(trig.z, 0x19) >> 10, 0};
		s32 output[3];

		gte_ldv0(input);
		gte_rtv0();
		gte_stlvnl(output);

		normalX += output[0];
		normalY += output[1];
		normalZ += output[2];
	}

	COLL_FIXED_PlayerSearch_NormalizeAxis2(d, normalX, normalY, normalZ);
}

	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(d->angle);

		d->rotCurr.z =
		    ratan2((CTR_MipsMulLo(-d->AxisAngle2_normalVec[0], trig.z) + CTR_MipsMulLo(d->AxisAngle2_normalVec[2], trig.x)) >> 12, d->AxisAngle2_normalVec[1]);
	}

	if (d->hazardTimer < 1)
	{
		if ((d->actionsFlagSet & 1) != 0)
		{
			s32 speed = Coll_MipsAbsS32(d->speed);

			if (speed > 0x1000)
			{
				s32 screenOffset = Coll_MipsAbsS32((s8)d->Screen_OffsetY);

				if ((screenOffset < 4) && ((d->terrainMeta1->flags & 1) != 0))
				{
					d->distanceFromGround = 4;
					goto UpdateGroundOffset;
				}
			}
		}

		d->distanceFromGround = 0;
	}
	else
	{
		s32 screenOffset = Coll_MipsAbsS32((s8)d->Screen_OffsetY);

		if (screenOffset < 4)
		{
			d->distanceFromGround = 4;

			if ((d->kartState != 3) && ((s8)d->Screen_OffsetY > 0))
			{
				OtherFX_Play(0x10, 1);
			}
		}
	}

UpdateGroundOffset:
	if (Coll_MipsAbsS32((s8)d->Screen_OffsetY) > 9)
	{
		d->distanceFromGround = 0;
	}

	if (d->distanceFromGround == 0)
	{
		s8 nextOffset = (s8)d->Screen_OffsetY - 4;

		if ((s8)d->Screen_OffsetY > 0)
		{
			d->Screen_OffsetY = nextOffset;

			if (nextOffset < 1)
			{
				d->Screen_OffsetY = 0;

				if ((d->terrainMeta1->flags & 0x20) != 0)
				{
					u32 soundFlags = ((d->actionsFlagSet & 0x10000) != 0) ? 0x1808080 : 0x808080;

					OtherFX_Play_LowLevel(d->terrainMeta1->sound, 0, soundFlags);
				}
			}
		}

		nextOffset = (s8)d->Screen_OffsetY - 4;
		d->Screen_OffsetY = nextOffset;

		if (nextOffset < 0)
		{
			d->Screen_OffsetY = 0;
		}
	}
	else
	{
		d->distanceFromGround--;
		d->Screen_OffsetY += 3;
	}

	if ((d->posCurr.y < -0x8000) && ((level->configFlags & 2) != 0))
	{
		d->unkAA |= 1;
	}

	if ((d->kartState != KS_MASK_GRABBED) && ((d->unkAA & 1) != 0) && (d->lastValid != NULL) && ((sdata->HudAndDebugFlags & 0x1000) == 0) &&
	    ((d->stepFlagSet & 8) == 0))
	{
		VehStuckProc_MaskGrab_Init(t, d);
	}
}


internal void CollMoved_GteRTV0(void)
{
	doCOP2(0x0486012);
}

internal void CollMoved_GteGPF12(void)
{
	doCOP2(0x0198003d);
}

internal void CollMoved_SelectProjection(CollNormalAxis normalAxis, struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex **v2,
                                         struct BspSearchVertex **v3, s32 *firstA, s32 *firstB, s32 *firstHit, s32 *secondA, s32 *secondB, s32 *secondHit)
{
	s32 origin;
	struct BspSearchVertex *tmp;

	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		origin = v1->pos.z;
		*firstA = (*v2)->pos.z - origin;
		*firstB = (*v3)->pos.z - origin;
		*firstHit = candidate->pushOut.z - origin;

		if (Coll_MipsAbsS32(*firstA) < Coll_MipsAbsS32(*firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.x;
		*secondA = (*v2)->pos.x - origin;
		*secondB = (*v3)->pos.x - origin;
		*secondHit = candidate->pushOut.x - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		origin = v1->pos.x;
		*firstA = (*v2)->pos.x - origin;
		*firstB = (*v3)->pos.x - origin;
		*firstHit = candidate->pushOut.x - origin;

		if (Coll_MipsAbsS32(*firstA) < Coll_MipsAbsS32(*firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.y;
		*secondA = (*v2)->pos.y - origin;
		*secondB = (*v3)->pos.y - origin;
		*secondHit = candidate->pushOut.y - origin;
	}
	else
	{
		origin = v1->pos.y;
		*firstA = (*v2)->pos.y - origin;
		*firstB = (*v3)->pos.y - origin;
		*firstHit = candidate->pushOut.y - origin;

		if (Coll_MipsAbsS32(*firstA) < Coll_MipsAbsS32(*firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.z;
		*secondA = (*v2)->pos.z - origin;
		*secondB = (*v3)->pos.z - origin;
		*secondHit = candidate->pushOut.z - origin;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f928-0x8001fc40
s32 COLL_MOVED_TRIANGL_ReorderNormals(struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	struct BspSearchVertex *baryV2 = v2;
	struct BspSearchVertex *baryV3 = v3;
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;
	s32 baryA = -0x1000;
	s32 baryB = -0x1000;

	CollMoved_SelectProjection(candidate->normalAxis, candidate, v1, &baryV2, &baryV3, &firstA, &firstB, &firstHit, &secondA, &secondB, &secondHit);

	if (firstA == 0)
	{
		if (firstB == 0)
			return -1;

		baryB = CTR_MipsSll(firstHit, 12) / firstB;

		if (secondA != 0)
			baryA = (CTR_MipsSll(secondHit, 12) - CTR_MipsMulLo(baryB, secondB)) / secondA;
	}
	else
	{
		s32 denom = (CTR_MipsMulLo(secondB, firstA) - CTR_MipsMulLo(firstB, secondA)) >> 6;

		if (denom != 0)
		{
			baryB = CTR_MipsMulLo(CTR_MipsMulLo(secondHit, firstA) - CTR_MipsMulLo(firstHit, secondA), 0x40) / denom;
			baryA = (CTR_MipsSll(firstHit, 12) - CTR_MipsMulLo(baryB, firstB)) / firstA;
		}
	}

	if (baryA == -0x1000)
		return -1;

	s32 sum = baryA + baryB - 0x1000;

	if (baryA < 0)
	{
		if (baryB < 0)
		{
			candidate->hitPos = v1->pos;
			return 0;
		}

		if (sum >= 0)
		{
			candidate->hitPos = baryV3->pos;
			return 4;
		}

		COLL_FIXED_TRIANGL_Barycentrics(candidate->hitPos.v, v1->pos.v, baryV3->pos.v, candidate->pushOut.v);
		return 5;
	}

	if (baryB >= 0)
	{
		if (sum <= 0)
		{
			candidate->hitPos = candidate->pushOut;
			return 6;
		}

		COLL_FIXED_TRIANGL_Barycentrics(candidate->hitPos.v, baryV2->pos.v, baryV3->pos.v, candidate->pushOut.v);
		return 3;
	}

	if (sum >= 0)
	{
		candidate->hitPos = baryV2->pos;
		return 2;
	}

	COLL_FIXED_TRIANGL_Barycentrics(candidate->hitPos.v, v1->pos.v, baryV2->pos.v, baryV3->pos.v);
	return 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001fc40-0x80020064
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 projectedFromInput;
	u16 quadFlags;

	sps->numTrianglesTested = (s16)((u16)sps->numTrianglesTested + 1);
	sps->candidate.normalAxis = (CollNormalAxis)v1->flags;
	sps->candidate.plane = v1->plane;

	struct QuadBlock *quad = sps->candidate.ptrQuadblock;
	s32 normalZW = (s32)CollFixed_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance);

	if (((quad->quadFlags & 0x400) != 0) && (((s32)(s8)quad->terrain_type & sdata->doorAccessFlags) != 0))
		return;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(sps->Input1.pos.x, sps->Input1.pos.y));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.x));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(sps->Union.QuadBlockColl.pos.y, sps->Union.QuadBlockColl.pos.z));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);
	CollMoved_GteRTV0();

	s32 halfDistance = sps->candidate.plane.halfDistance;
	s32 planeNear = CollFixed_GteReadMAC1() + halfDistance * -2;
	s32 planeFar = CollFixed_GteReadMAC2() + halfDistance * -2;

	if (planeFar < 0)
	{
		if (((quad->quadFlags & 0x40) == 0) && ((s32)quad->draw_order_low >= 0))
			goto KeepNormal;

		planeNear = -planeNear;
		planeFar = -planeFar;
		sps->candidate.plane.normal.x = (s16)-sps->candidate.plane.normal.x;
		sps->candidate.plane.normal.y = (s16)-sps->candidate.plane.normal.y;
		sps->candidate.plane.normal.z = (s16)-sps->candidate.plane.normal.z;
		sps->candidate.plane.halfDistance = (s16)-sps->candidate.plane.halfDistance;
	}

KeepNormal:
	quadFlags = quad->quadFlags;
	sps->numTrianglesTested = (s16)((u16)sps->numTrianglesTested + 1);

	if ((planeNear - sps->Input1.hitRadius) >= 0)
		return;

	if (planeFar < 0)
		return;

	if (((quadFlags & 0x40) == 0) && ((planeNear - planeFar) > 0))
		return;

	if (planeNear >= 0)
	{
		CollFixed_GteLoadIR0(planeNear);
		CollFixed_GteLoadIR(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y, sps->candidate.plane.normal.z);
		projectedFromInput = 0;
	}
	else
	{
		CollFixed_GteLoadIR(sps->Input1.pos.x - sps->Union.QuadBlockColl.pos.x, sps->Input1.pos.y - sps->Union.QuadBlockColl.pos.y,
		                    sps->Input1.pos.z - sps->Union.QuadBlockColl.pos.z);
		CollFixed_GteLoadIR0((CTR_MipsMulLo(planeNear, -0x1000)) / (planeFar - planeNear));
		projectedFromInput = 1;
	}

	CollMoved_GteGPF12();
	s32 hitX = CollFixed_GteReadMAC1();
	s32 hitY = CollFixed_GteReadMAC2();
	s32 hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(sps->candidate.pushOut.v, (s16)(sps->Input1.pos.x - hitX), (s16)(sps->Input1.pos.y - hitY), (s16)(sps->Input1.pos.z - hitZ));

	sps->bspSearchVertHit[0] = v1;
	sps->bspSearchVertHit[1] = v2;
	sps->bspSearchVertHit[2] = v3;

	s32 reorderResult = COLL_MOVED_TRIANGL_ReorderNormals(&sps->candidate, v1, v2, v3);
	if (reorderResult < 0)
		return;

	if (projectedFromInput != 0)
	{
		sps->candidateDelta.x = (s16)(sps->candidate.pushOut.x - sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)(sps->candidate.pushOut.y - sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)(sps->candidate.pushOut.z - sps->candidate.hitPos.z);
	}
	else
	{
		sps->candidateDelta.x = (s16)(sps->Input1.pos.x - sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)(sps->Input1.pos.y - sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)(sps->Input1.pos.z - sps->candidate.hitPos.z);
	}

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	CollFixed_GteLoadR13R21(sps->candidateDelta.z);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	CollFixed_GteLoadVZ0(sps->candidateDelta.z);
	CollFixed_GteMVMVA();
	s32 distanceSq = CollFixed_GteReadMAC1();

	if ((distanceSq - sps->Input1.hitRadiusSquared) > 0)
		return;

	if ((quadFlags & 0x40) != 0)
	{
		if ((planeNear < 0) || (((planeNear - sps->Input1.hitRadius) | (planeFar - sps->Input1.hitRadius)) < 0))
		{
			sps->collision.stepFlags |= (u8)quad->terrain_type;
			return;
		}
	}

	s32 distance = planeFar - planeNear;
	if (distance != 0)
		distance = 0x1000 - (CTR_MipsMulLo(sps->Input1.hitRadius - planeNear, 0x1000) / distance);

	if ((distance - sps->hitFraction) >= 0)
		return;

	if ((quadFlags & 0x10) != 0)
	{
		if ((quadFlags & 0x200) != 0)
			sps->collision.stepFlags |= 0x4000;

		return;
	}

	sps->hitFraction = distance;
	sps->levVertHit[0] = v1->pLevelVertex;
	sps->levVertHit[1] = v2->pLevelVertex;
	sps->levVertHit[2] = v3->pLevelVertex;

	sps->hit.hitPos = sps->candidate.hitPos;
	sps->hit.normalAxis = sps->candidate.normalAxis;
	sps->hit.plane = sps->candidate.plane;
	sps->hit.pushOut = sps->candidate.pushOut;
	sps->hit.ptrQuadblock = quad;

	sps->hit.triangleID = sps->candidate.triangleID;
	sps->hit.reorderResult = (s8)reorderResult;

	if (distance <= 0)
	{
		sps->Union.QuadBlockColl.hitPos = sps->Union.QuadBlockColl.pos;
	}
	else
	{
		CollFixed_GteLoadIR0(distance);
		CollFixed_GteLoadIR(sps->Input1.pos.x - sps->Union.QuadBlockColl.pos.x, sps->Input1.pos.y - sps->Union.QuadBlockColl.pos.y,
		                    sps->Input1.pos.z - sps->Union.QuadBlockColl.pos.z);
		CollMoved_GteGPF12();

		sps->Union.QuadBlockColl.hitPos.x = (s16)((u16)sps->Union.QuadBlockColl.pos.x + CollFixed_GteReadMAC1());
		sps->Union.QuadBlockColl.hitPos.y = (s16)((u16)sps->Union.QuadBlockColl.pos.y + CollFixed_GteReadMAC2());
		sps->Union.QuadBlockColl.hitPos.z = (s16)((u16)sps->Union.QuadBlockColl.pos.z + CollFixed_GteReadMAC3());
	}

	sps->boolDidTouchQuadblock = (s16)((u16)sps->boolDidTouchQuadblock + 1);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020064-0x800202a8
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->candidate.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min[0] > sps->bbox.max[0]) || (quad->bbox.min[1] > sps->bbox.max[1]) || (quad->bbox.min[2] > sps->bbox.max[2]) ||
	    (sps->bbox.min[0] > quad->bbox.max[0]) || (sps->bbox.min[1] > quad->bbox.max[1]) || (sps->bbox.min[2] > quad->bbox.max[2]))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_HIGH_LOD) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		sps->candidate.triangleID = 0;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2

		sps->candidate.triangleID = 1;

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_REUSE_NORMALS) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		sps->candidate.triangleID = 2;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		sps->candidate.triangleID = 3;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		sps->candidate.triangleID = 4;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		sps->candidate.triangleID = 5;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		sps->candidate.triangleID = 6;

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7
			sps->candidate.triangleID = 7;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8
			sps->candidate.triangleID = 8;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6
			sps->candidate.triangleID = 9;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800202a8-0x80020334
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	// if bsp flag is water
	if ((node->flag & 2) != 0)
	{
		sps->collision.stepFlags |= 0x8000;
	}

	s32 numQuads = node->data.leaf.numQuads;
	struct QuadBlock *ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_MOVED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_TEST_INSTANCES) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020334-0x80020410
void COLL_MOVED_FindScrub(struct QuadBlock *qb, s32 triangleID, struct ScratchpadStruct *sps)
{
	struct ScratchpadStructExtended *ext = (struct ScratchpadStructExtended *)sps;
	u16 searchFlags = sps->Union.QuadBlockColl.searchFlags;

	if (qb == NULL)
	{
		sps->Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
		sps->Input1.scrubDepth = 0;
		ext->numTriangles = 0;
		return;
	}

	for (s32 i = ext->numTriangles - 1; i >= 0; i--)
	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[i];

		if ((tri->quadblock == qb) && (tri->triangleID == triangleID))
		{
			s32 numCollision = tri->numCollision;
			s16 scrub = numCollision;

			if (numCollision < 0x401)
			{
				numCollision += 0x100;
				scrub = numCollision;
				tri->numCollision = numCollision;
			}

			sps->Union.QuadBlockColl.searchFlags = searchFlags | COLL_SEARCH_REPEAT_SCRUB;
			sps->Input1.scrubDepth = scrub;
			return;
		}
	}

	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[ext->numTriangles];

		tri->quadblock = qb;
		tri->triangleID = triangleID;
		tri->numCollision = 0;
	}

	sps->Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
	sps->Input1.scrubDepth = 0;
	ext->numTriangles++;
}


internal s32 CollMoved_PlayerSearch_StepVelocity(s32 velocity, s32 elapsedTimeMS, s32 multiplier)
{
	return CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(CTR_MipsMulLo(velocity, elapsedTimeMS), 5), multiplier), 12);
}

internal void CollMoved_PlayerSearch_SetBBoxAxis(struct ScratchpadStruct *sps, s32 axis, s16 current, s16 next)
{
	s16 radius = sps->Input1.hitRadius;
	s16 minCurrent = current - radius;
	s16 minNext = next - radius;
	s16 maxCurrent = current + radius;
	s16 maxNext = next + radius;

	sps->bbox.min[axis] = (minNext < minCurrent) ? minNext : minCurrent;
	sps->bbox.max[axis] = (maxCurrent < maxNext) ? maxNext : maxCurrent;
}

internal int CollMoved_PlayerSearch_RunHitboxLInC(struct ScratchpadStruct *sps, struct Thread *t)
{
	struct BSP *bsp = sps->bspHitbox;
	struct Instance *inst;
	s16 modelID;

	if ((bsp->flag & BSP_HITBOX_COLLIDABLE) != 0)
	{
		struct InstDef *instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
			return 1;

		inst = instDef->ptrInstance;
		if (inst == NULL)
			return 1;

		if ((inst->flags & 0xf) == 0)
			return 1;

		modelID = instDef->modelID;
	}
	else
	{
		if ((bsp->flag & 0x10) == 0)
			return 1;

		struct InstDef *instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
			return 1;

		// Retail passes the InstDef pointer for 0x10 hitboxes, not ptrInstance.
		inst = (struct Instance *)instDef;
		modelID = instDef->model->id;
	}

	struct MetaDataMODEL *meta = COLL_LevModelMeta(modelID);
	if ((meta != NULL) && (meta->LInC != NULL))
	{
		return meta->LInC(inst, t, sps);
	}

	return 1;
}

internal void CollMoved_PlayerSearch_StoreHitbox(struct ScratchpadStruct *sps)
{
	sps->bspHitboxesHit[sps->numBspHitboxesHit] = sps->bspHitbox;
	sps->numBspHitboxesHit++;
}

enum
{
	COLL_HITBOX_SCRUB_DEPTH_BONUS = 0x200,
};

internal u8 CollMoved_PlayerSearch_HitboxId(struct BSP *bsp)
{
	// NOTE(aalhendi): Retail reads the hitbox kind with lbu +0x1, not BSP.id.
	return ((u8 *)bsp)[1];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020410-0x80020c58
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	s32 multiplier = 0x1000;
	s16 hitRadius = 0x19;

	sps->Input1.hitRadius = hitRadius;
	sps->Input1.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
	}

	sps->numBspHitboxesHit = 0;
	sps->Input1.modelID = DYNAMIC_PLAYER;
	sps->collision.stepFlags = 0;

	COLL_MOVED_FindScrub(NULL, 0, sps);

	for (s32 iterations = 15; iterations != 0; iterations--)
	{
		s32 velocity[3];
		s16 current[3];
		s16 next[3];

		velocity[0] = CollMoved_PlayerSearch_StepVelocity(d->velocity.x, gGT->elapsedTimeMS, multiplier);
		velocity[1] = CollMoved_PlayerSearch_StepVelocity(d->velocity.y, gGT->elapsedTimeMS, multiplier);
		velocity[2] = CollMoved_PlayerSearch_StepVelocity(d->velocity.z, gGT->elapsedTimeMS, multiplier);

		sps->boolDidTouchQuadblock = 0;
		sps->numTrianglesTested = 0;
		sps->boolDidTouchHitbox = 0;
		sps->unk40 = 0;
		sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_TEST_INSTANCES;
		sps->hitFraction = 0x1000;

		current[0] = (s16)d->originToCenter.x + (d->posCurr.x >> 8);
		current[1] = (s16)d->originToCenter.y + (d->posCurr.y >> 8);
		current[2] = (s16)d->originToCenter.z + (d->posCurr.z >> 8);

		next[0] = (s16)d->originToCenter.x + ((d->posCurr.x + velocity[0]) >> 8);
		next[1] = (s16)d->originToCenter.y + ((d->posCurr.y + velocity[1]) >> 8);
		next[2] = (s16)d->originToCenter.z + ((d->posCurr.z + velocity[2]) >> 8);

		sps->Union.QuadBlockColl.pos.x = current[0];
		sps->Union.QuadBlockColl.pos.y = current[1];
		sps->Union.QuadBlockColl.pos.z = current[2];
		sps->Input1.pos.x = next[0];
		sps->Input1.pos.y = next[1];
		sps->Input1.pos.z = next[2];

		if ((sps->Input1.pos.x == sps->Union.QuadBlockColl.pos.x) && (sps->Input1.pos.y == sps->Union.QuadBlockColl.pos.y) &&
		    (sps->Input1.pos.z == sps->Union.QuadBlockColl.pos.z))
		{
			break;
		}

		CollMoved_PlayerSearch_SetBBoxAxis(sps, 0, current[0], next[0]);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 1, current[1], next[1]);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 2, current[2], next[2]);

		sps->Union.QuadBlockColl.hitPos = sps->Input1.pos;

		sps->Union.QuadBlockColl.searchFlags = (sps->Union.QuadBlockColl.searchFlags | COLL_SEARCH_TEST_INSTANCES) & ~COLL_SEARCH_REUSE_NORMALS;

		if ((gGT->level1 != NULL) && (gGT->level1->ptr_mesh_info != NULL) && (gGT->level1->ptr_mesh_info->bspRoot != NULL))
		{
			COLL_SearchBSP_CallbackPARAM(gGT->level1->ptr_mesh_info->bspRoot, &sps->bbox, COLL_MOVED_BSPLEAF_TestQuadblocks, sps);
		}

		if (sps->boolDidTouchQuadblock != 0)
		{
			d->unkAA |= 4;
		}

		if (sps->hitFraction > 0)
		{
			d->posCurr.x += CTR_MipsMulLo(velocity[0], sps->hitFraction) >> 12;
			d->posCurr.y += CTR_MipsMulLo(velocity[1], sps->hitFraction) >> 12;
			d->posCurr.z += CTR_MipsMulLo(velocity[2], sps->hitFraction) >> 12;
		}

		if (sps->boolDidTouchHitbox != 0)
		{
			struct BSP *bspHitbox = sps->bspHitbox;

			sps->Union.QuadBlockColl.searchFlags &= ~COLL_SEARCH_REUSE_NORMALS;
			d->unkAA &= 0xfffd;

			s32 hitboxResult = CollMoved_PlayerSearch_RunHitboxLInC(sps, t);
			u8 hitboxId = CollMoved_PlayerSearch_HitboxId(bspHitbox);

			if ((hitboxResult == 2) || (hitboxId == 4))
			{
				CollMoved_PlayerSearch_StoreHitbox(sps);
			}
			else
			{
				COLL_MOVED_FindScrub((struct QuadBlock *)bspHitbox, 0, sps);
				sps->Input1.scrubDepth = (s16)(sps->Input1.scrubDepth + COLL_HITBOX_SCRUB_DEPTH_BONUS);

				struct Scrub *scrub = VehAfterColl_GetSurface(hitboxId);
				hitboxResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity.x);

				if (hitboxResult == 0)
				{
					CollMoved_PlayerSearch_StoreHitbox(sps);
				}

				if (hitboxResult == 2)
				{
					return;
				}
			}
		}
		else
		{
			if (sps->boolDidTouchQuadblock == 0)
			{
				break;
			}

			struct QuadBlock *quad = sps->hit.ptrQuadblock;

			if ((quad->quadFlags & 0x200) != 0)
			{
				d->unkAA |= 1;
			}

			COLL_MOVED_FindScrub(quad, sps->hit.triangleID, sps);

			u32 scrubId;
			if ((quad->quadFlags & 0x1000) == 0)
			{
				scrubId = ((quad->quadFlags & 1) == 0) ? 0 : 6;
			}
			else
			{
				if ((quad != d->underDriver) && ((quad->quadFlags & 8) != 0))
				{
					d->underDriver = NULL;
				}

				d->currBlockTouching = quad;
				d->normalVecUP = sps->hit.plane.normal;
				d->AxisAngle1_normalVec = sps->hit.plane.normal;
				d->unkAA |= 8;
				scrubId = 5;
			}

			struct Scrub *scrub = VehAfterColl_GetSurface(scrubId);
			d->unkAA |= 2;
			CTR_COPY_VEC3(d->spsHitPos, sps->hit.hitPos.v);
			CTR_COPY_VEC3(d->spsNormalVec, sps->hit.plane.normal.v);

			u32 impactResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity.x);
			if (impactResult == 2)
			{
				return;
			}

			if (sps->hitFraction > 0)
			{
				multiplier -= CTR_MipsMulLo(multiplier, sps->hitFraction) >> 12;
				if (multiplier < 100)
				{
					break;
				}
			}

			sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_REUSE_NORMALS;
		}
	}

	d->stepFlagSet = sps->collision.stepFlags;
}


internal s32 CollMoved_ScrubImpact_TrigX(s32 angle)
{
	struct TrigTable trig = data.trigApprox[angle & 0x3ff];
	s32 value = ((angle & 0x400) == 0) ? trig.sin : trig.cos;

	if ((angle & 0x800) != 0)
	{
		value = -value;
	}

	return value;
}

internal void CollMoved_ScrubImpact_GteLLV0(s32 x, s32 y, s32 z, s32 *outX, s32 *outY, s32 *outZ)
{
	MTC2(CollFixed_PackS16Pair(x, y), 0);
	MTC2(z, 1);
	doCOP2(0x04a6012);
	*outX = MFC2_S(25);
	*outY = MFC2_S(26);
	*outZ = MFC2_S(27);
}

internal void CollMoved_ScrubImpact_GteOP12(void)
{
	doCOP2(0x0178000c);
}

internal void CollMoved_ScrubImpact_ProjectWallVelocity(s16 normalX, s16 normalY, s16 normalZ, s32 oldVelX, s32 oldVelZ, s32 *outX, s32 *outY, s32 *outZ)
{
	MTC2(oldVelX, 9);
	MTC2(0, 10);
	MTC2(oldVelZ, 11);
	CTC2(normalX, 0);
	CTC2(normalY, 2);
	CTC2(normalZ, 4);

	CollMoved_ScrubImpact_GteOP12();

	u32 r11 = CFC2(0);
	u32 r22 = CFC2(2);
	u32 r33 = CFC2(4);

	MTC2(r11, 9);
	MTC2(r22, 10);
	MTC2(r33, 11);

	s32 crossX = MFC2_S(25);
	s32 crossY = MFC2_S(26);
	s32 crossZ = MFC2_S(27);

	CTC2(crossX, 0);
	CTC2(crossY, 2);
	CTC2(crossZ, 4);

	CollMoved_ScrubImpact_GteOP12();

	*outX = MFC2_S(25);
	*outY = MFC2_S(26);
	*outZ = MFC2_S(27);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020c58-0x80021500
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, s32 *velocity)
{
	s16 normalX = sps->hit.plane.normal.x;
	s16 normalY = sps->hit.plane.normal.y;
	s16 normalZ = sps->hit.plane.normal.z;

	if ((d->unknownTraction != 0) && (sps->boolDidTouchQuadblock != 0) && ((sps->hit.ptrQuadblock->quadFlags & 0x1000) != 0) && (sps->hit.reorderResult != 6) &&
	    (sps->hit.ptrQuadblock != d->underDriver))
	{
		if ((Coll_MipsAbsS32(d->speedApprox) < 0x300) && (Coll_MipsAbsS32(d->jumpHeightCurr) < 0x300) && (d->fireSpeed == 0))
		{
			s32 diffX = (d->posCurr.x >> 8) - sps->hit.hitPos.x;
			s32 diffZ = (d->posCurr.z >> 8) - sps->hit.hitPos.z;
			s32 diffY = (d->posCurr.y >> 8) - sps->hit.hitPos.y;

			if ((diffX | diffY | diffZ) != 0)
			{
				s32 len = VehCalc_FastSqrt((u32)CTR_MipsMulLo(diffX, diffX) + (u32)CTR_MipsMulLo(diffY, diffY) + (u32)CTR_MipsMulLo(diffZ, diffZ), 0);

				normalX = CTR_MipsSll(diffX, 12) / len;
				normalY = CTR_MipsSll(diffY, 12) / len;
				normalZ = CTR_MipsSll(diffZ, 12) / len;
			}
		}
	}

	s32 dot = ((velocity[0] >> 3) * normalX + (velocity[1] >> 3) * normalY + (velocity[2] >> 3) * normalZ) >> 9;

	if (dot < -0xa00)
	{
		d->actionsFlagSet |= 0x80;
	}

	dot -= sps->Input1.scrubDepth;

	u32 ret = 0;
	if (dot < 0)
	{
		u32 scrubFlags = scrub->flags;

		if ((scrubFlags & 4) == 0)
		{
			d->actionsFlagSet |= 0x2000;
		}

		if ((scrubFlags & 8) == 0)
		{
			d->reserves = 0;
			d->turbo_outsideTimer = 0;
		}

		s32 scrubSpeed = scrub->unk_0x8;

		if (!((d->set_0xF0_OnWallRub == 0) ? (0x3e7ff < scrubSpeed) : (d->scrubMeta8 < scrubSpeed)))
		{
			d->set_0xF0_OnWallRub = 0xf0;
			d->scrubMeta8 = scrubSpeed;
			d->posWallColl[0] = sps->hit.hitPos.x;
			d->posWallColl[1] = sps->hit.hitPos.y;
			d->posWallColl[2] = sps->hit.hitPos.z;
		}

		ret = 0;

		if ((scrubFlags & 1) != 0)
		{
			s32 impactX = CTR_MipsSra(CTR_MipsMulLo(dot, normalX), 12);
			s32 impactY = CTR_MipsSra(CTR_MipsMulLo(dot, normalY), 12);
			s32 impactZ = CTR_MipsSra(CTR_MipsMulLo(dot, normalZ), 12);
			s32 speedSq = 0;
			s32 wallVelX;
			s32 wallVelY;
			s32 wallVelZ;

			if (scrub->unk_angle != 0)
			{
				speedSq = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(velocity[0], velocity[0]), CTR_MipsMulLo(velocity[1], velocity[1])),
				                                    CTR_MipsMulLo(velocity[2], velocity[2])),
				                      15);
			}

			s32 oldVelX = velocity[0];
			s32 oldVelZ = velocity[2];
			velocity[0] = oldVelX - impactX;
			velocity[2] = oldVelZ - impactZ;
			velocity[1] -= impactY;

			CollMoved_ScrubImpact_GteLLV0(impactX, impactY, impactZ, &impactX, &impactY, &impactZ);

			if ((sps->boolDidTouchQuadblock != 0) && ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_WALL_PROJECTION_DONE) == 0) &&
			    ((d->actionsFlagSetPrevFrame & 1) == 0) && ((sps->hit.ptrQuadblock->quadFlags & 0x1000) != 0))
			{
				CollMoved_ScrubImpact_ProjectWallVelocity(normalX, normalY, normalZ, oldVelX, oldVelZ, &wallVelX, &wallVelY, &wallVelZ);

				{
					u32 wallSpeed =
					    VehCalc_FastSqrt(
					        (u32)CTR_MipsMulLo(wallVelX, wallVelX) + (u32)CTR_MipsMulLo(wallVelY, wallVelY) + (u32)CTR_MipsMulLo(wallVelZ, wallVelZ), 0x10) >>
					    8;
					s32 speedApprox = d->speedApprox;

					if ((wallSpeed != 0) && (speedApprox > 0))
					{
						sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_WALL_PROJECTION_DONE;
						velocity[0] = CTR_MipsMulLo(wallVelX, speedApprox) / (s32)wallSpeed;
						velocity[1] = CTR_MipsMulLo(wallVelY, speedApprox) / (s32)wallSpeed;
						velocity[2] = CTR_MipsMulLo(wallVelZ, speedApprox) / (s32)wallSpeed;
						velocity[0] -= normalX >> 1;
						velocity[1] -= normalY >> 1;
						velocity[2] -= normalZ >> 1;
					}
				}
			}

			s32 transformedImpactXZ = CTR_MipsAddLo(CTR_MipsMulLo(impactX, impactX), CTR_MipsMulLo(impactZ, impactZ));

			if (((scrubFlags & 2) != 0) && (dot < -0x13ff) && (transformedImpactXZ > 0x1900000))
			{
				if (d->kartState != KS_MASK_GRABBED)
				{
					GAMEPAD_JogCon1(d, (d->simpTurnState < 1) ? 0x1f : 0x2f, 0x60);
				}

				if (scrub->unk_angle != 0)
				{
					s32 trig = CollMoved_ScrubImpact_TrigX(scrub->unk_angle);
					s32 scaledSpeed = CTR_MipsSra(CTR_MipsMulLo(speedSq, trig), 12);
					s32 angleLimit = CTR_MipsSra(CTR_MipsMulLo(scaledSpeed, trig), 12);
					s32 dotSq = CTR_MipsSra(CTR_MipsMulLo(dot, dot), 15);

					if (angleLimit >= dotSq)
						return 1;
				}

				if ((d->kartState != KS_MASK_GRABBED) && (transformedImpactXZ > 0x1900000))
				{
					u32 soundFlags = 0xff8080;

					if ((d->actionsFlagSet & 0x10000) != 0)
					{
						soundFlags = 0x1ff8080;
					}

					OtherFX_Play_LowLevel(6, 1, soundFlags);
					Voiceline_RequestPlay(6, data.characterIDs[d->driverID], 0x10);
					GAMEPAD_ShockFreq(d, 8, 0);
					GAMEPAD_ShockForce1(d, 8, 0x7f);

					if (d->kartState == KS_DRIFTING)
					{
						s16 turnAngle = d->turnAngleCurr;

						d->turnAngleCurr = 0;
						d->angle += turnAngle;
						d->rotCurr.w -= turnAngle;
					}

					d->instSelf->animIndex = 2;
					d->instSelf->animFrame = 0;
					d->matrixArray = 4;
					d->matrixIndex = 0;

					VehPhysProc_SlamWall_Init(t, d);
					return 2;
				}
			}

			ret = 1;
		}
	}

	return ret;
}
