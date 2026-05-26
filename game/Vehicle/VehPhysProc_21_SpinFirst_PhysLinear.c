#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063d44-0x80063dc8.
void VehPhysProc_SpinFirst_PhysLinear(struct Thread *t, struct Driver *d)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	d->NoInputTimer -= elapsedTimeMS;
	if (d->NoInputTimer < 0)
		d->NoInputTimer = 0;

	VehPhysProc_Driving_PhysLinear(t, d);

	// baseSpeed and fireSpeed
	// set both "shorts" in one "int"
	*(int *)&d->baseSpeed = 0;

	d->actionsFlagSet |= 0x5808;

	d->timeSpentSpinningOut += elapsedTimeMS;
}
