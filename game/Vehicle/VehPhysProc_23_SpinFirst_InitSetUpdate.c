#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063eac-0x80063ec0.
void VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *t, struct Driver *d)
{
	d->funcPtrs[0] = 0;
	d->funcPtrs[1] = VehPhysProc_SpinFirst_Update;
}
