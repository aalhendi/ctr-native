#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063634-0x8006364c.
void VehPhysProc_PowerSlide_Finalize(struct Driver *d)
{
	d->timeUntilDriftSpinout = d->unk46b << 5;
	d->previousFrameMultDrift = d->multDrift;
}
