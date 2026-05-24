#include <common.h>

static int VehPhysGeneral_Jump_Abs(int value)
{
	return value < 0 ? -value : value;
}

static int VehPhysGeneral_Jump_Div2TowardZero(int value)
{
	return (value + ((u32)value >> 31)) >> 1;
}

static int VehPhysGeneral_Jump_Div4TowardZero(int value)
{
	if (value < 0)
	{
		value += 3;
	}

	return value >> 2;
}

// NOTE(aalhendi): Native expression of retail gte_rtv0; retail reads MAC1-MAC3
// after rotating V0 by matrixMovingDir.
static Vec3 VehPhysGeneral_Jump_RotateVector(const MATRIX *m, s16 vx, s16 vy, s16 vz)
{
	Vec3 out;

	out.x = ((int)m->m[0][0] * vx + (int)m->m[0][1] * vy + (int)m->m[0][2] * vz) >> 12;
	out.y = ((int)m->m[1][0] * vx + (int)m->m[1][1] * vy + (int)m->m[1][2] * vz) >> 12;
	out.z = ((int)m->m[2][0] * vx + (int)m->m[2][1] * vy + (int)m->m[2][2] * vz) >> 12;

	return out;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060630-0x80060f0c
void VehPhysGeneral_JumpAndFriction(struct Thread *t, struct Driver *d)
{
	(void)t;

	if ((d->kartState != KS_DRIFTING) && ((d->actionsFlagSet & 0x800000) == 0) && (d->reserves == 0))
	{
		int ampTurn = (s16)d->ampTurnState >> 8;
		if (ampTurn < 0)
		{
			ampTurn = -ampTurn;
		}

		int turnDecrease = DECOMP_VehCalc_MapToRange(ampTurn, 0, (u8)d->const_BackwardTurnRate, 0, (int)d->const_TurnDecreaseRate);
		int baseSpeed = d->baseSpeed;
		int absBaseSpeed = baseSpeed;
		if (absBaseSpeed < 0)
		{
			absBaseSpeed = -absBaseSpeed;
		}

		if (absBaseSpeed < turnDecrease)
		{
			turnDecrease = absBaseSpeed;
		}

		s16 speedDelta = -(s16)turnDecrease;
		if (baseSpeed < 0)
		{
			speedDelta = (s16)turnDecrease;
		}

		d->baseSpeed += speedDelta;
	}

	if (d->set_0xF0_OnWallRub != 0)
	{
		if (d->scrubMeta8 < d->baseSpeed)
		{
			d->baseSpeed = d->scrubMeta8;
		}

		if (d->baseSpeed < -d->scrubMeta8)
		{
			d->baseSpeed = -d->scrubMeta8;
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

		acceleration = (int)d->const_Accel_ClassStat + (((int)d->accelConst << 5) / 5);

		if ((d->stepFlagSet & 3) == 0)
		{
			if ((d->reserves != 0) && (d->baseSpeed > 0))
			{
				acceleration = d->const_Accel_Reserves;
			}

			int slowUntilSpeed = d->terrainMeta1->slowUntilSpeed;
			if ((slowUntilSpeed != 0x100) && ((d->actionsFlagSet & 0x800000) == 0))
			{
				acceleration = (slowUntilSpeed * acceleration) >> 8;
			}
		}
		else if (d->baseSpeed > 0)
		{
			acceleration = 8000;
		}
	}

PROCESS_ACCEL:
{
	int forwardImpulse = (acceleration * sdata->gGT->elapsedTimeMS) >> 5;
	Vec3 rotated = VehPhysGeneral_Jump_RotateVector(&d->matrixMovingDir, 0, 0, (s16)forwardImpulse);

	if (d->baseSpeed < 0)
	{
		d->unk_offset3B2 = -(s16)forwardImpulse;

		movement.x -= rotated.x;
		movement.y -= rotated.y;
		movement.z -= rotated.z;

		d->unkVectorX = -(s16)rotated.x;
		d->unkVectorY = -(s16)rotated.y;
		d->unkVectorZ = -(s16)rotated.z;
	}
	else
	{
		d->unk_offset3B2 = (s16)forwardImpulse;

		movement.x += rotated.x;
		movement.y += rotated.y;
		movement.z += rotated.z;

		d->unkVectorX = (s16)rotated.x;
		d->unkVectorY = (s16)rotated.y;
		d->unkVectorZ = (s16)rotated.z;
	}

	speedLoss =
	    (int)(VehCalc_FastSqrt(movement.x * movement.x + movement.y * movement.y + movement.z * movement.z, 0x10) >> 8) - VehPhysGeneral_Jump_Abs(d->baseSpeed);

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
		DECOMP_GAMEPAD_ShockFreq(d, 8, 0);
		DECOMP_GAMEPAD_ShockForce1(d, 8, 0x7f);
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

			int jumpForce = d->const_JumpForce * 9;
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
						speedApprox = -speedApprox;
					}

					s16 antiGravVelY = (s16)((d->underDriver->mulNormVecY * speedApprox) >> 8);
					Vec3 rotated = VehPhysGeneral_Jump_RotateVector(&d->matrixMovingDir, 0, antiGravVelY, 0);

					movement.x += rotated.x;
					movement.y += rotated.y;
					movement.z += rotated.z;
				}
			}

			goto NOT_JUMPING;
		}

		d->jump_ForcedMS = 0xa0;
		d->numberOfJumps++;
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

		int jumpForce = d->const_JumpForce * 3;
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

	int jumpVelYSquared = bestJumpVelY * bestJumpVelY;
	if (VehPhysGeneral_Jump_Abs(bestJumpVelY) < VehPhysGeneral_Jump_Abs(jumpVelY))
	{
		jumpVelYSquared = jumpVelY * jumpVelY;
		bestJumpVelY = jumpVelY;
	}

	int verticalSpeed = VehCalc_FastSqrt((jumpVelYSquared + (int)d->jump_InitialVelY * (int)d->jump_InitialVelY) >> 8, 8);

	int maxVerticalSpeed = ((u8)sdata->gGT->level1->unk_18C) << 8;
	if (maxVerticalSpeed == 0)
	{
		maxVerticalSpeed = 0x3700;
	}
	else if (maxVerticalSpeed > 0x5000)
	{
		maxVerticalSpeed = 0x5000;
	}

	verticalSpeed -= bestJumpVelY;
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

	int speed = (u16)d->speed - speedLoss;
	d->speed = (s16)speed;
	if (d->speed < 0)
	{
		d->speed = 0;
	}

	int speedApprox = d->speedApprox;
	if (speedApprox < 0)
	{
		speedApprox = -speedApprox;

		if (speedApprox < 0x100)
		{
			d->unk36E = (s16)((u16)d->unk36E - (d->unk36E >> 3));
		}
		else
		{
			d->unk36E = (s16)(((u32)((int)d->unk36E * 0xd + (sdata->gGT->timer & 7) * 0x300)) >> 4);
		}
	}
	else
	{
		d->unk36E = (s16)(((int)d->unk36E * 0xd + speedApprox * 3) >> 4);
	}
}

void DECOMP_VehPhysGeneral_JumpAndFriction(struct Thread *t, struct Driver *d)
{
	VehPhysGeneral_JumpAndFriction(t, d);
}
