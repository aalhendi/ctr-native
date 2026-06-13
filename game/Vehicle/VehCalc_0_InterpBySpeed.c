#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058f54-0x80058f9c.
int VehCalc_InterpBySpeed(int val, int speed, int desired)
{
	if (val > desired)
	{
		val = CTR_MipsSubLo(val, speed);

		if (val < desired)
			return desired;
	}

	else
	{
		val = CTR_MipsAddLo(val, speed);

		if (val > desired)
			return desired;
	}

	return val;
}
