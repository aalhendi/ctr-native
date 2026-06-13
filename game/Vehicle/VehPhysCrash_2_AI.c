#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d0d0-0x8005d218.
void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel)
{
	sdata->unk_rot[0] = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[0], 4);
	sdata->unk_rot[1] = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[1], 4);
	sdata->unk_rot[2] = (s16)CTR_MipsSll(bot->botData.botNavFrame->rot[2], 4);

	// NOTE(aalhendi): Retail uses globals at 0x8009ae28 and 0x8009ae38.
	// `dataLibFiller` covers that exact EXE data range in ctr-native.
	int *forward = (int *)&sdata->dataLibFiller[0];
	MATRIX *matrix = (MATRIX *)&sdata->dataLibFiller[0x10];

	ConvertRotToMatrix(matrix, &sdata->unk_rot[0]);

	forward[0] = CTR_MipsSra(matrix->m[0][2], 4);
	forward[1] = CTR_MipsSra(matrix->m[1][2], 4);
	forward[2] = CTR_MipsSra(matrix->m[2][2], 4);

	int botSpeed =
	    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(forward[0], vel->x), CTR_MipsMulLo(forward[1], vel->y)), CTR_MipsMulLo(forward[2], vel->z)), 8);

	bot->botData.unk5bc.ai_speedLinear = botSpeed;
	bot->botData.unk5bc.ai_accelAxis[0] = CTR_MipsSubLo(vel->x, CTR_MipsSra(CTR_MipsMulLo(forward[0], botSpeed), 8));
	bot->botData.botFlags |= 8;
	bot->botData.unk5bc.ai_accelAxis[2] = CTR_MipsSubLo(vel->z, CTR_MipsSra(CTR_MipsMulLo(forward[2], botSpeed), 8));
}
