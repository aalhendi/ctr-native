#include <common.h>

static const s16 s_warpballParticleHeight = 0xff;

static void RB_Warpball_AdvanceStraight(struct TrackerWeapon *tw, struct Instance *inst, int elapsedTime)
{
	inst->matrix.t[0] += ((int)tw->vel[0] * elapsedTime) >> 5;
	inst->matrix.t[1] += ((int)tw->vel[1] * elapsedTime) >> 5;
	inst->matrix.t[2] += ((int)tw->vel[2] * elapsedTime) >> 5;
}

static int RB_Warpball_NodeDeltaLength(struct CheckpointNode *curr, struct CheckpointNode *next, int *dx, int *dy, int *dz)
{
	*dx = next->pos[0] - curr->pos[0];
	*dy = next->pos[1] - curr->pos[1];
	*dz = next->pos[2] - curr->pos[2];

	return SquareRoot0_stub(((*dx) * (*dx)) + ((*dy) * (*dy)) + ((*dz) * (*dz)));
}

static void RB_Warpball_SetQuadblockIndex(struct TrackerWeapon *tw, struct ScratchpadStruct *sps)
{
	if (sps->hit.ptrQuadblock->checkpointIndex != 0xff)
	{
		tw->nodeNextIndex = sps->hit.ptrQuadblock->checkpointIndex;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aef9c-0x800afb70.
// NOTE(aalhendi): Native uses the extracted warpball particle-height halfword from RDATA 0x800b2c84.
void RB_Warpball_ThTick(struct Thread *t)
{
	struct GameTracker *gGT;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *target;
	struct ScratchpadStruct *sps;
	struct Instance *hitInst;
	s16 posTop[3];
	s16 posBottom[3];
	int elapsedTime;
	int distX;
	int distY;
	int distZ;
	int distXZ;

	gGT = sdata->gGT;
	inst = t->inst;
	tw = t->object;

	((s16 *)&tw->unk4c)[0] = (s16)inst->matrix.t[0];
	((s16 *)&tw->unk4c)[1] = (s16)inst->matrix.t[1];
	tw->unk50 = (s16)inst->matrix.t[2];

	if ((int)inst->animFrame + 1 < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
	}
	else
	{
		inst->animFrame = 0;
	}

	if (tw->driverTarget != NULL)
	{
		if ((tw->driverTarget->kartState == KS_MASK_GRABBED) && ((tw->flags & 4) != 0))
		{
			struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;

			tw->flags = (tw->flags & 0xfffb) | 0x18;
			tw->ptrNodeCurr = &nodes[tw->nodeNextIndex];
			tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
			tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);
			RB_Warpball_SetTargetDriver(tw);
		}

		if ((tw->flags & 0x204) == 0)
		{
			tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);

			if (tw->driverTarget != NULL)
			{
				RB_Warpball_SetTargetDriver(tw);
			}
		}
	}

	target = tw->driverTarget;
	tw->flags &= 0xfdff;
	elapsedTime = gGT->elapsedTimeMS;

	if (target != NULL)
	{
		distX = (target->posCurr.x >> 8) - inst->matrix.t[0];
		distZ = (target->posCurr.z >> 8) - inst->matrix.t[2];
		distY = (target->posCurr.y >> 8) - inst->matrix.t[1];
		distXZ = (distX * distX) + (distZ * distZ);
		tw->distanceToTarget = distXZ;
		target->thTrackingMe = RB_GetThread_ClosestTracker(target);

		if ((tw->flags & 0xc) != 0)
		{
			s16 rotSpeed = 0x100;

			if ((tw->flags & 4) == 0)
			{
				struct CheckpointNode *pathNode = tw->ptrNodeCurr;

				distX = pathNode->pos[0] - inst->matrix.t[0];
				distZ = pathNode->pos[2] - inst->matrix.t[2];
				distY = pathNode->pos[1] - inst->matrix.t[1];
				distXZ = (distX * distX) + (distZ * distZ);

				if (distXZ < 0x4000)
				{
					tw->flags = (tw->flags & 0xfff7) | 0x100;
				}
				else if (distXZ < 0x24000)
				{
					rotSpeed = 0x400;
				}
			}

			if (distXZ < 0x90000)
			{
				rotSpeed = 0x400 - (distXZ >> 9);

				if (rotSpeed < 0x100)
				{
					rotSpeed = 0x100;
				}
			}

			if (tw->frameCount_DontHurtParent > 0)
			{
				rotSpeed = 0x40;
			}

			tw->dir[0] = 0;
			tw->dir[1] = RB_Hazard_InterpolateValue(tw->dir[1], ratan2(distX, distZ), rotSpeed);
			tw->dir[2] = 0;
			tw->vel[0] = (MATH_Sin(tw->dir[1]) * 7) >> 8;
			tw->vel[2] = (MATH_Cos(tw->dir[1]) * 7) >> 8;

			if (distY > 0)
			{
				tw->vel[1] += (elapsedTime << 2) >> 5;

				if (distY < tw->vel[1])
				{
					tw->vel[1] = distY;
				}

				if (tw->vel[1] > 0x60)
				{
					tw->vel[1] = 0x60;
				}
			}
			else if (distY < 0)
			{
				tw->vel[1] -= (elapsedTime << 2) >> 5;

				if (tw->vel[1] < distY)
				{
					tw->vel[1] = distY;
				}

				if (tw->vel[1] < -0x60)
				{
					tw->vel[1] = -0x60;
				}
			}

			RB_Warpball_AdvanceStraight(tw, inst, elapsedTime);
		}
		else
		{
			struct CheckpointNode *curr = tw->ptrNodeCurr;
			struct CheckpointNode *next = tw->ptrNodeNext;
			int segmentLength = RB_Warpball_NodeDeltaLength(curr, next, &distX, &distY, &distZ);
			int progress = tw->respawnPointIndex + ((elapsedTime * 0x70) >> 5);
			int fraction;

			if (segmentLength <= progress)
			{
				progress -= segmentLength;

				do
				{
					int keepAdvancing;

					curr = next;
					next = RB_Warpball_NewPathNode(curr, tw->driverTarget);
					segmentLength = RB_Warpball_NodeDeltaLength(curr, next, &distX, &distY, &distZ);
					keepAdvancing = segmentLength <= progress;
					progress -= segmentLength;

					if (!keepAdvancing)
					{
						break;
					}
				} while (1);

				progress += segmentLength;
			}

			tw->respawnPointIndex = progress;
			tw->ptrNodeCurr = curr;
			tw->ptrNodeNext = next;

			if (segmentLength == 0)
			{
				fraction = 0;
			}
			else
			{
				fraction = (progress << 12) / segmentLength;
			}

			inst->matrix.t[0] = curr->pos[0] + ((distX * fraction) >> 12);
			inst->matrix.t[1] = curr->pos[1] + ((distY * fraction) >> 12);
			inst->matrix.t[2] = curr->pos[2] + ((distZ * fraction) >> 12);

			tw->dir[1] = ratan2(distX, distZ);
			tw->vel[0] = (MATH_Sin(tw->dir[1]) * 7) >> 8;
			tw->vel[2] = (MATH_Cos(tw->dir[1]) * 7) >> 8;
			tw->vel[1] = 0;
		}
	}
	else
	{
		RB_Warpball_AdvanceStraight(tw, inst, elapsedTime);
	}

	PlaySound3D_Flags((u32 *)&tw->audioPtr, 0x4e, inst);

	posTop[0] = (s16)inst->matrix.t[0];
	posTop[1] = (s16)(inst->matrix.t[1] - 0x80);
	posTop[2] = (s16)inst->matrix.t[2];
	posBottom[0] = (s16)inst->matrix.t[0];
	posBottom[1] = (s16)(inst->matrix.t[1] + 0x80);
	posBottom[2] = (s16)inst->matrix.t[2];

	sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x1040;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_FORCE_INSTANCE_HIT;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD | COLL_SEARCH_FORCE_INSTANCE_HIT;
	}

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
	COLL_SearchBSP_CallbackQUADBLK((u32 *)&posTop[0], (u32 *)&posBottom[0], sps, 0);
	RB_MakeInstanceReflective(sps, inst);

	if ((sps->collision.stepFlags & 4) != 0)
	{
		RB_Warpball_TurnAround(t);
	}

	if (sps->boolDidTouchHitbox != 0)
	{
		sps->Input1.modelID = 0x36;

		if (RB_Hazard_CollLevInst(sps, t) == 1)
		{
			RB_Warpball_TurnAround(t);
		}
	}

	if (sps->boolDidTouchQuadblock != 0)
	{
		tw->flags |= 0x100;
		RB_Warpball_SetQuadblockIndex(tw, sps);
		tw->vel[1] = 0;

		if (((tw->flags & 0xc) != 0) && (inst->matrix.t[1] < sps->hit.hitPos[1]))
		{
			inst->matrix.t[1] = sps->hit.hitPos[1];
			inst->unk50 = sps->hit.ptrQuadblock->draw_order_low - 1;
		}
	}
	else
	{
		posTop[0] = (s16)inst->matrix.t[0];
		posTop[1] = (s16)(inst->matrix.t[1] - 0x900);
		posTop[2] = (s16)inst->matrix.t[2];
		COLL_SearchBSP_CallbackQUADBLK((u32 *)&posTop[0], (u32 *)&posBottom[0], sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			tw->flags |= 0x100;
			RB_Warpball_SetQuadblockIndex(tw, sps);
		}

		if ((sps->boolDidTouchQuadblock == 0) && (((tw->flags & 0xc) != 0) || (tw->driverTarget == NULL)))
		{
			RB_Warpball_TurnAround(t);
		}
	}

	if ((s_warpballParticleHeight != 0) && (tw->ptrParticle != NULL))
	{
		struct Particle *p = tw->ptrParticle;

		p->axis[0].startVal = inst->matrix.t[0] << 8;
		p->axis[1].startVal = (inst->matrix.t[1] + s_warpballParticleHeight) << 8;
		p->axis[2].startVal = inst->matrix.t[2] << 8;
		p->axis[3].startVal = s_warpballParticleHeight << 8;
		p->axis[4].startVal = s_warpballParticleHeight * 0xc0;
		p->axis[5].startVal = s_warpballParticleHeight << 7;
		p->unk18 = inst->unk50 + 1;
		p->framesLeftInLife = -1;
	}

	hitInst = RB_Hazard_CollideWithDrivers(inst, tw->frameCount_DontHurtParent, 0x9000, tw->instParent);

	if (hitInst != NULL)
	{
		struct Driver *hitDriver = hitInst->thread->object;

		if (hitDriver != tw->driverParent)
		{
			u16 hadTargetPathFlag = tw->flags & 4;
			u16 flagsBeforeHit;

			RB_Hazard_HurtDriver(hitDriver, 2, tw->driverParent, 0);
			hitDriver->damageColorTimer = 0x1e;

			flagsBeforeHit = tw->flags | 0x40;
			tw->flags = flagsBeforeHit;

			if ((((flagsBeforeHit & 1) == 0) && (tw->driverTarget == hitDriver)) || (hitDriver->driverRank == 0))
			{
				tw->driverParent->instBombThrow = NULL;
				RB_Warpball_Death(t);
				return;
			}

			tw->driversHit |= 1u << (hitDriver->driverID & 0x1f);

			for (int rank = hitDriver->driverRank; rank < 8; rank++)
			{
				struct Driver *rankDriver = gGT->driversInRaceOrder[rank];

				if (rankDriver != NULL)
				{
					tw->driversHit |= 1u << (rankDriver->driverID & 0x1f);
				}
			}

			if (tw->driverTarget == hitDriver)
			{
				tw->flags &= 0xfffb;
				tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);

				if (tw->driverTarget == NULL)
				{
					tw->driverParent->instBombThrow = NULL;
					RB_Warpball_Death(t);
				}
				else
				{
					RB_Warpball_SetTargetDriver(tw);
				}

				if (hadTargetPathFlag != 0)
				{
					RB_Warpball_SeekDriver(tw, hitDriver->unknown_lap_related[1], hitDriver);
				}

				if (((tw->flags & 4) == 0) && (hadTargetPathFlag != 0))
				{
					if (tw->nodeCurrIndex != 0xff)
					{
						struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;

						tw->ptrNodeCurr = &nodes[tw->nodeCurrIndex];
						tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
					}

					tw->flags |= 8;
				}
			}
		}
	}
	else
	{
		hitInst = RB_Hazard_CollideWithBucket(inst, t, gGT->threadBuckets[MINE].thread, tw->frameCount_DontHurtParent, 0x2400, tw->instParent);

		if (hitInst != NULL)
		{
			struct Thread *hitTh = hitInst->thread;

			hitTh->funcThCollide(hitTh);
		}
	}

	if (tw->frameCount_DontHurtParent != 0)
	{
		tw->frameCount_DontHurtParent--;
	}
}
