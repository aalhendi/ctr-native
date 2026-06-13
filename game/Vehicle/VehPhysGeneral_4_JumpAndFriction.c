#include <common.h>

static int VehPhysGeneral_Jump_Abs(int value)
{
	return value < 0 ? CTR_MipsNegLo(value) : value;
}

static int VehPhysGeneral_Jump_Div2TowardZero(int value)
{
	return CTR_MipsSra(CTR_MipsAddLo(value, (u32)value >> 31), 1);
}

static int VehPhysGeneral_Jump_Div4TowardZero(int value)
{
	if (value < 0)
	{
		value = CTR_MipsAddLo(value, 3);
	}

	return CTR_MipsSra(value, 2);
}

static u32 VehPhysGeneral_Jump_PackS16Pair(s32 lo, s32 hi)
{
	return ((u32)(u16)lo) | ((u32)(u16)hi << 16);
}

static Vec3 VehPhysGeneral_Jump_RotateLoadedVector(s16 vx, s16 vy, s16 vz)
{
	Vec3 out;

	MTC2(VehPhysGeneral_Jump_PackS16Pair(vx, vy), 0);
	MTC2((u32)(u16)vz, 1);
	gte_mvmva(1, 0, 0, 3, 0);
	out.x = MFC2_S(25);
	out.y = MFC2_S(26);
	out.z = MFC2_S(27);

	return out;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060630-0x80060f0c
void VehPhysGeneral_JumpAndFriction(struct Thread *t, struct Driver *d)
{
	(void)t;

	// Retail loads matrixMovingDir once, then reuses the same CP2 rotation regs
	// for every jump/friction impulse in this function.
	gte_SetRotMatrix(&d->matrixMovingDir);

	if ((d->kartState != KS_DRIFTING) && ((d->actionsFlagSet & 0x800000) == 0) && (d->reserves == 0))
	{
		int ampTurn = VehPhysGeneral_Jump_Abs(CTR_MipsSra((s16)d->ampTurnState, 8));

		int turnDecrease = VehCalc_MapToRange(ampTurn, 0, (u8)d->const_BackwardTurnRate, 0, (int)d->const_TurnDecreaseRate);
		int baseSpeed = d->baseSpeed;
		int absBaseSpeed = VehPhysGeneral_Jump_Abs(baseSpeed);

		if (absBaseSpeed < turnDecrease)
		{
			turnDecrease = absBaseSpeed;
		}

		if (baseSpeed < 0)
		{
			d->baseSpeed = (s16)CTR_MipsAddLo((u16)d->baseSpeed, turnDecrease);
		}
		else
		{
			d->baseSpeed = (s16)CTR_MipsSubLo((u16)d->baseSpeed, turnDecrease);
		}
	}

	if (d->set_0xF0_OnWallRub != 0)
	{
		if (d->scrubMeta8 < d->baseSpeed)
		{
			d->baseSpeed = d->scrubMeta8;
		}

		if (d->baseSpeed < CTR_MipsNegLo(d->scrubMeta8))
		{
			d->baseSpeed = (s16)CTR_MipsNegLo(d->scrubMeta8);
		}
	}

	Vec3 movement = d->velocity;
	int speedLoss = 0;

	if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		goto CHECK_FOR_ANY_JUMP;
	}

	int acceleration = 0;

	if (((d->stepFlagSet & 3) != 0) && (d->baseSpeed > 0))
	{
		acceleration = 8000;
	}
	else if (d->baseSpeed != 0)
	{
		if (((d->terrainMeta1->flags & 4) == 0) || (d->baseSpeed < 1) || (d->speedApprox >= 0))
		{
			int speedApprox = d->speedApprox;
			int absSpeedApprox = VehPhysGeneral_Jump_Abs(speedApprox);

			if ((absSpeedApprox > 0x2ff) && ((d->baseSpeed < 1) || (speedApprox < 1)) && ((d->baseSpeed >= 0) || (speedApprox >= 0)))
			{
				goto PROCESS_ACCEL;
			}
		}

		acceleration = CTR_MipsAddLo(d->const_Accel_ClassStat, CTR_MipsSll((s8)d->accelConst, 5) / 5);

		if ((d->stepFlagSet & 3) == 0)
		{
			if ((d->reserves != 0) && (d->baseSpeed > 0))
			{
				acceleration = d->const_Accel_Reserves;
			}

			int slowUntilSpeed = d->terrainMeta1->slowUntilSpeed;
			if ((slowUntilSpeed != 0x100) && ((d->actionsFlagSet & 0x800000) == 0))
			{
				acceleration = CTR_MipsSra(CTR_MipsMulLo(slowUntilSpeed, acceleration), 8);
			}
		}
		else if (d->baseSpeed > 0)
		{
			acceleration = 8000;
		}
	}

PROCESS_ACCEL:
{
	int forwardImpulse = CTR_MipsSra(CTR_MipsMulLo(acceleration, sdata->gGT->elapsedTimeMS), 5);
	Vec3 rotated = VehPhysGeneral_Jump_RotateLoadedVector(0, 0, (s16)forwardImpulse);

	if (d->baseSpeed < 0)
	{
		d->unk_offset3B2 = (s16)CTR_MipsNegLo(forwardImpulse);

		movement.x = CTR_MipsSubLo(movement.x, rotated.x);
		movement.y = CTR_MipsSubLo(movement.y, rotated.y);
		movement.z = CTR_MipsSubLo(movement.z, rotated.z);

		d->unkVectorX = (s16)CTR_MipsNegLo(rotated.x);
		d->unkVectorY = (s16)CTR_MipsNegLo(rotated.y);
		d->unkVectorZ = (s16)CTR_MipsNegLo(rotated.z);
	}
	else
	{
		d->unk_offset3B2 = (s16)forwardImpulse;

		movement.x = CTR_MipsAddLo(movement.x, rotated.x);
		movement.y = CTR_MipsAddLo(movement.y, rotated.y);
		movement.z = CTR_MipsAddLo(movement.z, rotated.z);

		d->unkVectorX = (s16)rotated.x;
		d->unkVectorY = (s16)rotated.y;
		d->unkVectorZ = (s16)rotated.z;
	}

	u32 movementLengthSq =
	    (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(movement.x, movement.x), CTR_MipsMulLo(movement.y, movement.y)), CTR_MipsMulLo(movement.z, movement.z));
	speedLoss = CTR_MipsSubLo((s32)(VehCalc_FastSqrt(movementLengthSq, 0x10) >> 8), VehPhysGeneral_Jump_Abs(d->baseSpeed));

	bool clampToForwardImpulse = forwardImpulse < speedLoss;
	if (speedLoss < 0)
	{
		speedLoss = 0;
		clampToForwardImpulse = forwardImpulse < 0;
	}
	if (clampToForwardImpulse)
	{
		speedLoss = forwardImpulse;
	}

	if (((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0) || (d->jump_ForcedMS == 0))
	{
		goto CHECK_FOR_ANY_JUMP;
	}

	if (d->jump_unknown != 0)
	{
		d->jump_unknown = 0x180;
	}

	if (d->kartState == KS_BLASTED)
	{
		GAMEPAD_ShockFreq(d, 8, 0);
		GAMEPAD_ShockForce1(d, 8, 0x7f);
	}
}

	goto PROCESS_JUMP;

CHECK_FOR_ANY_JUMP:
	if (((d->actionsFlagSet & 0x8000) != 0) && (d->heldItemID == 5))
	{
		d->actionsFlagSet &= ~0x8000u;

		if ((d->jump_CoyoteTimerMS != 0) && (d->jump_CooldownMS == 0))
		{
			d->jump_ForcedMS = 0xa0;

			int jumpForce = CTR_MipsAddLo(CTR_MipsSll(d->const_JumpForce, 3), d->const_JumpForce);
			d->jump_InitialVelY = (s16)VehPhysGeneral_Jump_Div4TowardZero(jumpForce);

			OtherFX_Play_Echo(9, 1, (d->actionsFlagSet >> 16) & 1);

			d->jump_unknown = 0x180;
			goto PROCESS_JUMP;
		}

		d->noItemTimer = 0;
	}

	if (d->forcedJump_trampoline == 0)
	{
		if ((d->jump_CoyoteTimerMS == 0) || (d->jump_TenBuffer == 0) || (d->jump_CooldownMS != 0))
		{
			if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0)
			{
				if ((d->underDriver != NULL) && (d->underDriver->mulNormVecY != 0))
				{
					int speedApprox = d->speedApprox;
					if (speedApprox < 0)
					{
						speedApprox = VehPhysGeneral_Jump_Abs(speedApprox);
					}

					s16 antiGravVelY = (s16)CTR_MipsSra(CTR_MipsMulLo(d->underDriver->mulNormVecY, speedApprox), 8);
					Vec3 rotated = VehPhysGeneral_Jump_RotateLoadedVector(0, antiGravVelY, 0);

					movement.x = CTR_MipsAddLo(movement.x, rotated.x);
					movement.y = CTR_MipsAddLo(movement.y, rotated.y);
					movement.z = CTR_MipsAddLo(movement.z, rotated.z);
				}
			}

			goto NOT_JUMPING;
		}

		d->jump_ForcedMS = 0xa0;
		d->numberOfJumps = (s16)CTR_MipsAddLo((u16)d->numberOfJumps, 1);
		d->jump_InitialVelY = d->const_JumpForce;

		OtherFX_Play_Echo(8, 1, (d->actionsFlagSet >> 16) & 1);
	}
	else
	{
		if ((d->jump_ForcedMS == 0) || (d->jump_InitialVelY == d->const_JumpForce))
		{
			OtherFX_Play(0x7e, 1);
		}

		d->jump_ForcedMS = 0xa0;

		int jumpForce = CTR_MipsAddLo(CTR_MipsSll(d->const_JumpForce, 1), d->const_JumpForce);
		if (d->forcedJump_trampoline == 2)
		{
			d->jump_unknown = 0x180;
			d->jump_InitialVelY = (s16)jumpForce;
		}
		else
		{
			d->jump_InitialVelY = (s16)VehPhysGeneral_Jump_Div2TowardZero(jumpForce);
		}

		d->forcedJump_trampoline = 0;
	}

