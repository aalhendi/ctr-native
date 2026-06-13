#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058f9c-0x8005900c.
int VehCalc_MapToRange(int val, int oldMin, int oldMax, int newMin, int newMax)
{
	if (val <= oldMin)
		return newMin;

	if (val >= oldMax)
		return newMax;

	int distFromBottom = CTR_MipsSubLo(val, oldMin);
	int newRange = CTR_MipsSubLo(newMax, newMin);
	int oldRange = CTR_MipsSubLo(oldMax, oldMin);
	int scaledDistance = CTR_MipsDiv(CTR_MipsMulLo(distFromBottom, newRange), oldRange);

	return CTR_MipsAddLo(newMin, scaledDistance);
}
