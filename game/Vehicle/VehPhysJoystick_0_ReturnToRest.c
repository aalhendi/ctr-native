#include <common.h>

extern struct RacingWheelData rwd_default;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006163c-0x800616b0.
int VehPhysJoystick_ReturnToRest(int stickVal, int half, struct RacingWheelData *rwd)
{
	if (rwd == 0)
		rwd = &rwd_default;

	stickVal = CTR_MipsSubLo(stickVal, rwd->gamepadCenter);

	if (stickVal < 0)
	{
		return CTR_MipsNegLo(VehCalc_MapToRange(CTR_MipsNegLo(stickVal), rwd->deadZone, rwd->range, 0, half));
	}

	return VehCalc_MapToRange(stickVal, rwd->deadZone, rwd->range, 0, half);
}

struct RacingWheelData rwd_default = {.gamepadCenter = 0x80,

                                      .deadZone = 0x30,

                                      .range = 0x7f};
