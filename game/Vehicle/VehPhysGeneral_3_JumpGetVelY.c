#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800605a0-0x80060630.
int VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ)
{
	int normalY = normalVec[1];
	int absNormalY = normalY;

	if (absNormalY < 0)
	{
		absNormalY = CTR_MipsNegLo(absNormalY);
	}

	if (absNormalY < 0x15)
	{
		return 0;
	}

	int dot = CTR_MipsAddLo(CTR_MipsMulLo(speedXYZ->x, normalVec[0]), CTR_MipsMulLo(speedXYZ->z, normalVec[2]));

	return CTR_MipsDiv(dot, normalY);
}
