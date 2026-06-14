#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac638-0x800ac6b4.
void RB_Potion_OnShatter_TeethSearch(struct Instance *inst)
{
#define SPS ((struct ScratchpadStruct *)0x1f800108)

	SPS->Input1.pos.x = (s16)inst->matrix.t[0];
	SPS->Input1.pos.y = (s16)inst->matrix.t[1];
	SPS->Input1.pos.z = (s16)inst->matrix.t[2];
	SPS->Input1.hitRadius = 0x140;
	SPS->Input1.hitRadiusSquared = 0x19000;
	SPS->Input1.modelID = inst->model->id;

	SPS->Union.ThBuckColl.thread = inst->thread;
	SPS->Union.ThBuckColl.funcCallback = RB_Potion_OnShatter_TeethCallback;

	PROC_StartSearch_Self(SPS);
}
