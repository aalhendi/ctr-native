#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800638d4-0x80063920.
void VehPhysProc_PowerSlide_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	VehPhysProc_Driving_PhysLinear(thread, driver);
	driver->actionsFlagSet |= 0x1800;
	driver->timeSpentDrifting += sdata->gGT->elapsedTimeMS;
}
