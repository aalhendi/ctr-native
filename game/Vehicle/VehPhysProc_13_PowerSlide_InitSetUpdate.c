#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063920-0x80063934.
void VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	d->funcPtrs[0] = 0;
	d->funcPtrs[1] = VehPhysProc_PowerSlide_Update;
}
