#include <common.h>

static int VehPhysCrash_BounceSelf_Div6Shift9(int value)
{
	s64 product = (s64)value * 0x2aaaaaab;
	int high = (s32)((u64)product >> 32);

	return CTR_MipsSubLo(CTR_MipsSra(high, 9), CTR_MipsSra(value, 31));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005cf64-0x8005d0d0.
int VehPhysCrash_BounceSelf(s16 *normal, Vec3 *origin, Vec3 *vel, int boolOtherDriver)
{
	int diffX = CTR_MipsSubLo(vel->x, origin->x);
	int diffY = CTR_MipsSubLo(vel->y, origin->y);
	int diffZ = CTR_MipsSubLo(vel->z, origin->z);
	int dot = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(diffX, normal[0]), CTR_MipsMulLo(diffY, normal[1])), CTR_MipsMulLo(diffZ, normal[2])), 0xc);

	if (boolOtherDriver == 0)
	{
		if (dot >= 0)
		{
			return 0;
		}
	}
	else if (dot <= 0)
	{
		return 0;
	}

	int absDot = dot;
	if (absDot < 0)
	{
		absDot = CTR_MipsNegLo(absDot);
	}

	if (sdata->unk_8008d9f4[0] < absDot)
	{
		sdata->unk_8008d9f4[0] = absDot;
	}

	diffX = CTR_MipsSubLo(diffX, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal[0])));
	diffY = CTR_MipsSubLo(diffY, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal[1])));
	diffZ = CTR_MipsSubLo(diffZ, VehPhysCrash_BounceSelf_Div6Shift9(CTR_MipsMulLo(dot, normal[2])));

	vel->x = CTR_MipsAddLo(diffX, origin->x);

	int oldY = vel->y;
	int newY = CTR_MipsAddLo(diffY, origin->y);
	if ((oldY < newY) && (newY > 0x3200))
	{
		newY = 0x3200;
	}
	vel->y = newY;
	vel->z = CTR_MipsAddLo(diffZ, origin->z);

	return 0;
}