PROCESS_JUMP:
	d->jump_CooldownMS = 0x180;
	d->jump_TenBuffer = 0;
	d->actionsFlagSet |= 0x480;

	int bestJumpVelY = 0;
	int jumpVelY = VehPhysGeneral_JumpGetVelY(d->AxisAngle4_normalVec, &movement);
	if (VehPhysGeneral_Jump_Abs(bestJumpVelY) < VehPhysGeneral_Jump_Abs(jumpVelY))
	{
		bestJumpVelY = jumpVelY;
	}

	s16 *normalVec = d->AxisAngle1_normalVec.v;
	if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		normalVec = d->AxisAngle2_normalVec;
	}

	jumpVelY = VehPhysGeneral_JumpGetVelY(normalVec, &movement);

	int jumpVelYSquared = CTR_MipsMulLo(bestJumpVelY, bestJumpVelY);
	if (VehPhysGeneral_Jump_Abs(bestJumpVelY) < VehPhysGeneral_Jump_Abs(jumpVelY))
	{
		jumpVelYSquared = CTR_MipsMulLo(jumpVelY, jumpVelY);
		bestJumpVelY = jumpVelY;
	}

	int verticalSpeed = VehCalc_FastSqrt((u32)CTR_MipsSra(CTR_MipsAddLo(jumpVelYSquared, CTR_MipsMulLo(d->jump_InitialVelY, d->jump_InitialVelY)), 8), 8);

	int maxVerticalSpeed = ((u8)sdata->gGT->level1->unk_18C) << 8;
	if (maxVerticalSpeed == 0)
	{
		maxVerticalSpeed = 0x3700;
	}
	else if (maxVerticalSpeed > 0x5000)
	{
		maxVerticalSpeed = 0x5000;
	}

	verticalSpeed = CTR_MipsSubLo(verticalSpeed, bestJumpVelY);
	if (maxVerticalSpeed < verticalSpeed)
	{
		verticalSpeed = maxVerticalSpeed;
	}

	if (movement.y < verticalSpeed)
	{
		movement.y = verticalSpeed;
	}

NOT_JUMPING:
	VehPhysCrash_ConvertVecToSpeed(d, &movement);

	int speed = CTR_MipsSubLo((u16)d->speed, speedLoss);
	d->speed = (s16)speed;
	if (d->speed < 0)
	{
		d->speed = 0;
	}

	int speedApprox = d->speedApprox;
	if (speedApprox < 0)
	{
		speedApprox = VehPhysGeneral_Jump_Abs(speedApprox);

		if (speedApprox < 0x100)
		{
			d->unk36E = (s16)CTR_MipsSubLo((u16)d->unk36E, CTR_MipsSra(d->unk36E, 3));
		}
		else
		{
			d->unk36E = (s16)((u32)CTR_MipsAddLo(CTR_MipsMulLo(d->unk36E, 0xd), CTR_MipsMulLo(sdata->gGT->timer & 7, 0x300)) >> 4);
		}
	}
	else
	{
		d->unk36E = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo(d->unk36E, 0xd), CTR_MipsMulLo(speedApprox, 3)), 4);
	}
}
