#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800617cc-0x8006181c.
int VehPhysJoystick_GetStrengthAbsolute(int stickVal, int maxSteer, struct RacingWheelData *rwd)
{
	int center = 0x80;
	if (rwd != NULL)
		center = rwd->gamepadCenter;

	int distFromCenter = CTR_MipsSubLo(stickVal, center);

	// if steering right
	if (distFromCenter < 0)
	{
		return CTR_MipsNegLo(VehPhysJoystick_GetStrength(CTR_MipsNegLo(distFromCenter), maxSteer, rwd));
	}

	// steer left
	return VehPhysJoystick_GetStrength(distFromCenter, maxSteer, rwd);
}
