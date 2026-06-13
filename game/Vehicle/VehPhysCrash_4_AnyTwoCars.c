#include <common.h>

// NOTE(aalhendi): These static helpers factor repeated retail blocks; they
// are not separate retail symbols.
static s32 VehPhysCrash_WeightedAverage(s32 lhs, s16 lhsWeight, s32 rhs, s16 rhsWeight)
{
	return CTR_MipsDiv(CTR_MipsAddLo(CTR_MipsMulLo(lhs, lhsWeight), CTR_MipsMulLo(rhs, rhsWeight)), CTR_MipsAddLo(lhsWeight, rhsWeight));
}

static void VehPhysCrash_WeightedVelocity(Vec3 *out, Vec3 *lhs, struct Driver *lhsDriver, Vec3 *rhs, struct Driver *rhsDriver)
{
	out->x = VehPhysCrash_WeightedAverage(lhs->x, lhsDriver->unk47C, rhs->x, rhsDriver->unk47C);
	out->y = VehPhysCrash_WeightedAverage(lhs->y, lhsDriver->unk47C, rhs->y, rhsDriver->unk47C);
	out->z = VehPhysCrash_WeightedAverage(lhs->z, lhsDriver->unk47C, rhs->z, rhsDriver->unk47C);
}

static void VehPhysCrash_AddImpulse(Vec3 *vel, s16 *hitDir, s32 strength)
{
	vel->x = CTR_MipsAddLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(hitDir[0], strength), 8));
	vel->y = CTR_MipsAddLo(vel->y, CTR_MipsSra(CTR_MipsMulLo(hitDir[1], strength), 8));
	vel->z = CTR_MipsAddLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(hitDir[2], strength), 8));
}

static void VehPhysCrash_SubImpulse(Vec3 *vel, s16 *hitDir, s32 strength)
{
	vel->x = CTR_MipsSubLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(hitDir[0], strength), 8));
	vel->y = CTR_MipsSubLo(vel->y, CTR_MipsSra(CTR_MipsMulLo(hitDir[1], strength), 8));
	vel->z = CTR_MipsSubLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(hitDir[2], strength), 8));
}

static void VehPhysCrash_BouncePair(s16 *hitDir, Vec3 *weightedVel, Vec3 *otherVel, Vec3 *selfVel)
{
	if (VehPhysCrash_BounceSelf(hitDir, weightedVel, otherVel, 1) < 0)
	{
		sdata->unk_8008d9f4[0] = 0;
	}

	if (VehPhysCrash_BounceSelf(hitDir, weightedVel, selfVel, 0) > 0)
	{
		sdata->unk_8008d9f4[0] = 0;
	}
}

