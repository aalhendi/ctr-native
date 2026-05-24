#include <common.h>

static int VehPhysForce_TranslateMatrix_Abs(int value)
{
	return value < 0 ? -value : value;
}

static int VehPhysForce_TranslateMatrix_Div256TowardZero(int value)
{
	if (value < 0)
	{
		value += 0xff;
	}

	return value >> 8;
}

// NOTE(aalhendi): Native expression of retail gte_rtv0; retail reads MAC1-MAC3
// after rotating V0 by matrixFacingDir.
static Vec3 VehPhysForce_TranslateMatrix_RotateVector(const MATRIX *m, s16 vx, s16 vy, s16 vz)
{
	Vec3 out;

	out.x = ((int)m->m[0][0] * vx + (int)m->m[0][1] * vy + (int)m->m[0][2] * vz) >> 12;
	out.y = ((int)m->m[1][0] * vx + (int)m->m[1][1] * vy + (int)m->m[1][2] * vz) >> 12;
	out.z = ((int)m->m[2][0] * vx + (int)m->m[2][1] * vy + (int)m->m[2][2] * vz) >> 12;

	return out;
}

static struct MatrixND *VehPhysForce_TranslateMatrix_GetBakedEntry(u8 matrixArray, u8 matrixIndex)
{
	return &((struct MatrixND *)data.bakedGteMath[matrixArray].physEntry)[matrixIndex];
}

static u8 VehPhysForce_TranslateMatrix_RemapIndex(u8 matrixIndex, int fromArray, int toArray)
{
	int fromLast = data.bakedGteMath[fromArray].numEntries - 1;
	int blend = 0x100 - (((u32)matrixIndex << 8) / fromLast);

	if (blend < 0)
	{
		blend = 0;
	}
	if (blend > 0x100)
	{
		blend = 0x100;
	}

	return (u8)((blend * (data.bakedGteMath[toArray].numEntries - 1)) >> 8);
}

static void VehPhysForce_TranslateMatrix_ResetMatrixAnim(struct Driver *d)
{
	d->matrixArray = 0;
	d->matrixIndex = 0;
}

