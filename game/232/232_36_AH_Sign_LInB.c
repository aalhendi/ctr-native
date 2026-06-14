#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 232 0x800b4c80-0x800b4ddc.
void AH_Sign_LInB(struct Instance *inst)
{
	s16 *normal;
	u8 *scratch;
	struct ScratchpadStruct *sps;

	scratch = CTR_SCRATCHPAD_BASE;
	normal = (s16 *)&scratch[0x118];
	sps = (struct ScratchpadStruct *)&scratch[0x120];

	normal[0] = inst->matrix.m[0][2] >> 6;
	normal[1] = inst->matrix.m[1][2] >> 6;
	normal[2] = inst->matrix.m[2][2] >> 6;

	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	*(s16 *)&scratch[0x108] = inst->matrix.t[0] + normal[0] * 2;
	*(s16 *)&scratch[0x110] = *(s16 *)&scratch[0x108] - normal[0] * 4;

	*(s16 *)&scratch[0x10a] = inst->matrix.t[1] + normal[1] * 2;
	*(s16 *)&scratch[0x112] = *(s16 *)&scratch[0x10a] - normal[1] * 4;

	*(s16 *)&scratch[0x10c] = inst->matrix.t[2] + normal[2] * 2;
	*(s16 *)&scratch[0x114] = *(s16 *)&scratch[0x10c] - normal[2] * 4;

	COLL_SearchBSP_CallbackQUADBLK((u32 *)&scratch[0x108], (u32 *)&scratch[0x110], sps, 0);

	if (sps->boolDidTouchQuadblock != 0)
	{
		normal[0] = -normal[0];
		normal[1] = -normal[1];
		normal[2] = -normal[2];
	}

	inst->bitCompressed_NormalVector_AndDriverIndex = ((u16)normal[0] & 0xff) | (((u16)normal[1] & 0xff) << 8) | (((u16)normal[2] & 0xff) << 0x10);
}
