#include <common.h>

static u32 VehPhysCrash_LengthSq2(s32 x, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(z, z));
}

static u32 VehPhysCrash_LengthSq3(s32 x, s32 y, s32 z)
{
	return (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(y, y)), CTR_MipsMulLo(z, z));
}

static s32 VehPhysCrash_Dot3(s32 ax, s32 ay, s32 az, s32 bx, s32 by, s32 bz)
{
	return CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(ax, bx), CTR_MipsMulLo(ay, by)), CTR_MipsMulLo(az, bz));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005cd1c-0x8005cf64.
void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *vel)
{
	int speed2D = VehCalc_FastSqrt(VehPhysCrash_LengthSq2(vel->x, vel->z), 0x10);
	s16 speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(vel->x, vel->y, vel->z), 0x10) >> 8);

	d->speed = speed3D;
	d->axisRotationY = (s16)ratan2(CTR_MipsSll(vel->y, 8), speed2D);
	d->axisRotationX = (s16)ratan2(vel->x, vel->z);

	int projOnMovingDirAxis =
	    CTR_MipsSra(VehPhysCrash_Dot3(vel->x, vel->y, vel->z, d->matrixMovingDir.m[0][1], d->matrixMovingDir.m[1][1], d->matrixMovingDir.m[2][1]), 0xc);

	int projX = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[0][1], projOnMovingDirAxis), 0xc);
	int projY = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[1][1], projOnMovingDirAxis), 0xc);
	int projZ = CTR_MipsSra(CTR_MipsMulLo(d->matrixMovingDir.m[2][1], projOnMovingDirAxis), 0xc);

	speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), 0x10) >> 8);

	d->jumpHeightCurr = speed3D;
	if (projOnMovingDirAxis < 0)
	{
		d->jumpHeightCurr = (s16)CTR_MipsNegLo(speed3D);
	}

	projX = CTR_MipsSubLo(vel->x, projX);
	projY = CTR_MipsSubLo(vel->y, projY);
	projZ = CTR_MipsSubLo(vel->z, projZ);

	speed3D = (s16)(VehCalc_FastSqrt(VehPhysCrash_LengthSq3(projX, projY, projZ), 0x10) >> 8);

	d->speedApprox = speed3D;

	if (VehPhysCrash_Dot3(projX, projY, projZ, d->matrixMovingDir.m[0][2], d->matrixMovingDir.m[1][2], d->matrixMovingDir.m[2][2]) < 0)
	{
		d->speedApprox = (s16)CTR_MipsNegLo(speed3D);
	}
}
