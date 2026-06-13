#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004caa8-0x8004cac8.
int UI_ConvertX_2(int oldPosX, int newPosX)
{
#define midpointX 0x100
	newPosX = (oldPosX - midpointX) * newPosX;

	// If new posX is on the left of the screen
	if (newPosX < 0)
	{
		newPosX += 0xff;
	}

	// divide by 256 (0x100)
	return newPosX >> 8;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004cac8-0x8004cae8.
int UI_ConvertY_2(int oldPosY, int newPosY)
{
#define midpointY 0x6c
	newPosY = (oldPosY - midpointY) * newPosY;

	// If new posY is under the screen
	if (newPosY < 0)
	{
		newPosY += 0xff;
	}

	// divide by 256 (0x100)
	return newPosY >> 8;
}
