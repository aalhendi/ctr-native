#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800616b0-0x800617cc.
int VehPhysJoystick_GetStrength(int val, int max, struct RacingWheelData *rwd)
{
	int dead;
	int range;
	int dist;

	if (rwd == 0)
	{
		dead = 0x30;
		range = 0x7f;
		dist = 0x5e;
	}

	else
	{
		dead = rwd->deadZone;
		range = rwd->range;
		dist = CTR_MipsSubLo(range, dead);
	}

	if (val < dead)
		return 0;

	dead = CTR_MipsSubLo(val, dead);

	if (range <= val)
		return max;

	int halfDist = CTR_MipsSra(CTR_MipsAddLo(dist, (u32)dist >> 31), 1);
	int maxFifth = max / 5;

	if (halfDist <= dead)
	{
		dead = CTR_MipsSubLo(dead, halfDist);
		dead = CTR_MipsMulLo(dead, CTR_MipsSubLo(max, maxFifth));
		dead = CTR_MipsSll(dead, 1);
		return CTR_MipsAddLo(CTR_MipsDiv(dead, dist), maxFifth);
	}

	dead = CTR_MipsMulLo(dead, CTR_MipsSll(maxFifth, 1));
	return CTR_MipsDiv(dead, dist);
}
