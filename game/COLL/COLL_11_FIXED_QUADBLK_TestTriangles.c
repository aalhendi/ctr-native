#include <common.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): CTR_NATIVE bridge, not ASM-verified. This keeps the retail
// fixed-quadblock symbol wired for callers while the lower triangle/GTE path is
// still pending a full COLL audit.
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct mesh_info *meshInfo;
	struct CtrNativeCollVec top;
	struct CtrNativeCollVec bottom;
	struct QuadBlock *bestQuad = NULL;
	s16 bestHit[3];
	s16 bestNormal[4] = {0, 0x1000, 0, 0};
	double bestT = 2.0;
	s16 searchFlags;

	sps->Set1.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min[0] > sps->bbox.max[0]) || (quad->bbox.min[1] > sps->bbox.max[1]) || (quad->bbox.min[2] > sps->bbox.max[2]) ||
	    (sps->bbox.min[0] > quad->bbox.max[0]) || (sps->bbox.min[1] > quad->bbox.max[1]) || (sps->bbox.min[2] > quad->bbox.max[2]))
	{
		return;
	}

	meshInfo = sps->ptr_mesh_info;
	if ((meshInfo == NULL) || (meshInfo->ptrVertexArray == NULL))
		return;

	top.x = sps->Input1.pos[0];
	top.y = sps->Input1.pos[1];
	top.z = sps->Input1.pos[2];
	bottom.x = sps->Union.QuadBlockColl.pos[0];
	bottom.y = sps->Union.QuadBlockColl.pos[1];
	bottom.z = sps->Union.QuadBlockColl.pos[2];

	searchFlags = sps->Union.QuadBlockColl.searchFlags;
	if ((searchFlags & 2) == 0)
	{
		CtrNativeColl_TryQuadTriangle(meshInfo, quad, 0, 1, 2, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);

		if (quad->index[2] != quad->index[3])
			CtrNativeColl_TryQuadTriangle(meshInfo, quad, 1, 3, 2, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
	}
	else
	{
		CtrNativeColl_TryQuadTriangle(meshInfo, quad, 8, 6, 7, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
		CtrNativeColl_TryQuadTriangle(meshInfo, quad, 7, 3, 8, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
		CtrNativeColl_TryQuadTriangle(meshInfo, quad, 1, 7, 6, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
		CtrNativeColl_TryQuadTriangle(meshInfo, quad, 2, 6, 8, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);

		if (quad->index[2] != quad->index[3])
		{
			CtrNativeColl_TryQuadTriangle(meshInfo, quad, 0, 4, 5, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
			CtrNativeColl_TryQuadTriangle(meshInfo, quad, 4, 6, 5, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
			CtrNativeColl_TryQuadTriangle(meshInfo, quad, 6, 4, 1, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
			CtrNativeColl_TryQuadTriangle(meshInfo, quad, 5, 6, 2, top, bottom, &bestT, bestHit, bestNormal, &bestQuad);
		}
	}

	if (bestQuad == NULL)
		return;

	if ((quad->quadFlags & 0x40) != 0)
	{
		*(u32 *)&sps->dataOutput[0] |= (u8)quad->terrain_type;
		return;
	}

	sps->Set2.ptrQuadblock = bestQuad;
	sps->Union.QuadBlockColl.hitPos[0] = bestHit[0];
	sps->Union.QuadBlockColl.hitPos[1] = bestHit[1];
	sps->Union.QuadBlockColl.hitPos[2] = bestHit[2];
	sps->Set1.hitPos[0] = bestHit[0];
	sps->Set1.hitPos[1] = bestHit[1];
	sps->Set1.hitPos[2] = bestHit[2];
	sps->Set1.normalVec[0] = bestNormal[0];
	sps->Set1.normalVec[1] = bestNormal[1];
	sps->Set1.normalVec[2] = bestNormal[2];
	sps->Set1.normalVec[3] = bestNormal[3];
	sps->Set1.BspSearchVertexFlags = CtrNativeColl_NormalDominantAxis(bestNormal);
	sps->Set1.ptrQuadblock = bestQuad;
	sps->Set2.hitPos[0] = bestHit[0];
	sps->Set2.hitPos[1] = bestHit[1];
	sps->Set2.hitPos[2] = bestHit[2];
	sps->Set2.normalVec[0] = bestNormal[0];
	sps->Set2.normalVec[1] = bestNormal[1];
	sps->Set2.normalVec[2] = bestNormal[2];
	sps->Set2.normalVec[3] = bestNormal[3];
	sps->Set2.BspSearchVertexFlags = sps->Set1.BspSearchVertexFlags;
	sps->boolDidTouchQuadblock++;
}
#else
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->Set1.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) != 0) && ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) == 0) &&
	    (quad->bbox.min[0] < sps->bbox.max[0]) && (quad->bbox.min[1] < sps->bbox.max[1]) && (quad->bbox.min[2] < sps->bbox.max[2]) &&
	    (sps->bbox.min[0] < quad->bbox.max[0]) && (sps->bbox.min[1] < quad->bbox.max[1]) && (sps->bbox.min[2] < quad->bbox.max[2]))
	{
		// if 3P or 4P mode,
		// then use low-LOD quadblock collision (two triangles)
		if ((sps->Union.QuadBlockColl.searchFlags & 2) == 0)
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
			if ((sps->Union.QuadBlockColl.searchFlags & 8) == 0)
			{
				COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
			}

			COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[8] + 0x10, (int)&bsv[6] + 0x10, (int)&bsv[7] + 0x10); // 8, 6, 7
			COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[7] + 0x10, (int)&bsv[3] + 0x10, (int)&bsv[8] + 0x10); // 7, 3, 8
			COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[1] + 0x10, (int)&bsv[7] + 0x10, (int)&bsv[6] + 0x10); // 1, 7, 6
			COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[2] + 0x10, (int)&bsv[6] + 0x10, (int)&bsv[8] + 0x10); // 2, 6, 8

			// If this is a quad instead of a triangle
			if (quad->index[2] != quad->index[3])
			{
				COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[0] + 0x10, (int)&bsv[4] + 0x10, (int)&bsv[5] + 0x10); // 0, 4, 5
				COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[4] + 0x10, (int)&bsv[6] + 0x10, (int)&bsv[5] + 0x10); // 4, 6, 5
				COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[6] + 0x10, (int)&bsv[4] + 0x10, (int)&bsv[1] + 0x10); // 6, 4, 1
				COLL_FIXED_TRIANGL_TestPoint((int)sps + 0x10, (int)&bsv[5] + 0x10, (int)&bsv[6] + 0x10, (int)&bsv[2] + 0x10); // 5, 6, 2
			}
		}
	}
}
#endif