static void VehPhysForce_TranslateMatrix_UpdateSquashStretch(struct Instance *inst, struct Driver *d)
{
	if (d->kartState == KS_WARP_PAD)
	{
		return;
	}

	if ((d->kartState == KS_MASK_GRABBED) && ((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0))
	{
		int scaleXZ;

		inst->scale[1] = d->jumpSquishStretch + 0xccc;

		scaleXZ = 0xccc - VehPhysForce_TranslateMatrix_Div256TowardZero(d->jumpSquishStretch * 0x28);
		if (scaleXZ < 0x400)
		{
			scaleXZ = 0x400;
		}

		inst->scale[0] = scaleXZ;
		inst->scale[2] = scaleXZ;
		return;
	}

	int jumpHeightCurr = d->jumpHeightCurr;
	int targetSquish = -800;

	if ((d->actionsFlagSet & 0x400) == 0)
	{
		int smoothed = (d->jumpSquishStretch2 * 9 + jumpHeightCurr * 7) >> 4;
		int delta = (d->jumpSquishStretch2 - smoothed) * 4;

		if (VehPhysForce_TranslateMatrix_Abs(delta) < 0x960)
		{
			delta = 0;
		}

		if (((d->actionsFlagSet | d->actionsFlagSetPrevFrame) & 2) == 0)
		{
			if (delta < -800)
			{
				delta = -800;
			}
		}
		else if (delta < -0x640)
		{
			delta = -0x640;
		}

		if (delta > 800)
		{
			delta = 800;
		}

		targetSquish = delta;
	}

	if ((d->hazardTimer > 0) && ((d->hazardTimer & 0x80) == 0) && (targetSquish > -800))
	{
		targetSquish = -800;
	}

	if (((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0) && (jumpHeightCurr < 0))
	{
		int mapped = DECOMP_VehCalc_MapToRange(-jumpHeightCurr, 0, 0xa00, 0x280, 0x320);

		if (targetSquish < mapped)
		{
			targetSquish = mapped;
		}

		d->jumpSquishStretch2 = jumpHeightCurr;
	}

	if ((d->instTntRecv != NULL) && (d->instTntRecv->scale[1] < 2500))
	{
		targetSquish += (d->instTntRecv->scale[1] - 0x800) * 2;
	}

	if (VehPhysForce_TranslateMatrix_Abs(d->jumpSquishStretch) < VehPhysForce_TranslateMatrix_Abs(targetSquish))
	{
		d->jumpSquishStretch = targetSquish;
	}

	d->jumpSquishStretch = DECOMP_VehCalc_InterpBySpeed(d->jumpSquishStretch, 300, 0);
	d->jumpSquishStretch2 = (d->jumpSquishStretch2 * 9 + jumpHeightCurr * 7) >> 4;

	if (d->squishTimer != 0)
	{
		inst->scale[1] = 0;
	}
	else if (inst->scale[1] == 0)
	{
		if (d->instSelf->thread->modelIndex == 0x18)
		{
			OtherFX_Play_Echo(0x5b, 1, (d->actionsFlagSet >> 16) & 1);
		}

		inst->scale[1] = d->jumpSquishStretch + 0xccc;
		d->matrixArray = 5;
		d->matrixIndex = 0;
	}
	else
	{
		inst->scale[1] = DECOMP_VehCalc_InterpBySpeed(inst->scale[1], 0xa0, d->jumpSquishStretch + 0xccc);
	}

	int scaleXZ = 0xccc - VehPhysForce_TranslateMatrix_Div256TowardZero(d->jumpSquishStretch * 0xa0);
	inst->scale[0] = DECOMP_VehCalc_InterpBySpeed(inst->scale[0], 0xa0, scaleXZ);
	inst->scale[2] = DECOMP_VehCalc_InterpBySpeed(inst->scale[2], 0xa0, scaleXZ);
}

static void VehPhysForce_TranslateMatrix_UpdateMatrixAnimation(struct Driver *d)
{
	if ((d->reserves == 0) || (d->fireSpeed < d->const_Speed_ClassStat) || ((d->actionsFlagSet & 0x80) != 0))
	{
		if (d->matrixArray == 2)
		{
			d->matrixArray = 3;
			d->matrixIndex = 0;
		}
		else if (d->matrixArray == 1)
		{
			d->matrixIndex = VehPhysForce_TranslateMatrix_RemapIndex(d->matrixIndex, 1, 3);
			d->matrixArray = 3;
		}
		else if (d->matrixArray == 3)
		{
			d->matrixIndex++;
			if (d->matrixIndex >= data.bakedGteMath[3].numEntries)
			{
				VehPhysForce_TranslateMatrix_ResetMatrixAnim(d);
			}
		}
	}
	else
	{
		if (d->matrixArray == 1)
		{
			d->matrixIndex++;
			if (d->matrixIndex >= data.bakedGteMath[1].numEntries)
			{
				d->matrixArray = 2;
				d->matrixIndex = 0;
			}
		}
		else if (d->matrixArray == 3)
		{
			d->matrixIndex = VehPhysForce_TranslateMatrix_RemapIndex(d->matrixIndex, 3, 1);
			d->matrixArray = 1;
		}
		else if (d->matrixArray == 0)
		{
			d->matrixArray = 1;
			d->matrixIndex = 0;
		}
	}

	if (d->matrixArray == 5)
	{
		d->matrixIndex++;
		if (d->matrixIndex >= data.bakedGteMath[5].numEntries)
		{
			VehPhysForce_TranslateMatrix_ResetMatrixAnim(d);
		}
	}
}

static void VehPhysForce_TranslateMatrix_CopyFacingMatrix(struct Instance *inst, struct Driver *d)
{
	inst->matrix.m[0][0] = d->matrixFacingDir.m[0][0];
	inst->matrix.m[0][1] = d->matrixFacingDir.m[0][1];
	inst->matrix.m[0][2] = d->matrixFacingDir.m[0][2];
	inst->matrix.m[1][0] = d->matrixFacingDir.m[1][0];
	inst->matrix.m[1][1] = d->matrixFacingDir.m[1][1];
	inst->matrix.m[1][2] = d->matrixFacingDir.m[1][2];
	inst->matrix.m[2][0] = d->matrixFacingDir.m[2][0];
	inst->matrix.m[2][1] = d->matrixFacingDir.m[2][1];
	inst->matrix.m[2][2] = d->matrixFacingDir.m[2][2];
}

static void VehPhysForce_TranslateMatrix_UpdateInstanceMatrix(struct Instance *inst, struct Driver *d)
{
	int screenOffsetY = ((s8)d->Screen_OffsetY * 3) >> 3;

	if (d->matrixArray == 0)
	{
		VehPhysForce_TranslateMatrix_CopyFacingMatrix(inst, d);

		inst->matrix.t[0] = d->posCurr.x >> 8;
		inst->matrix.t[1] = (d->posCurr.y >> 8) + screenOffsetY;
		inst->matrix.t[2] = d->posCurr.z >> 8;
	}
	else
	{
		struct MatrixND *entry = VehPhysForce_TranslateMatrix_GetBakedEntry(d->matrixArray, d->matrixIndex);
		s16 *entryVec = (s16 *)entry;
		Vec3 rotated;

		MatrixRotate(&inst->matrix, &d->matrixFacingDir, (MATRIX *)(void *)((u8 *)entry + 8));

		rotated = VehPhysForce_TranslateMatrix_RotateVector(&d->matrixFacingDir, entryVec[0], entryVec[1], entryVec[2]);

		inst->matrix.t[0] = (d->posCurr.x + rotated.x) >> 8;
		inst->matrix.t[1] = ((d->posCurr.y + rotated.y) >> 8) + screenOffsetY;
		inst->matrix.t[2] = (d->posCurr.z + rotated.z) >> 8;
	}

	if (d->squishTimer != 0)
	{
		inst->matrix.t[0] += (d->AxisAngle2_normalVec[0] * 0x13) >> 12;
		inst->matrix.t[1] += (d->AxisAngle2_normalVec[1] * 0x13) >> 12;
		inst->matrix.t[2] += (d->AxisAngle2_normalVec[2] * 0x13) >> 12;
	}
}

static void VehPhysForce_TranslateMatrix_HideWake(struct Instance *inst, struct Driver *d)
{
	struct Instance *wake = d->wakeInst;

	if (inst->matrix.t[1] > 0)
	{
		inst->flags &= ~SPLIT_LINE;
	}

	if (wake != NULL)
	{
		wake->flags |= HIDE_MODEL;
		d->wakeScale = 0;
		wake->scale[0] = 0;
		wake->scale[2] = d->wakeScale;
	}
}

static void VehPhysForce_TranslateMatrix_SpawnWakeParticle(struct Driver *d)
{
	struct Particle *p = Particle_Init(0, sdata->gGT->iconGroup[9], &data.emSet_Falling[0]);

	if (p != NULL)
	{
		p->unk18 = d->instSelf->unk50;
		p->driverInst = d->instSelf;
		p->unk19 = d->driverID;
	}
}

static void VehPhysForce_TranslateMatrix_SetWakeRotation(struct Instance *wake, struct Driver *d)
{
	s32 sin = DECOMP_MATH_Sin(d->angle);
	s32 cos = DECOMP_MATH_Cos(d->angle);

	// Retail writes paired matrix shorts with word stores.
	*(s32 *)(void *)&wake->matrix.m[0][0] = cos;
	*(s32 *)(void *)&wake->matrix.m[0][2] = sin;
	*(s32 *)(void *)&wake->matrix.m[1][1] = 0x1000;
	*(s32 *)(void *)&wake->matrix.m[2][0] = -sin;
	wake->matrix.m[2][2] = (s16)cos;
}

static void VehPhysForce_TranslateMatrix_UpdateWake(struct Instance *inst, struct Driver *d)
{
	struct Instance *wake;

	if (inst->matrix.t[1] >= 0)
	{
		VehPhysForce_TranslateMatrix_HideWake(inst, d);
		return;
	}

	if ((inst->matrix.t[1] < -0x4f) || ((inst->flags & SPLIT_LINE) == 0))
	{
		VehPhysForce_TranslateMatrix_HideWake(inst, d);
		return;
	}

	wake = d->wakeInst;
	if (wake == NULL)
	{
		return;
	}

	wake->flags &= ~HIDE_MODEL;
	wake->unk50 = inst->unk50 + 1;
	wake->unk51 = inst->unk51 - 1;

	wake->matrix.t[0] = inst->matrix.t[0];
	wake->matrix.t[1] = 0;
	wake->matrix.t[2] = inst->matrix.t[2];

	VehPhysForce_TranslateMatrix_SetWakeRotation(wake, d);

	if (d->wakeScale == 0)
	{
		d->wakeScale = 0x1000;

		if (sdata->gGT->numPlyrCurrGame < 2)
		{
			if ((VehPhysForce_TranslateMatrix_Abs(d->speed) > 0xc00) && (d->posPrev.y > -0x200))
			{
				int i;

				for (i = 10; i > 0; i--)
				{
					VehPhysForce_TranslateMatrix_SpawnWakeParticle(d);
				}
			}
		}
	}
	else if (sdata->gGT->numPlyrCurrGame < 2)
	{
		if (VehPhysForce_TranslateMatrix_Abs(d->speed) > 0xc00)
		{
			VehPhysForce_TranslateMatrix_SpawnWakeParticle(d);
		}
	}

	wake->scale[0] = d->wakeScale;
	wake->scale[2] = d->wakeScale;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005ee34-0x8005f89c
void VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver)
{
	struct Instance *inst = thread->inst;

	VehPhysForce_TranslateMatrix_UpdateSquashStretch(inst, driver);
	VehPhysForce_RotAxisAngle(&driver->matrixFacingDir, driver->AxisAngle2_normalVec, driver->rotCurr.y);
	VehPhysForce_TranslateMatrix_UpdateMatrixAnimation(driver);
	VehPhysForce_TranslateMatrix_UpdateInstanceMatrix(inst, driver);
	VehPhysForce_TranslateMatrix_UpdateWake(inst, driver);
}

void DECOMP_VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver)
{
	VehPhysForce_TranslateMatrix(thread, driver);
}