static void VehPhysCrash_PlayHumanFeedback(struct Thread *selfThread, struct Thread *otherThread, struct Driver *selfDriver, struct Driver *otherDriver,
                                           u32 canPlayFeedback)
{
	if (sdata->unk_8008d9f4[0] <= 0x200)
	{
		return;
	}

	if ((selfThread->modelIndex == DYNAMIC_PLAYER) || (otherThread->modelIndex == DYNAMIC_PLAYER))
	{
		int volume = VehCalc_MapToRange(sdata->unk_8008d9f4[0], 0, 0x1900, 0x3f, 0xff);

		if ((canPlayFeedback != 0) && (selfDriver->kartState != KS_BLASTED) && (selfDriver->invincibleTimer == 0) && (otherDriver->kartState != KS_BLASTED) &&
		    (otherDriver->invincibleTimer == 0))
		{
			OtherFX_DriverCrashing((selfDriver->actionsFlagSet >> 0x10) & 1, volume);

			// NOTE(aalhendi): Retail uses DAT_8008d838. This field currently
			// names the same USA address as the last audioDefaults slot.
			sdata->audioDefaults[8] = sdata->gGT->frameTimer_MainFrame_ResetDB;

			if ((u32)volume > 0xdc)
			{
				Voiceline_RequestPlay(5, data.characterIDs[selfDriver->driverID], 0x10);
			}
		}
	}

	GAMEPAD_ShockFreq(otherDriver, 8, 0);
	GAMEPAD_ShockForce1(otherDriver, 8, 0x7f);
	GAMEPAD_JogCon1(otherDriver, (otherDriver->simpTurnState > 0) ? 0x29 : 0x19, 0x60);

	GAMEPAD_ShockFreq(selfDriver, 8, 0);
	GAMEPAD_ShockForce1(selfDriver, 8, 0x7f);
	GAMEPAD_JogCon1(selfDriver, (selfDriver->simpTurnState > 0) ? 0x29 : 0x19, 0x60);

	selfDriver->actionsFlagSet |= 0x10000000;
	otherDriver->actionsFlagSet |= 0x10000000;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d404-0x8005e104
void VehPhysCrash_AnyTwoCars(struct Thread *thread, struct DriverCollisionSearch *search, Vec3 *selfVel)
{
	int distance = VehCalc_FastSqrt(search->bucket.radius, 0);
	s16 *dist = &search->bucket.distX;
	s16 *hitDir = &search->hitDir[0];

	if (distance == 0)
	{
		hitDir[0] = 0;
		hitDir[1] = 0;
		hitDir[2] = 0x1000;
	}
	else
	{
		hitDir[0] = (s16)CTR_MipsDiv(CTR_MipsSll(dist[0], 0xc), distance);
		hitDir[1] = (s16)CTR_MipsDiv(CTR_MipsSll(dist[1], 0xc), distance);
		hitDir[2] = (s16)CTR_MipsDiv(CTR_MipsSll(dist[2], 0xc), distance);
	}

	struct Thread *otherThread = search->bucket.th;
	struct Driver *otherDriver = otherThread->object;
	struct Driver *selfDriver = thread->object;

	int hitStrength = CTR_MipsSubLo(CTR_MipsAddLo(thread->driver_HitRadius, otherThread->driver_HitRadius), distance);
	if (hitStrength <= 0)
	{
		return;
	}

	sdata->unk_8008d9f4[0] = 0;

	if ((selfDriver->actionsFlagSet & ACTION_BOT) != 0)
	{
		Vec3 otherVel;
		Vec3 weightedVel;

		if ((otherDriver->actionsFlagSet & ACTION_BOT) == 0)
		{
			VehPhysForce_ConvertSpeedToVecOut(otherDriver, &otherVel);
			VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
			VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
			VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
			VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
			VehPhysCrash_AI(selfDriver, selfVel);
			VehPhysCrash_ConvertVecToSpeed(otherDriver, &otherVel);
		}
		else
		{
			otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.unk5bc.ai_accelAxis[0]);
			otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.unk5bc.ai_accelAxis[1]);
			otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.unk5bc.ai_accelAxis[2]);

			VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
			VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
			VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
			VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
			VehPhysCrash_AI(otherDriver, &otherVel);
			VehPhysCrash_AI(selfDriver, selfVel);
			BOTS_CollideWithOtherAI(selfDriver, otherDriver);
		}

		return;
	}

	if ((otherDriver->actionsFlagSet & ACTION_BOT) != 0)
	{
		Vec3 otherVel;
		Vec3 weightedVel;

		otherVel.x = CTR_MipsAddLo(otherDriver->xSpeed, otherDriver->botData.unk5bc.ai_accelAxis[0]);
		otherVel.y = CTR_MipsAddLo(otherDriver->ySpeed, otherDriver->botData.unk5bc.ai_accelAxis[1]);
		otherVel.z = CTR_MipsAddLo(otherDriver->zSpeed, otherDriver->botData.unk5bc.ai_accelAxis[2]);

		VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, &otherVel, otherDriver);
		VehPhysCrash_BouncePair(hitDir, &weightedVel, &otherVel, selfVel);
		VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
		VehPhysCrash_SubImpulse(&otherVel, hitDir, hitStrength);
		VehPhysCrash_AI(otherDriver, &otherVel);
	}
	else
	{
		Vec3 weightedVel;
		Vec3 *otherVel = &otherDriver->velocity;

		VehPhysCrash_WeightedVelocity(&weightedVel, selfVel, selfDriver, otherVel, otherDriver);
		VehPhysCrash_BouncePair(hitDir, &weightedVel, otherVel, selfVel);
		VehPhysCrash_AddImpulse(selfVel, hitDir, hitStrength);
		VehPhysCrash_SubImpulse(otherVel, hitDir, hitStrength);
	}

	u32 canPlayFeedback = ((u32)CTR_MipsSubLo(sdata->gGT->frameTimer_MainFrame_ResetDB, sdata->audioDefaults[8]) >= 3);

	VehPhysCrash_PlayHumanFeedback(thread, otherThread, selfDriver, otherDriver, canPlayFeedback);

	int attackResult = VehPhysCrash_Attack(selfDriver, otherDriver, canPlayFeedback, 0);
	VehPhysCrash_Attack(otherDriver, selfDriver, attackResult, 1);
}
